#include "stdafx.h"
#include "D3D8PortraitFix.h"
#include "Client.h"
#include "Memory.h"
// Forward declaration (defined in ReplacementFuncs.h, which cannot be
// included here: it holds non-inline definitions owned by dllmain.cpp).
DWORD GetFuncAddress(LPCSTR lpModule, LPCSTR lpFunc);
#include <vector>

namespace D3D8PortraitFix {
namespace {

// ---------------------------------------------------------------- monitor pick

struct MonitorInfo {
    HMONITOR h;
    RECT rcWork;
};

BOOL CALLBACK EnumMonitors(HMONITOR h, HDC, LPRECT, LPARAM l) {
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfo(h, &mi)) {
        MonitorInfo info;
        info.h = h;
        info.rcWork = mi.rcWork;
        ((std::vector<MonitorInfo>*)l)->push_back(info);
    }
    return TRUE;
}

// Best work rect for a window of (wantW, wantH). Prefers a monitor whose
// orientation matches the request, then the largest fitting one.
// Falls back to the largest monitor when nothing fits.
bool PickMonitorFor(int wantW, int wantH, RECT& outWork) {
    std::vector<MonitorInfo> mons;
    EnumDisplayMonitors(NULL, NULL, EnumMonitors, (LPARAM)&mons);
    if (mons.empty())
        return false;
    bool wantLandscape = wantW >= wantH;
    const MonitorInfo* best = NULL;
    int bestArea = -1;
    for (size_t pass = 0; pass < 3 && !best; ++pass) {
        bestArea = -1;
        for (size_t i = 0; i < mons.size(); ++i) {
            int w = mons[i].rcWork.right - mons[i].rcWork.left;
            int h = mons[i].rcWork.bottom - mons[i].rcWork.top;
            bool fits = (w >= wantW && h >= wantH);
            bool orient = ((w >= h) == wantLandscape);
            if (pass == 0 && !(fits && orient))
                continue;
            if (pass == 1 && !fits)
                continue;
            int area = w * h;
            if (area > bestArea) {
                bestArea = area;
                best = &mons[i];
            }
        }
    }
    if (!best)
        return false;
    outWork = best->rcWork;
    return true;
}

// ---------------------------------------------------------------- D3D8 wrap
// IDirect3D8 vtable order from d3d8.h (16 entries, stdcall on x86).
enum {
    VT_QueryInterface = 0,
    VT_AddRef = 1,
    VT_Release = 2,
    VT_RegisterSoftwareDevice = 3,
    VT_GetAdapterCount = 4,
    VT_GetAdapterIdentifier = 5,
    VT_GetAdapterModeCount = 6,
    VT_EnumAdapterModes = 7,
    VT_GetAdapterDisplayMode = 8,
    VT_CheckDeviceType = 9,
    VT_CheckDeviceFormat = 10,
    VT_CheckDeviceMultiSampleType = 11,
    VT_CheckDepthStencilMatch = 12,
    VT_GetDeviceCaps = 13,
    VT_GetAdapterMonitor = 14,
    VT_CreateDevice = 15,
    VT_Count = 16,
};

struct D3D8Mode {
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    DWORD Format;
};

// D3DERR_INVALIDCALL, returned by EnumAdapterModes past the last mode.
const HRESULT kD3DInvalidCall = (HRESULT)0x8876086C;
// D3DFMT_X8R8G8B8, fallback backbuffer format for spoofed modes.
const DWORD kFormatX8R8G8B8 = 22;

typedef void* (WINAPI* PfnD3DCreate8)(UINT sdkVersion);
typedef UINT (WINAPI* PfnGetAdapterCount)(void* self);
typedef HMONITOR (WINAPI* PfnGetAdapterMonitor)(void* self, UINT adapter);
typedef UINT (WINAPI* PfnGetAdapterModeCount)(void* self, UINT adapter);
typedef HRESULT (WINAPI* PfnEnumAdapterModes)(void* self, UINT adapter, UINT mode, D3D8Mode* pMode);
typedef HRESULT (WINAPI* PfnGetAdapterDisplayMode)(void* self, UINT adapter, D3D8Mode* pMode);
typedef HRESULT (WINAPI* PfnCheckDeviceType)(void* self, UINT adapter, UINT devType, DWORD displayFmt, DWORD backBufFmt, BOOL windowed);
typedef HRESULT (WINAPI* PfnGetAdapterIdentifier)(void* self, UINT adapter, DWORD flags, void* pId);
typedef HRESULT (WINAPI* PfnCheckDeviceFormat)(void* self, UINT adapter, UINT devType, DWORD adapterFmt, DWORD usage, DWORD resType, DWORD checkFmt);
typedef HRESULT (WINAPI* PfnCheckMultiSample)(void* self, UINT adapter, UINT devType, DWORD surfaceFmt, BOOL windowed, DWORD multiSample);
typedef HRESULT (WINAPI* PfnCheckDepthStencil)(void* self, UINT adapter, UINT devType, DWORD adapterFmt, DWORD rtFmt, DWORD dsFmt);
typedef HRESULT (WINAPI* PfnGetDeviceCaps)(void* self, UINT adapter, UINT devType, void* pCaps);
typedef HRESULT (WINAPI* PfnCreateDevice)(void* self, UINT adapter, UINT devType, HWND focus, DWORD behavior, void* pParams, void** ppDevice);

PfnD3DCreate8 g_realCreate8 = NULL;
void* g_vtableOrig[VT_Count] = { 0 };
LONG g_wrapLock = 0;
bool g_vtPatched = false;
UINT g_adapter = 0;
int g_fallbackAdapter = -1;
bool g_spoofModes = false;
D3D8Mode g_displayMode = { 0, 0, 0, 0 };
UINT g_realModeCount = 0;

bool EnsureRealFactory() {
    if (g_realCreate8)
        return true;
    wchar_t path[MAX_PATH];
    if (!GetSystemDirectoryW(path, MAX_PATH))
        return false;
    wcscat_s(path, L"\\d3d8.dll");
    HMODULE mod = LoadLibraryW(path);
    if (!mod)
        return false;
    g_realCreate8 = (PfnD3DCreate8)(void*)::GetProcAddress(mod, "Direct3DCreate8");
    return g_realCreate8 != NULL;
}

void GameSize(int& w, int& h) {
    w = Client::m_nGameWidth;
    h = Client::m_nGameHeight;
    if (w <= 0 || h <= 0) {
        w = 1280;
        h = 720;
    }
}

// Portrait-first policy: windowed D3D8 device creation works fine on a
// portrait adapter (verified with a standalone 32-bit d3d8 test), so the
// game always stays on adapter 0. Only the mode enumeration needs help:
// Gr2D's "find proper screen mode" scan fails whenever the game
// resolution is absent from the adapter's mode list (portrait adapters
// report portrait modes only; some virtual adapters skip modes like
// 1280x720), so the missing sizes are appended transparently.
void ChooseAdapter(void* d3d) {
    int wantW, wantH;
    GameSize(wantW, wantH);
    g_adapter = 0;
    g_spoofModes = false;
    g_fallbackAdapter = -1;
    UINT count = ((PfnGetAdapterCount)g_vtableOrig[VT_GetAdapterCount])(d3d);
    for (UINT i = 0; i < count; ++i) {
        HMONITOR hm = ((PfnGetAdapterMonitor)g_vtableOrig[VT_GetAdapterMonitor])(d3d, i);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (!hm || !GetMonitorInfo(hm, &mi))
            continue;
        int w = mi.rcMonitor.right - mi.rcMonitor.left;
        int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
        if (i == 0) {
            // Spoof when the primary adapter is portrait: its list hides
            // every landscape size.
            g_spoofModes = (h > w);
        } else if (w >= h && w >= wantW && h >= wantH) {
            // Remember a landscape adapter as a CreateDevice fallback.
            if (g_fallbackAdapter < 0)
                g_fallbackAdapter = (int)i;
        }
    }
    if (((PfnGetAdapterDisplayMode)g_vtableOrig[VT_GetAdapterDisplayMode])(d3d, g_adapter, &g_displayMode) != S_OK) {
        g_displayMode.Width = 0;
        g_displayMode.Height = 0;
        g_displayMode.RefreshRate = 0;
        g_displayMode.Format = 0;
    }
    if (g_displayMode.Format == 0)
        g_displayMode.Format = kFormatX8R8G8B8;
    g_realModeCount = ((PfnGetAdapterModeCount)g_vtableOrig[VT_GetAdapterModeCount])(d3d, g_adapter);
    if (!g_spoofModes) {
        // Spoof whenever the game resolution is missing from the real
        // list (e.g. virtual adapters without 1280x720).
        bool found = false;
        for (UINT i = 0; i < g_realModeCount && !found; ++i) {
            D3D8Mode m = { 0, 0, 0, 0 };
            if (((PfnEnumAdapterModes)g_vtableOrig[VT_EnumAdapterModes])(d3d, g_adapter, i, &m) == S_OK &&
                (int)m.Width == wantW && (int)m.Height == wantH)
                found = true;
        }
        g_spoofModes = !found;
    }
}

UINT MapAdapter(UINT adapter) {
    (void)adapter;
    return g_adapter;
}

HRESULT WINAPI F_EnumAdapterModes(void* self, UINT adapter, UINT mode, D3D8Mode* pMode) {
    UINT real = MapAdapter(adapter);
    HRESULT hr = ((PfnEnumAdapterModes)g_vtableOrig[VT_EnumAdapterModes])(self, real, mode, pMode);
    if (SUCCEEDED(hr) || !g_spoofModes || real != 0 || !pMode)
        return hr;
    // Append a few landscape modes so Gr2D finds the game resolution.
    int wantW, wantH;
    GameSize(wantW, wantH);
    // Each size is reported at 60 Hz and at the display refresh rate:
    // Gr2D wants the rate forced by Client::RefreshRate (60), but fall
    // back to the display rate when that hook has not run (yet).
    static const UINT kExtra[][2] = { { 0, 0 }, { 800, 600 }, { 1024, 768 }, { 1280, 720 }, { 1366, 768 } };
    static const UINT kExtraCount = sizeof(kExtra) / sizeof(kExtra[0]);
    UINT extra = mode - g_realModeCount;
    if (mode < g_realModeCount || extra >= kExtraCount * 2)
        return kD3DInvalidCall;
    UINT sizeIdx = extra / 2;
    pMode->Width = (sizeIdx == 0) ? (UINT)wantW : kExtra[sizeIdx][0];
    pMode->Height = (sizeIdx == 0) ? (UINT)wantH : kExtra[sizeIdx][1];
    pMode->RefreshRate = (extra % 2 == 0) ? 60 : g_displayMode.RefreshRate;
    pMode->Format = g_displayMode.Format;
    return S_OK;
}

UINT WINAPI F_GetAdapterModeCount(void* self, UINT adapter) {
    UINT n = ((PfnGetAdapterModeCount)g_vtableOrig[VT_GetAdapterModeCount])(self, MapAdapter(adapter));
    return g_spoofModes ? n + 10 : n;
}

HRESULT WINAPI F_GetAdapterDisplayMode(void* self, UINT adapter, D3D8Mode* pMode) {
    return ((PfnGetAdapterDisplayMode)g_vtableOrig[VT_GetAdapterDisplayMode])(self, MapAdapter(adapter), pMode);
}

HRESULT WINAPI F_CheckDeviceType(void* self, UINT adapter, UINT devType, DWORD displayFmt, DWORD backBufFmt, BOOL windowed) {
    return ((PfnCheckDeviceType)g_vtableOrig[VT_CheckDeviceType])(self, MapAdapter(adapter), devType, displayFmt, backBufFmt, windowed);
}

HRESULT WINAPI F_GetAdapterIdentifier(void* self, UINT adapter, DWORD flags, void* pId) {
    return ((PfnGetAdapterIdentifier)g_vtableOrig[VT_GetAdapterIdentifier])(self, MapAdapter(adapter), flags, pId);
}

HRESULT WINAPI F_CheckDeviceFormat(void* self, UINT adapter, UINT devType, DWORD adapterFmt, DWORD usage, DWORD resType, DWORD checkFmt) {
    return ((PfnCheckDeviceFormat)g_vtableOrig[VT_CheckDeviceFormat])(self, MapAdapter(adapter), devType, adapterFmt, usage, resType, checkFmt);
}

HRESULT WINAPI F_CheckMultiSample(void* self, UINT adapter, UINT devType, DWORD surfaceFmt, BOOL windowed, DWORD multiSample) {
    return ((PfnCheckMultiSample)g_vtableOrig[VT_CheckDeviceMultiSampleType])(self, MapAdapter(adapter), devType, surfaceFmt, windowed, multiSample);
}

HRESULT WINAPI F_CheckDepthStencil(void* self, UINT adapter, UINT devType, DWORD adapterFmt, DWORD rtFmt, DWORD dsFmt) {
    return ((PfnCheckDepthStencil)g_vtableOrig[VT_CheckDepthStencilMatch])(self, MapAdapter(adapter), devType, adapterFmt, rtFmt, dsFmt);
}

HRESULT WINAPI F_GetDeviceCaps(void* self, UINT adapter, UINT devType, void* pCaps) {
    return ((PfnGetDeviceCaps)g_vtableOrig[VT_GetDeviceCaps])(self, MapAdapter(adapter), devType, pCaps);
}

HMONITOR WINAPI F_GetAdapterMonitor(void* self, UINT adapter) {
    return ((PfnGetAdapterMonitor)g_vtableOrig[VT_GetAdapterMonitor])(self, MapAdapter(adapter));
}

HRESULT WINAPI F_CreateDevice(void* self, UINT adapter, UINT devType, HWND focus, DWORD behavior, void* pParams, void** ppDevice) {
    HRESULT hr = ((PfnCreateDevice)g_vtableOrig[VT_CreateDevice])(self, MapAdapter(adapter), devType, focus, behavior, pParams, ppDevice);
    if (FAILED(hr) && g_adapter == 0 && g_fallbackAdapter >= 0) {
        // Portrait adapter refused the device: retry once on the
        // landscape fallback and stay there for the rest of the session.
        hr = ((PfnCreateDevice)g_vtableOrig[VT_CreateDevice])(self, (UINT)g_fallbackAdapter, devType, focus, behavior, pParams, ppDevice);
        if (SUCCEEDED(hr))
            g_adapter = (UINT)g_fallbackAdapter;
    }
    return hr;
}

void PatchVTable(void* d3d) {
    void** vt = *(void***)d3d;
    DWORD oldProtect = 0;
    VirtualProtect(vt, sizeof(g_vtableOrig), PAGE_READWRITE, &oldProtect);
    memcpy(g_vtableOrig, vt, sizeof(g_vtableOrig));
    vt[VT_GetAdapterIdentifier] = (void*)F_GetAdapterIdentifier;
    vt[VT_GetAdapterModeCount] = (void*)F_GetAdapterModeCount;
    vt[VT_EnumAdapterModes] = (void*)F_EnumAdapterModes;
    vt[VT_GetAdapterDisplayMode] = (void*)F_GetAdapterDisplayMode;
    vt[VT_CheckDeviceType] = (void*)F_CheckDeviceType;
    vt[VT_CheckDeviceFormat] = (void*)F_CheckDeviceFormat;
    vt[VT_CheckDeviceMultiSampleType] = (void*)F_CheckMultiSample;
    vt[VT_CheckDepthStencilMatch] = (void*)F_CheckDepthStencil;
    vt[VT_GetDeviceCaps] = (void*)F_GetDeviceCaps;
    vt[VT_GetAdapterMonitor] = (void*)F_GetAdapterMonitor;
    vt[VT_CreateDevice] = (void*)F_CreateDevice;
    DWORD ignored = 0;
    VirtualProtect(vt, sizeof(g_vtableOrig), oldProtect, &ignored);
    g_vtPatched = true;
}

void* WINAPI Fake_Direct3DCreate8(UINT sdkVersion) {
    if (!EnsureRealFactory())
        return NULL;
    void* d3d = g_realCreate8(sdkVersion);
    if (d3d && !g_vtPatched && InterlockedCompareExchange(&g_wrapLock, 1, 0) == 0) {
        if (!g_vtPatched && g_vtableOrig[VT_GetAdapterCount] == NULL) {
            // Snapshot the pristine vtable before patching.
            void** vt = *(void***)d3d;
            memcpy(g_vtableOrig, vt, sizeof(g_vtableOrig));
            ChooseAdapter(d3d);
            PatchVTable(d3d);
        }
        InterlockedExchange(&g_wrapLock, 0);
    }
    return d3d;
}

decltype(&GetProcAddress) _GetProcAddress = NULL;

FARPROC WINAPI Fake_GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    if (lpProcName && ((ULONG_PTR)lpProcName >> 16) != 0 && g_realCreate8) {
        if (strcmp(lpProcName, "Direct3DCreate8") == 0)
            return (FARPROC)(void*)Fake_Direct3DCreate8;
    }
    return _GetProcAddress(hModule, lpProcName);
}

} // namespace

void PlaceWindowOnBestMonitor(int& x, int& y, int nWidth, int nHeight) {
    x = (GetSystemMetrics(SM_CXSCREEN) - nWidth) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - nHeight) / 4;
    if (nWidth <= 0 || nHeight <= 0)
        return;
    POINT center;
    center.x = x + nWidth / 2;
    center.y = y + nHeight / 2;
    HMONITOR hDef = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    if (hDef && GetMonitorInfo(hDef, &mi)) {
        int w = mi.rcWork.right - mi.rcWork.left;
        int h = mi.rcWork.bottom - mi.rcWork.top;
        // Portrait-first: if the historical placement fits, keep it even
        // when the window/monitor orientations differ. The D3D device
        // stays on the same adapter, so window + device never split.
        if (w >= nWidth && h >= nHeight)
            return;
    }
    RECT work;
    if (!PickMonitorFor(nWidth, nHeight, work))
        return;
    int w = work.right - work.left;
    int h = work.bottom - work.top;
    x = work.left + (w - nWidth) / 2;
    y = work.top + (h - nHeight) / 4;
    if (x < work.left)
        x = work.left;
    if (y < work.top)
        y = work.top;
    if (x + nWidth > work.right)
        x = work.right - nWidth;
    if (y + nHeight > work.bottom)
        y = work.bottom - nHeight;
}

bool HookD3D8AdapterRemap(bool bEnable) {
    if (!_GetProcAddress) {
        _GetProcAddress = (decltype(_GetProcAddress))(void*)GetFuncAddress("KERNEL32", "GetProcAddress");
        if (!_GetProcAddress)
            return false;
    }
    if (bEnable)
        EnsureRealFactory(); // preload outside the detour; factory retries lazily.
    return Memory::SetHook(bEnable, (void**)&_GetProcAddress, (void*)Fake_GetProcAddress);
}

} // namespace D3D8PortraitFix
