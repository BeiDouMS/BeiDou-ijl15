#pragma once
#include <imm.h>
#pragma comment(lib, "imm32.lib")

void EnableIme() { // 目前没有用到这个方法，未测试效果
	HWND hwnd = GetForegroundWindow(); // 获取当前前台窗口的句柄
	if (hwnd) {
		// 获取输入法上下文
		HIMC hImc = ImmGetContext(hwnd);
		if (hImc) {
			// 将输入法上下文重新关联到窗口
			ImmAssociateContext(hwnd, hImc);
			ImmReleaseContext(hwnd, hImc);
		}
	}
}

void DisableIme() {
	HWND hwnd = GetForegroundWindow(); // 获取当前前台窗口的句柄
	if (hwnd) {
		// 获取输入法上下文
		HIMC hImc = ImmGetContext(hwnd);
		if (hImc) {
			// 解除输入法上下文的关联
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
	// 这里原函数会直接跳过切换IME的地方，我们要让他跳到切换IME的地方
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
		cmp[esi + 0x80], 1 // 判断是否密码框
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
		// 适合较旧的win10系统
		// 已知问题：商城的礼物赠送，填写内容的地方会无法调出IME

		GeneralHook();
		// 单行输入框OnSetFocus@CCtrlEdit
		Memory::CodeCave(setOnFocusFirstJudgement, 0x004CA05B, 6);
		Memory::CodeCave(switchIme, 0x004CA089, 6);
		// 多行输入框OnSetFocus@CCtrlMLEdit
		Memory::FillBytes(0x004D32C6, 0x90, 2);
		Memory::CodeCave(switchMLIme, 0x004D32D9, 7);
		Memory::CodeCave(destroyWindow, 0x004DFEA4, 9); // 销毁窗口时固定禁用IME
		std::cout << "Old Ime Hook" << std::endl;
	}

	static void HookNew() {
		// 适合较新的win10系统和win11系统
		// 由于原来的方法在 win11下失效因此重写了

		GeneralHook();
		// 单行输入框启用IME
		Memory::CodeCave(newSwitchIme, 0x004CA089, 6);
		Memory::CodeCave(destroyWindow, 0x004DFEA4, 9); // 销毁窗口时固定禁用IME
		//Memory::WriteByte(0x004D32D9 + 1, 1); // 多行输入
		Memory::CodeCave(newSwitchMLIme, 0x004D32D9, 7); // 多行输入
		std::cout << "New Ime Hook" << std::endl;
		// 测试
		// 登录界面 密码禁止调用IME————账号框无法识别，密码框已特殊处理禁用IME了
		// 平常状态下 输入法可以输入中文，非输入法不卡门————已测试
		// 商城礼物 日期/标题/内容————生日禁用IME / 标题内容均可调用输入法
		// 老虎喇叭 多行输入框————可正常输入
	}
private:
	static void GeneralHook() {
		Memory::FillBytes(0x008D54A6, 0x90, 9); // Key ?
		Memory::FillBytes(0x00937225, 0x90, 9); // Chat
		Memory::FillBytes(0x00531EE8, 0x90, 9); // Group Message
		// 剪贴板支持中文
		Memory::FillBytes(0x004CAE7D, 0x90, 2);
		Memory::WriteByte(0x004CAE8F, 0xEB);
		// 角色名中文检测
		Memory::FillBytes(0x007A015D, 0x90, 2);
	}
};