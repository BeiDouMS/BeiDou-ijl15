#pragma once
#include <imm.h>
#pragma comment(lib, "imm32.lib")

void EnableIme() { // Currently unused, not yet effective
	HWND hwnd = GetForegroundWindow(); // Get current foreground window handle
	if (hwnd) {
		// Get IME handle
		HIMC hImc = ImmGetContext(hwnd);
		if (hImc) {
			// Reload IME engine features
			ImmAssociateContext(hwnd, hImc);
			ImmReleaseContext(hwnd, hImc);
		}
	}
}

void DisableIme() {
	HWND hwnd = GetForegroundWindow(); // Get current foreground window handle
	if (hwnd) {
		// Get IME handle
		HIMC hImc = ImmGetContext(hwnd);
		if (hImc) {
			// Close IME engine
			ImmAssociateContext(hwnd, NULL);
			ImmReleaseContext(hwnd, hImc);
		}
	}
}

BYTE enabled = 1;

DWORD funcEnableImeAddr = 0x009E85F3;

DWORD setOnFocusFirstJudgementRtnAddr = 0x004CA061;
DWORD switchImeAddr = 0x004CA078;
__declspec(naked) void setOnFocusFirstJudgement() {
	// Jump to original return point, redirect to IME switch location
	__asm {
		cmp[esp + 0Ch], edi
		jz label_jmp_switch_ime
		jmp setOnFocusFirstJudgementRtnAddr

		label_jmp_switch_ime :
		jmp switchImeAddr
	}
}

DWORD enableRtnAddr = 0x004CA08F;
DWORD disableRtnAddr = 0x004CA091;
__declspec(naked) void switchIme() {
	__asm {
		cmp [esp + 0Ch], edi
		jz  label_jz
		xor eax, eax
		cmp [esi + 0x80], eax
		setz al
		push eax
		call funcEnableImeAddr
		mov enabled, 1
		jmp  enableRtnAddr

		label_jz :
		push 0
		call funcEnableImeAddr
		jmp  disableRtnAddr
	}
}

DWORD enableMLRtnAddr = 0x004D32E0;
DWORD disableMLRtnAddr = 0x004D32E2;
__declspec(naked) void switchMLIme() {
	__asm {
		cmp  dword ptr[esp + 8], 0
		jz   label_jz
		push 1
		call funcEnableImeAddr
		mov enabled, 1
		jmp  enableMLRtnAddr

		label_jz :
		push 0
		call funcEnableImeAddr
		jmp  disableMLRtnAddr
	}
}

DWORD newSwitchImeRtnAddr = 0x004CA08F;
__declspec(naked) void newSwitchIme() {
	__asm {
		cmp[esi + 0x80], 1 // Check if disabled
		jz label_disable
		push 1
		call funcEnableImeAddr
		mov enabled, 1
		jmp newSwitchImeRtnAddr

		label_disable :
		call DisableIme
		jmp newSwitchImeRtnAddr
	}
}

DWORD destroyWindowRtnAddr = 0x004DFEAD;
DWORD destroyWindowFuncAddr = 0x0041FE69;
__declspec(naked) void destroyWindow() {
	__asm {
		call destroyWindowFuncAddr
		or dword ptr[esi + 14h], 0FFFFFFFFh

		cmp enabled, 0
		jz label_return

		call DisableIme
		mov enabled, 0

		label_return :
		jmp destroyWindowRtnAddr
	}
}

DWORD newSwitchMLImeRtnAddr = 0x004D32EE;
__declspec(naked) void newSwitchMLIme() {
	__asm {
		push 1
		call funcEnableImeAddr
		mov enabled, 1
		jmp  newSwitchMLImeRtnAddr
	}
}


class FixIme {
public:
	static void HookOld() {
		// For older Win10 systems
		// Known issue: Mall chat box and data entry areas don't work with IME

		GeneralHook();
		// Chat input OnSetFocus@CCtrlEdit
		Memory::CodeCave(setOnFocusFirstJudgement, 0x004CA05B, 6);
		Memory::CodeCave(switchIme, 0x004CA089, 6);
		// Multi-line input OnSetFocus@CCtrlMLEdit
		Memory::FillBytes(0x004D32C6, 0x90, 2);
		Memory::CodeCave(switchMLIme, 0x004D32D9, 7);
		Memory::CodeCave(destroyWindow, 0x004DFEA4, 9); // Fix IME when closing window
		std::cout << "Old Ime Hook" << std::endl;
	}

	static void HookNew() {
		// For newer Win10 and Win11 systems
		// Modified original method, Win11 would break, so rewritten

		GeneralHook();
		// Chat input IME switch
		Memory::CodeCave(newSwitchIme, 0x004CA089, 6);
		Memory::CodeCave(destroyWindow, 0x004DFEA4, 9); // Fix IME when closing window
		//Memory::WriteByte(0x004D32D9 + 1, 1); // Multi-line input
		Memory::CodeCave(newSwitchMLIme, 0x004D32D9, 7); // Multi-line input
		std::cout << "New Ime Hook" << std::endl;
		// Notes
		// Chat history: IME works during input but fails after logout, handled here
		// Balanced state: IME off mode vs on mode, system no longer...
		// Mall chat input/output/data. Mall battle IME / old IME for data entry
		// Leaderboard input interface...
	}
	static void GeneralHook() {
		Memory::FillBytes(0x008D54A6, 0x90, 9); // Key (unconditional - char filter breaks IME)
		Memory::FillBytes(0x00937225, 0x90, 9); // Chat
		Memory::FillBytes(0x00531EE8, 0x90, 9); // Group Message
		// Keyboard not yet supported
		Memory::FillBytes(0x004CAE7D, 0x90, 2);
		Memory::WriteByte(0x004CAE8F, 0xEB);
		// Color dye filtering
		Memory::FillBytes(0x007A015D, 0x90, 2);
	}
};
