#pragma once

// Portrait-primary monitor fix for Gr2D_DX8 (Direct3D 8) initialization.
//
// Symptom: with a portrait monitor as the Windows primary display,
// BeiDou.exe aborts during startup with a "MapleStory / error code :
// -2147467259" (E_FAIL) dialog. Gr2D_DX8 only enumerates display modes
// on D3D adapter 0 (== primary monitor); a portrait adapter reports
// portrait modes only, so the landscape game resolution is never found
// ("Failed in finding proper screen mode for Gr2D") and
// CWvsApp::InitializeGr2D fails.
//
// Fix, applied from ijl15.dll before the client initializes Direct3D
// (portrait-first: windowed D3D8 device creation works fine on a
// portrait adapter, so the game stays on the primary monitor):
//  1. GetProcAddress is hooked: resolving "Direct3DCreate8" returns our
//     factory instead of the d3d8.dll one.
//  2. The factory creates the real IDirect3D8 object and patches its
//     vtable. EnumAdapterModes additionally reports the game resolution
//     (plus common sizes, at 60 Hz and the display rate) whenever it is
//     missing from the real list (portrait adapters; sparse virtual ones).
//  3. If CreateDevice on adapter 0 ever fails, it is retried once on a
//     landscape adapter when one exists.
//  4. CreateWindowExA placement keeps the historical coordinates
//     whenever they fit, so the window stays where it always was.
//
namespace D3D8PortraitFix {
// Center (x, y, nWidth, nHeight) for a window of the requested size.
// Keeps the historical primary-monitor formula whenever it fits.
void PlaceWindowOnBestMonitor(int& x, int& y, int nWidth, int nHeight);

// Install/remove the GetProcAddress hook that redirects Direct3DCreate8.
bool HookD3D8AdapterRemap(bool bEnable);

} // namespace D3D8PortraitFix
