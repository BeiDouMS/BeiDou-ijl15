#include "stdafx.h"
#include "LocaleFix.h"
#include "detours.h"

namespace {

const UINT kCodePageGBK = 936;
const BYTE kCharsetGB2312 = 134;   // GB2312_CHARSET
const BYTE kCharsetDefault = 1;    // DEFAULT_CHARSET

CPINFO g_gbkInfo;

typedef int (WINAPI* MultiByteToWideChar_t)(UINT, DWORD, LPCCH, int, LPWSTR, int);
typedef int (WINAPI* WideCharToMultiByte_t)(UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
typedef UINT(WINAPI* GetACP_t)(void);
typedef UINT(WINAPI* GetOEMCP_t)(void);
typedef BOOL(WINAPI* GetCPInfo_t)(UINT, LPCPINFO);
typedef BOOL(WINAPI* IsDBCSLeadByte_t)(BYTE);
typedef BOOL(WINAPI* IsDBCSLeadByteEx_t)(UINT, BYTE);
typedef HFONT(WINAPI* CreateFontIndirectA_t)(const LOGFONTA*);
typedef HFONT(WINAPI* CreateFontIndirectW_t)(const LOGFONTW*);

MultiByteToWideChar_t oMultiByteToWideChar = nullptr;
WideCharToMultiByte_t oWideCharToMultiByte = nullptr;
GetACP_t oGetACP = nullptr;
GetOEMCP_t oGetOEMCP = nullptr;
GetCPInfo_t oGetCPInfo = nullptr;
IsDBCSLeadByte_t oIsDBCSLeadByte = nullptr;
IsDBCSLeadByteEx_t oIsDBCSLeadByteEx = nullptr;
CreateFontIndirectA_t oCreateFontIndirectA = nullptr;
CreateFontIndirectW_t oCreateFontIndirectW = nullptr;

// 只接管"跟随系统"的伪代码页，显式指定代码页的调用原样放行
UINT MapCodePage(UINT codePage)
{
	if (codePage == CP_ACP || codePage == CP_OEMCP || codePage == CP_THREAD_ACP) {
		return kCodePageGBK;
	}
	return codePage;
}

int WINAPI HkMultiByteToWideChar(UINT codePage, DWORD flags, LPCCH src, int srcLen, LPWSTR dst, int dstLen)
{
	const UINT mapped = MapCodePage(codePage);
	const int result = oMultiByteToWideChar(mapped, flags, src, srcLen, dst, dstLen);
	// 不同代码页支持的 flags 不一样，在目标代码页下不合法就退回原样调用
	if (result == 0 && mapped != codePage && GetLastError() == ERROR_INVALID_FLAGS) {
		return oMultiByteToWideChar(codePage, flags, src, srcLen, dst, dstLen);
	}
	return result;
}

int WINAPI HkWideCharToMultiByte(UINT codePage, DWORD flags, LPCWCH src, int srcLen,
	LPSTR dst, int dstLen, LPCCH defaultChar, LPBOOL usedDefaultChar)
{
	const UINT mapped = MapCodePage(codePage);
	const int result = oWideCharToMultiByte(mapped, flags, src, srcLen, dst, dstLen, defaultChar, usedDefaultChar);
	if (result == 0 && mapped != codePage && GetLastError() == ERROR_INVALID_FLAGS) {
		return oWideCharToMultiByte(codePage, flags, src, srcLen, dst, dstLen, defaultChar, usedDefaultChar);
	}
	return result;
}

UINT WINAPI HkGetACP(void)
{
	return kCodePageGBK;
}

UINT WINAPI HkGetOEMCP(void)
{
	return kCodePageGBK;
}

BOOL WINAPI HkGetCPInfo(UINT codePage, LPCPINFO info)
{
	return oGetCPInfo(MapCodePage(codePage), info);
}

// 按 GBK 的前导字节区间判断（0x81-0xFE）。ACP 不对时原版一律返回 FALSE，
// 汉字会被当成两个独立字符，折行和光标位置全乱
BOOL WINAPI HkIsDBCSLeadByte(BYTE testChar)
{
	for (int i = 0; i + 1 < MAX_LEADBYTES && g_gbkInfo.LeadByte[i] != 0; i += 2) {
		if (testChar >= g_gbkInfo.LeadByte[i] && testChar <= g_gbkInfo.LeadByte[i + 1]) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL WINAPI HkIsDBCSLeadByteEx(UINT codePage, BYTE testChar)
{
	return oIsDBCSLeadByteEx(MapCodePage(codePage), testChar);
}

// 只改 DEFAULT_CHARSET —— 它本来就是"由系统区域决定"，替换成 GB2312 等于
// 还原中文系统上的行为。显式指定了 charset 的调用不动。
HFONT WINAPI HkCreateFontIndirectA(const LOGFONTA* logFont)
{
	if (logFont != nullptr && logFont->lfCharSet == kCharsetDefault) {
		LOGFONTA patched = *logFont;
		patched.lfCharSet = kCharsetGB2312;
		return oCreateFontIndirectA(&patched);
	}
	return oCreateFontIndirectA(logFont);
}

HFONT WINAPI HkCreateFontIndirectW(const LOGFONTW* logFont)
{
	if (logFont != nullptr && logFont->lfCharSet == kCharsetDefault) {
		LOGFONTW patched = *logFont;
		patched.lfCharSet = kCharsetGB2312;
		return oCreateFontIndirectW(&patched);
	}
	return oCreateFontIndirectW(logFont);
}

void Attach(void** target, void* detour)
{
	if (*target != nullptr && !Memory::SetHook(true, target, detour)) {
		*target = nullptr;
	}
}

}   // namespace

void LocaleFix_Install(bool forceFontCharset)
{
	// 系统本来就是 GBK 就什么都不做
	if (GetACP() == kCodePageGBK) {
		return;
	}

	ZeroMemory(&g_gbkInfo, sizeof(g_gbkInfo));
	if (!GetCPInfo(kCodePageGBK, &g_gbkInfo)) {
		return;   // 这台机器没有 936 代码页，帮不上忙
	}

	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
	if (kernel32 != nullptr) {
		oMultiByteToWideChar = (MultiByteToWideChar_t)GetProcAddress(kernel32, "MultiByteToWideChar");
		oWideCharToMultiByte = (WideCharToMultiByte_t)GetProcAddress(kernel32, "WideCharToMultiByte");
		oGetACP = (GetACP_t)GetProcAddress(kernel32, "GetACP");
		oGetOEMCP = (GetOEMCP_t)GetProcAddress(kernel32, "GetOEMCP");
		oGetCPInfo = (GetCPInfo_t)GetProcAddress(kernel32, "GetCPInfo");
		oIsDBCSLeadByte = (IsDBCSLeadByte_t)GetProcAddress(kernel32, "IsDBCSLeadByte");
		oIsDBCSLeadByteEx = (IsDBCSLeadByteEx_t)GetProcAddress(kernel32, "IsDBCSLeadByteEx");

		Attach((void**)&oMultiByteToWideChar, (void*)HkMultiByteToWideChar);
		Attach((void**)&oWideCharToMultiByte, (void*)HkWideCharToMultiByte);
		Attach((void**)&oGetACP, (void*)HkGetACP);
		Attach((void**)&oGetOEMCP, (void*)HkGetOEMCP);
		Attach((void**)&oGetCPInfo, (void*)HkGetCPInfo);
		Attach((void**)&oIsDBCSLeadByte, (void*)HkIsDBCSLeadByte);
		Attach((void**)&oIsDBCSLeadByteEx, (void*)HkIsDBCSLeadByteEx);
	}

	if (!forceFontCharset) {
		return;
	}

	const HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
	if (gdi32 != nullptr) {
		oCreateFontIndirectA = (CreateFontIndirectA_t)GetProcAddress(gdi32, "CreateFontIndirectA");
		oCreateFontIndirectW = (CreateFontIndirectW_t)GetProcAddress(gdi32, "CreateFontIndirectW");
		Attach((void**)&oCreateFontIndirectA, (void*)HkCreateFontIndirectA);
		Attach((void**)&oCreateFontIndirectW, (void*)HkCreateFontIndirectW);
	}
}
