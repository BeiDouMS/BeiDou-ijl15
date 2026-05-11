#include "stdafx.h"
#include "ModifierRepeat.h"
#include "INIReader.h"
#include <array>
#include <atomic>
#include <utility>

namespace {
	struct RepeatConfig {
		bool enabled = true;
		DWORD initialDelayMs = 500;
		DWORD intervalMs = 40;
	};

	struct ModifierState {
		bool isDown = false;
		ULONGLONG firstDownAt = 0;
		ULONGLONG lastRepeatAt = 0;
	};

	std::atomic<bool> g_initialized(false);
	std::atomic<bool> g_running(false);
	HANDLE g_thread = nullptr;

	RepeatConfig LoadConfig() {
		RepeatConfig cfg;
		INIReader reader("config.ini");
		if (reader.ParseError() != 0) {
			return cfg;
		}

		cfg.enabled = reader.GetBoolean("general", "FixModifierHold", true);

		UINT kbDelay = 1;
		UINT kbSpeed = 20;
		SystemParametersInfoA(SPI_GETKEYBOARDDELAY, 0, &kbDelay, 0);
		SystemParametersInfoA(SPI_GETKEYBOARDSPEED, 0, &kbSpeed, 0);

		if (kbDelay > 3) kbDelay = 3;
		if (kbSpeed > 31) kbSpeed = 31;

		cfg.initialDelayMs = (kbDelay + 1) * 250;

		// Windows keyboard speed 0..31 maps to roughly 2.5..30 repeats/sec.
		const double repeatsPerSecond = 2.5 + (static_cast<double>(kbSpeed) * (27.5 / 31.0));
		DWORD interval = static_cast<DWORD>(1000.0 / repeatsPerSecond);
		if (interval < 20) interval = 20;
		if (interval > 400) interval = 400;
		cfg.intervalMs = interval;

		return cfg;
	}

	BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid != GetCurrentProcessId()) {
			return TRUE;
		}
		if (!IsWindowVisible(hwnd)) {
			return TRUE;
		}
		if (GetWindow(hwnd, GW_OWNER) != nullptr) {
			return TRUE;
		}
		*reinterpret_cast<HWND*>(lParam) = hwnd;
		return FALSE;
	}

	HWND FindMainWindow() {
		HWND hwnd = nullptr;
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&hwnd));
		return hwnd;
	}

	LPARAM BuildRepeatLParam(int vkCode) {
		const UINT scanCode = MapVirtualKeyA(static_cast<UINT>(vkCode), MAPVK_VK_TO_VSC);
		LPARAM lParam = 1 | (static_cast<LPARAM>(scanCode) << 16);
		if (vkCode == VK_MENU) {
			lParam |= (1LL << 29);
		}
		// Repeat keydown: previous key state = 1, transition state = 0.
		lParam |= (1LL << 30);
		return lParam;
	}

	void PostRepeatKeyDown(HWND hwnd, int vkCode) {
		const UINT message = (vkCode == VK_MENU) ? WM_SYSKEYDOWN : WM_KEYDOWN;
		PostMessageA(hwnd, message, static_cast<WPARAM>(vkCode), BuildRepeatLParam(vkCode));
	}

	DWORD WINAPI RepeatThreadProc(LPVOID) {
		ModifierState altState{};
		ModifierState ctrlState{};
		ModifierState shiftState{};
		HWND hwnd = nullptr;
		RepeatConfig cfg = LoadConfig();
		ULONGLONG lastConfigReloadAt = 0;

		while (g_running.load()) {
			const ULONGLONG now = GetTickCount64();
			if (now - lastConfigReloadAt >= 1000) {
				cfg = LoadConfig();
				lastConfigReloadAt = now;
			}

			if (!cfg.enabled) {
				altState = ModifierState{};
				ctrlState = ModifierState{};
				shiftState = ModifierState{};
				Sleep(10);
				continue;
			}

			if (hwnd == nullptr || !IsWindow(hwnd)) {
				hwnd = FindMainWindow();
				Sleep(10);
				continue;
			}

			if (GetForegroundWindow() != hwnd) {
				altState = ModifierState{};
				ctrlState = ModifierState{};
				shiftState = ModifierState{};
				Sleep(5);
				continue;
			}

			const std::array<std::pair<int, ModifierState*>, 3> keys = {
				std::make_pair(VK_MENU, &altState),
				std::make_pair(VK_CONTROL, &ctrlState),
				std::make_pair(VK_SHIFT, &shiftState),
			};

			for (const auto& key : keys) {
				const int vkCode = key.first;
				ModifierState& state = *key.second;
				const bool isDown = (GetAsyncKeyState(vkCode) & 0x8000) != 0;

				if (!isDown) {
					state = ModifierState{};
					continue;
				}

				if (!state.isDown) {
					state.isDown = true;
					state.firstDownAt = now;
					state.lastRepeatAt = now;
					continue;
				}

				if (now - state.firstDownAt < cfg.initialDelayMs) {
					continue;
				}
				if (now - state.lastRepeatAt < cfg.intervalMs) {
					continue;
				}

				PostRepeatKeyDown(hwnd, vkCode);
				state.lastRepeatAt = now;
			}

			Sleep(1);
		}

		return 0;
	}
}

void ModifierRepeat::Init() {
	if (g_initialized.exchange(true)) {
		return;
	}

	g_running.store(true);
	g_thread = CreateThread(nullptr, 0, RepeatThreadProc, nullptr, 0, nullptr);
}
