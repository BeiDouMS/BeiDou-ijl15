#include "stdafx.h"
#include "ModeTableFix.h"
#include "detours.h"

namespace {

// d3d8 的最小 ABI 定义，不依赖 DirectX SDK 的 d3d8.h
struct D3DDisplayMode
{
	UINT  Width;
	UINT  Height;
	UINT  RefreshRate;
	DWORD Format;
};

// IDirect3D8 vtable 槽位
const int kSlotGetAdapterModeCount = 6;
const int kSlotEnumAdapterModes = 7;
const int kSlotGetAdapterDisplayMode = 8;

const DWORD kFormatX8R8G8B8 = 22;
const DWORD kFormatR5G6B5 = 23;
const int kMaxInjectedModes = 4;

typedef void* (WINAPI* Direct3DCreate8_t)(UINT);
typedef UINT(__stdcall* GetAdapterModeCount_t)(void*, UINT);
typedef HRESULT(__stdcall* EnumAdapterModes_t)(void*, UINT, UINT, D3DDisplayMode*);
typedef HRESULT(__stdcall* GetAdapterDisplayMode_t)(void*, UINT, D3DDisplayMode*);
typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR);
typedef HMODULE(WINAPI* LoadLibraryW_t)(LPCWSTR);
typedef HMODULE(WINAPI* LoadLibraryExA_t)(LPCSTR, HANDLE, DWORD);
typedef HMODULE(WINAPI* LoadLibraryExW_t)(LPCWSTR, HANDLE, DWORD);

Direct3DCreate8_t oDirect3DCreate8 = nullptr;
GetAdapterModeCount_t oGetAdapterModeCount = nullptr;
EnumAdapterModes_t oEnumAdapterModes = nullptr;
LoadLibraryA_t oLoadLibraryA = nullptr;
LoadLibraryW_t oLoadLibraryW = nullptr;
LoadLibraryExA_t oLoadLibraryExA = nullptr;
LoadLibraryExW_t oLoadLibraryExW = nullptr;

bool g_bound = false;
bool g_vtableHooked = false;
bool g_injectPrepared = false;
int g_injectedCount = 0;
D3DDisplayMode g_injectedModes[kMaxInjectedModes];
DWORD g_desktopFormat = kFormatX8R8G8B8;
UINT g_desktopRefresh = 60;

// COM 方法直接改 vtable 槽位：比改函数体稳（不受反汇编器影响），
// 而且 d3d8 在 Wine 下是内建实现，代码补丁不一定吃得住
bool PatchSlot(void** vtable, int index, void* hook, void** original)
{
	if (vtable == nullptr) {
		return false;
	}

	DWORD oldProtect = 0;
	if (VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect) == 0) {
		return false;
	}

	*original = vtable[index];
	vtable[index] = hook;

	DWORD restored = 0;
	VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &restored);
	return true;
}

// 目标分辨率已经在真实模式表里就什么都不补，能正常启动的机器完全不受影响
void PrepareInjectedModes(void* self, UINT adapter)
{
	if (g_injectPrepared) {
		return;
	}
	g_injectPrepared = true;
	if (oGetAdapterModeCount == nullptr || oEnumAdapterModes == nullptr) {
		return;
	}

	const UINT targetWidth = (UINT)Client::m_nGameWidth;
	const UINT targetHeight = (UINT)Client::m_nGameHeight;

	const UINT realCount = oGetAdapterModeCount(self, adapter);
	for (UINT i = 0; i < realCount; ++i) {
		D3DDisplayMode mode;
		ZeroMemory(&mode, sizeof(mode));
		if (FAILED(oEnumAdapterModes(self, adapter, i, &mode))) {
			continue;
		}
		if (mode.Width == targetWidth && mode.Height == targetHeight) {
			return;   // 表里有，不用补
		}
	}

	// 客户端到底拿什么条件去匹配不好说，所以常见组合都摆上（去重）
	const DWORD formats[2] = { g_desktopFormat, kFormatR5G6B5 };
	const UINT rates[2] = { g_desktopRefresh, 60 };
	for (int f = 0; f < 2; ++f) {
		for (int r = 0; r < 2; ++r) {
			bool duplicate = false;
			for (int i = 0; i < g_injectedCount; ++i) {
				if (g_injectedModes[i].Format == formats[f] && g_injectedModes[i].RefreshRate == rates[r]) {
					duplicate = true;
				}
			}
			if (duplicate || g_injectedCount >= kMaxInjectedModes) {
				continue;
			}

			D3DDisplayMode& mode = g_injectedModes[g_injectedCount++];
			mode.Width = targetWidth;
			mode.Height = targetHeight;
			mode.RefreshRate = rates[r];
			mode.Format = formats[f];
		}
	}
}

UINT __stdcall HkGetAdapterModeCount(void* self, UINT adapter)
{
	const UINT count = oGetAdapterModeCount(self, adapter);
	PrepareInjectedModes(self, adapter);
	return count + (UINT)g_injectedCount;
}

HRESULT __stdcall HkEnumAdapterModes(void* self, UINT adapter, UINT mode, D3DDisplayMode* displayMode)
{
	PrepareInjectedModes(self, adapter);

	if (g_injectedCount > 0 && displayMode != nullptr) {
		const UINT realCount = oGetAdapterModeCount(self, adapter);
		if (mode >= realCount && (mode - realCount) < (UINT)g_injectedCount) {
			*displayMode = g_injectedModes[mode - realCount];
			return S_OK;
		}
	}
	return oEnumAdapterModes(self, adapter, mode, displayMode);
}

void* WINAPI HkDirect3DCreate8(UINT sdkVersion)
{
	void* d3d8 = oDirect3DCreate8(sdkVersion);
	if (d3d8 == nullptr || g_vtableHooked) {
		return d3d8;
	}
	g_vtableHooked = true;

	void** vtable = *(void***)d3d8;
	PatchSlot(vtable, kSlotGetAdapterModeCount, (void*)HkGetAdapterModeCount, (void**)&oGetAdapterModeCount);
	PatchSlot(vtable, kSlotEnumAdapterModes, (void*)HkEnumAdapterModes, (void**)&oEnumAdapterModes);

	// 自己问一次桌面模式，保证补进去的像素格式/刷新率是这台机器真实的值
	GetAdapterDisplayMode_t getDisplayMode = (GetAdapterDisplayMode_t)vtable[kSlotGetAdapterDisplayMode];
	D3DDisplayMode desktop;
	ZeroMemory(&desktop, sizeof(desktop));
	if (SUCCEEDED(getDisplayMode(d3d8, 0, &desktop))) {
		if (desktop.Format != 0) {
			g_desktopFormat = desktop.Format;
		}
		if (desktop.RefreshRate != 0) {
			g_desktopRefresh = desktop.RefreshRate;
		}
	}
	return d3d8;
}

void BindD3D8()
{
	if (g_bound) {
		return;
	}
	const HMODULE module = GetModuleHandleA("d3d8.dll");
	if (module == nullptr) {
		return;
	}
	g_bound = true;

	oDirect3DCreate8 = (Direct3DCreate8_t)GetProcAddress(module, "Direct3DCreate8");
	if (oDirect3DCreate8 != nullptr) {
		if (!Memory::SetHook(true, (void**)&oDirect3DCreate8, (void*)HkDirect3DCreate8)) {
			oDirect3DCreate8 = nullptr;
		}
	}
}

// d3d8.dll 在我们 DllMain 时通常还没加载，而在 DllMain 里 LoadLibrary 会踩 loader lock，
// 所以钩住 LoadLibrary 系列，等客户端自己加载 d3d8 时再绑
HMODULE WINAPI HkLoadLibraryA(LPCSTR fileName)
{
	const HMODULE result = oLoadLibraryA(fileName);
	if (result != nullptr && !g_bound) {
		BindD3D8();
	}
	return result;
}

HMODULE WINAPI HkLoadLibraryW(LPCWSTR fileName)
{
	const HMODULE result = oLoadLibraryW(fileName);
	if (result != nullptr && !g_bound) {
		BindD3D8();
	}
	return result;
}

HMODULE WINAPI HkLoadLibraryExA(LPCSTR fileName, HANDLE file, DWORD flags)
{
	const HMODULE result = oLoadLibraryExA(fileName, file, flags);
	if (result != nullptr && !g_bound) {
		BindD3D8();
	}
	return result;
}

HMODULE WINAPI HkLoadLibraryExW(LPCWSTR fileName, HANDLE file, DWORD flags)
{
	const HMODULE result = oLoadLibraryExW(fileName, file, flags);
	if (result != nullptr && !g_bound) {
		BindD3D8();
	}
	return result;
}

void InstallLoaderHooks()
{
	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	if (kernel32 == nullptr) {
		return;
	}

	oLoadLibraryA = (LoadLibraryA_t)GetProcAddress(kernel32, "LoadLibraryA");
	oLoadLibraryW = (LoadLibraryW_t)GetProcAddress(kernel32, "LoadLibraryW");
	oLoadLibraryExA = (LoadLibraryExA_t)GetProcAddress(kernel32, "LoadLibraryExA");
	oLoadLibraryExW = (LoadLibraryExW_t)GetProcAddress(kernel32, "LoadLibraryExW");

	if (oLoadLibraryA != nullptr) {
		Memory::SetHook(true, (void**)&oLoadLibraryA, (void*)HkLoadLibraryA);
	}
	if (oLoadLibraryW != nullptr) {
		Memory::SetHook(true, (void**)&oLoadLibraryW, (void*)HkLoadLibraryW);
	}
	if (oLoadLibraryExA != nullptr) {
		Memory::SetHook(true, (void**)&oLoadLibraryExA, (void*)HkLoadLibraryExA);
	}
	if (oLoadLibraryExW != nullptr) {
		Memory::SetHook(true, (void**)&oLoadLibraryExW, (void*)HkLoadLibraryExW);
	}
}

}   // namespace

void ModeTableFix_Install(bool enable)
{
	if (!enable) {
		return;
	}

	BindD3D8();
	if (!g_bound) {
		InstallLoaderHooks();
	}
}
