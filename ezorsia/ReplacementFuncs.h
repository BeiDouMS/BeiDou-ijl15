#pragma once
#include "AutoTypes.h"
//#pragma optimize("", off)
//notes from my knowledge as i have not used these kinds of codes practically well
//function replacement is when you replace the original function in the client with your own fake function, usually to add some extra functionality
//for more complex applications you would also need to define the client's variables and reinterpret_cast those (no void this time)
//you need the right calling convention (match client's original or use _fastcall, i havent tried it much)
//it would help to know the benefits and drawbacks of "reinterpret_cast", as well as how it is hooking to prevent accidents
//hooking to the original function will replace it at all times when it is called by the client
//i personally have not tried it more because it requires a very thorough understanding of how the client code works, re-making the parts here,
//and using it, all together, in a way that doesnt break anything
//it would be the best way to do it for very extensive client edits and if you need to replace entire functions in that context but
//code caving is generally easier for short term, one-time patchwork fixes	//thanks you teto for helping me on this learning journey
bool HookGetModuleFileName_initialized = true;
bool Hook_GetModuleFileNameW(bool bEnable) {
	static decltype(&GetModuleFileNameW) _GetModuleFileNameW = &GetModuleFileNameW;

	const decltype(&GetModuleFileNameW) GetModuleFileNameW_Hook = [](HMODULE hModule, LPWSTR lpFileName, DWORD dwSize) -> DWORD {
		if (HookGetModuleFileName_initialized)
		{
			std::cout << "HookGetModuleFileName started" << std::endl;
			HookGetModuleFileName_initialized = false;
		}
		auto len = _GetModuleFileNameW(hModule, lpFileName, dwSize);
		// Check to see if the length is invalid (zero)
		if (!len) {
			// Try again without the provided module for a fixed result
			len = _GetModuleFileNameW(nullptr, lpFileName, dwSize);
			std::cout << "HookGetModuleFileName null file name" << std::endl;
		}
		return len;
	};

	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_GetModuleFileNameW), GetModuleFileNameW_Hook);
}

/// <summary>
/// Creates a detour for the User32.dll CreateWindowExA function applying the following changes:
/// 1. Enable the window minimize box
/// </summary>
CreateWindowExA_t CreateWindowExA_Original = (CreateWindowExA_t)GetFuncAddress("USER32", "CreateWindowExA");
bool HookCreateWindowExA_initialized = true;
HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	if(HookCreateWindowExA_initialized)//credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
	{
		std::cout << "HookCreateWindowExA started" << std::endl;
		HookCreateWindowExA_initialized = false;
	}
	if(strstr(lpClassName, "MapleStoryClass"))
	{
		dwStyle |= WS_MINIMIZEBOX; // enable minimize button
	}
	else if (strstr(lpClassName, "StartUpDlgClass"))
	{
		return NULL; //kill startup balloon
	}
	//if(Client::WindowedMode)
	//{ //unfortunately doesnt work, reverting to old window mode fix
	//	dwExStyle = 0;
	//}
	return CreateWindowExA_Original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
};
bool Hook_CreateWindowExA(bool bEnable) {
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&CreateWindowExA_Original), CreateWindowExA_Hook);
}
bool CreateMutexA_initialized = true; ////credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
CreateMutexA_t CreateMutexA_Original = (CreateMutexA_t)GetFuncAddress("KERNEL32", "CreateMutexA");
HANDLE WINAPI CreateMutexA_Hook(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR	lpName) {
	if (CreateMutexA_initialized)
		{
			std::cout << "HookCreateMutexA started" << std::endl;
			CreateMutexA_initialized = false;
		}
	if (!CreateMutexA_Original)
	{
		std::cout << "Original CreateMutex pointer corrupted. Failed to return mutex value to calling function." << std::endl;
		return nullptr;
	}
	else if (lpName && strstr(lpName, "WvsClientMtx"))
	{
		// from https://github.com/pokiuwu/AuthHook-v203.4/blob/AuthHook-v203.4/Client176/WinHook.cpp
		char szMutex[128];	//multiclient
		int nPID = GetCurrentProcessId();
		sprintf_s(szMutex, "%s-%d", lpName, nPID);
		lpName = szMutex;
		return CreateMutexA_Original(lpMutexAttributes, bInitialOwner, lpName);
	}
	return CreateMutexA_Original(lpMutexAttributes, bInitialOwner, lpName);
}
bool Hook_CreateMutexA(bool bEnable)	//ty darter	//ty angel!
{
	//static auto _CreateMutexA = decltype(&CreateMutexA)(GetFuncAddress("KERNEL32", "CreateMutexA"));
	//decltype(&CreateMutexA) Hook = [](LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) -> HANDLE
	//{
	//	if (CreateMutexA_initialized)
	//	{
	//		std::cout << "HookCreateMutexA started" << std::endl;
	//		CreateMutexA_initialized = false;
	//	}
	//	//Multi-Client Check Removal
	//	if (lpName && strstr(lpName, "WvsClientMtx"))
	//	{
	//		return (HANDLE)0x0BADF00D;
	//		//char szMutex[128];
	//		//lpName = szMutex;
	//	}
	//	return _CreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
	//};	//original
	//return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_CreateMutexA), Hook);
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&CreateMutexA_Original), CreateMutexA_Hook);
}
FindFirstFileA_t FindFirstFileA_Original = (FindFirstFileA_t)GetFuncAddress("KERNEL32", "FindFirstFileA");
bool FindFirstFileA_initialized = true; //ty joo for advice with this, check out their releases: https://github.com/OpenRustMS
HANDLE WINAPI FindFirstFileA_Hook(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
	if (FindFirstFileA_initialized)
	{
		std::cout << "HookFindFirstFileA started" << std::endl;
	}
	if (!FindFirstFileA_Original)
	{
		std::cout << "Original FindFirstFileA pointer corrupted. Failed to return ??? value to calling function." << std::endl;
		return nullptr;
	}
	else if (lpFileName && strstr(lpFileName, "*") && FindFirstFileA_initialized)
	{
		FindFirstFileA_initialized = false;
		//std::cout << "FindFirstFileA dinput8.dll spoofed" << std::endl;
		return FindFirstFileA_Original("*.wz", lpFindFileData);
	}
	//else if (FindFirstFileA_initialized)
	//{
	//	std::cout << "FindFirstFileA failed... trying again" << std::endl;
	//	Sleep(1); //keep trying to find the file instead of failing
	//	return FindFirstFileA_Hook;
	//}
	//std::cout << "FindFirstFileA failed... unable to try again" << lpFileName << std::endl;
	FindFirstFileA_initialized = false;
	return FindFirstFileA_Original(lpFileName, lpFindFileData);
}
bool Hook_FindFirstFileA(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&FindFirstFileA_Original), FindFirstFileA_Hook);
}
GetLastError_t GetLastError_Original = (GetLastError_t)GetFuncAddress("KERNEL32", "GetLastError");
bool GetLastError_initialized = true;
DWORD WINAPI GetLastError_Hook() {
	if (GetLastError_initialized)
	{
		std::cout << "HookGetLastError started" << std::endl;
		GetLastError_initialized = false;
	}
	DWORD error = GetLastError_Original();
	std::cout << "GetLastError: " << error << std::endl;
	return error;
}
bool Hook_GetLastError(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&GetLastError_Original), GetLastError_Hook);
}
sockaddr_in default_nXXXON_if;
#define WSAAddressToString  WSAAddressToStringA
bool WSPStartup_initialized = true; ////credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
INT WSPAPI WSPConnect_Hook(SOCKET s, const struct sockaddr* name, int namelen, LPWSABUF	lpCallerData, LPWSABUF lpCalleeData, LPQOS lpSQOS, LPQOS lpGQOS, LPINT lpErrno) {
	char szAddr[50];
	DWORD dwLen = 50;
	WSAAddressToString((sockaddr*)name, namelen, NULL, szAddr, &dwLen);

	sockaddr_in* service = (sockaddr_in*)name;
	//hardcoded NXXON IP addies in default client
	if (strstr(szAddr, "63.251.217.2") || strstr(szAddr, "63.251.217.3") || strstr(szAddr, "63.251.217.4"))
	{
		default_nXXXON_if = *service;
		service->sin_addr.S_un.S_addr = inet_addr(MainMain::m_sRedirectIP);
		//service->sin_port = htons(MainMain::porthere);
		MainMain::m_GameSock = s;
	}

	return MainMain::m_ProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}
INT WSPAPI WSPGetPeerName_Hook(SOCKET s, struct sockaddr* name, LPINT namelen, LPINT lpErrno) {
	int nRet = MainMain::m_ProcTable.lpWSPGetPeerName(s, name, namelen, lpErrno);//credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
	if (nRet != SOCKET_ERROR && s == MainMain::m_GameSock)
	{
		sockaddr_in* service = (sockaddr_in*)name; //suspecting this is for checking rather than actually connecting
		service->sin_addr.S_un.S_addr = default_nXXXON_if.sin_addr.S_un.S_addr;//inet_addr(MainMain::m_sRedirectIP)
		//service->sin_port = htons(MainMain::porthere);
	}
	return nRet;
}
INT WSPAPI WSPCloseSocket_Hook(SOCKET s, LPINT lpErrno) {//credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
	int nRet = MainMain::m_ProcTable.lpWSPCloseSocket(s, lpErrno);
	if (s == MainMain::m_GameSock)
	{
		MainMain::m_GameSock = INVALID_SOCKET;
	}
	return nRet;
}
WSPStartup_t WSPStartup_Original = (WSPStartup_t)GetFuncAddress("MSWSOCK", "WSPStartup"); /*in this function we'll call the WSP Functions*/					const wchar_t* v42;
INT WSPAPI WSPStartup_Hook(WORD wVersionRequested, LPWSPDATA lpWSPData, LPWSAPROTOCOL_INFOW lpProtocolInfo, WSPUPCALLTABLE UpcallTable, LPWSPPROC_TABLE	lpProcTable) {
	if (WSPStartup_initialized)//credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate
	{
		std::cout << "HookWSPStartup started" << std::endl;
		WSPStartup_initialized = false;
	}
	int nRet = WSPStartup_Original(wVersionRequested, lpWSPData, lpProtocolInfo, UpcallTable, lpProcTable);

	if (nRet == NO_ERROR)
	{
		MainMain::m_GameSock = INVALID_SOCKET;
		MainMain::m_ProcTable = *lpProcTable;

		lpProcTable->lpWSPConnect = WSPConnect_Hook;
		lpProcTable->lpWSPGetPeerName = WSPGetPeerName_Hook;
		lpProcTable->lpWSPCloseSocket = WSPCloseSocket_Hook;
	}
	else
	{
		std::cout << "WSPStartup Error Code: " + nRet << std::endl;
	}
	return nRet;
}
bool Hook_WSPStartup(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&WSPStartup_Original), WSPStartup_Hook);
}
#define x86CMPEAX 0x3D
bool GetACP_initialized = true; DWORD LocaleSpoofValue = 0;//choose value from https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
GetACP_t GetACP_Original = (GetACP_t)GetFuncAddress("KERNEL32", "GetACP");
UINT WINAPI GetACP_Hook() { // AOB: FF 15 ?? ?? ?? ?? 3D ?? ?? ?? 00 00 74 <- library call inside winmain func
	if (GetACP_initialized){//credits to the creators of https://github.com/MapleStory-Archive/MapleClientEditTemplate	
		std::cout << "HookGetACP started" << std::endl;
		GetACP_initialized = false;	//NOTE: idk what this really does tbh, maybe it is custom locale value but more likely it is to skip a check
	}	//that some clients may have that restricts you based on locale; if it is not a check and instead logged by server feel free to feed bad data by disabling the part below
	UINT uiNewLocale = LocaleSpoofValue;
	if (uiNewLocale == 0) { return GetACP_Original(); } //we do hook in my version!//should not happen cuz we dont hook if value is zero
	DWORD dwRetAddr = reinterpret_cast<DWORD>(_ReturnAddress());
	// return address should be a cmp eax instruction because ret value is stored in eax
	// and nothing else should happen before the cmp
	if(ReadValue<BYTE>(dwRetAddr) == x86CMPEAX) {	//disable this if and else if you wanna always use spoof value (i.e. if server logs it)
			uiNewLocale = ReadValue<DWORD>(dwRetAddr + 1); // check value is always 4 bytes
			std::cout << "[GetACP] Found desired locale: " << uiNewLocale << std::endl; }
	else { std::cout << "[GetACP] Unable to automatically determine locale, using stored locale: " << uiNewLocale << std::endl; }
	std::cout << "[GetACP] Locale spoofed to: " << uiNewLocale << " unhooking. Calling address: " << dwRetAddr << std::endl;
	if (Memory::SetHook(FALSE, reinterpret_cast<void**>(&GetACP_Original), GetACP_Hook)) {
		std::cout << "Failed to unhook GetACP." << std::endl; }
	return uiNewLocale;
}
bool Hook_GetACP(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&GetACP_Original), GetACP_Hook);
}
//bool Hook_get_unknown(bool bEnable)
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_get_unknown), _get_unknown_Hook);
//}
//bool Hook_get_resource_object(bool bEnable)
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_get_resource_object), _get_resource_object_Hook);
//}
//bool Hook_com_ptr_t_IWzProperty__ctor(bool bEnable)
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_com_ptr_t_IWzProperty__ctor), _com_ptr_t_IWzProperty__ctor_Hook);
//}
//bool Hook_com_ptr_t_IWzProperty__dtor(bool bEnable)
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_com_ptr_t_IWzProperty__dtor), _com_ptr_t_IWzProperty__dtor_Hook);
//}
bool HookPcCreateObject_IWzResMan(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9FAF55), _PcCreateObject_IWzResMan_Hook);
}
bool HookPcCreateObject_IWzNameSpace(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9FAFBA), _PcCreateObject_IWzNameSpace_Hook);
}
bool HookPcCreateObject_IWzFileSystem(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9FB01F), _PcCreateObject_IWzFileSystem_Hook);
}
bool HookCWvsApp__Dir_BackSlashToSlash(bool bEnable)
{
	BYTE firstval = 0x56;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F95FE;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F95FE), _CWvsApp__Dir_BackSlashToSlash_rewrite);
}
bool HookCWvsApp__Dir_upDir(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F9644), _CWvsApp__Dir_upDir_Hook);
}
bool Hookbstr_ctor(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_406301), _bstr_ctor_Hook);
}
bool HookIWzNameSpace__Mount(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F790A), _IWzNameSpace__Mount_Hook);
}
//void loadMyShA() {	//partial credits to blackwings v95
//	void* pDataFileSystem = nullptr;
//	void* pThePackage = nullptr; //9FB0E9
//	//@_com_ptr_t<_com_IIID<IWzNameSpace,&_GUID_2aeeeb36_a4e1_4e2b_8f6f_2e7bdec5c53d> > g_root
//	//_sub_9FAFBA(L"NameSpace", g_root, NULL);//void __cdecl PcCreateObject(const wchar_t* sUOL, _com_ptr_t<_com_IIID<IWzNameSpace, &_GUID_2aeeeb36_a4e1_4e2b_8f6f_2e7bdec5c53d> > *pObj, IUnknown * pUnkOuter)
//
//	void* pIWzNameSpace_Instance = g_root; //partial credits to https://github.com/MapleMyth/ClientImageLoader
//	//auto PcSetRootNameSpace = (void(__cdecl*)(void*, int)) * (int*)pNameSpace;//Hard Coded//HRESULT __cdecl PcSetRootNameSpace(IUnknown *pNameSpace)
//	//PcSetRootNameSpace(pIWzNameSpace_Instance, 1);
//
//	char sStartPath[MAX_PATH];
//	GetModuleFileNameA(NULL, sStartPath, MAX_PATH);
//	_CWvsApp__Dir_BackSlashToSlash_rewrite(sStartPath);	//_sub_9F95FE
//	_sub_9F9644(sStartPath);//_CWvsApp__Dir_upDir
//
//	strcat(sStartPath, "/Ezorsia_v2_files");//sStartPath += "./Ezorsia_v2_files";
//	//char sStartPath2[MAX_PATH]; strcpy(sStartPath2, sStartPath);
//	//strcat(sStartPath2, "/");//sStartPath += "./Ezorsia_v2_files";
//
//	Ztl_bstr_t BsStartPath = Ztl_bstr_t();
//	_sub_425ADD(&BsStartPath, nullptr, sStartPath);//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//	//Ztl_bstr_t BsStartPath2 = Ztl_bstr_t();
//	//_sub_425ADD(&BsStartPath2, nullptr, "/");//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//
//	_sub_9FB01F(L"NameSpace#FileSystem", &pDataFileSystem, NULL);//void __cdecl PcCreateObject(const wchar_t *sUOL, _com_ptr_t<_com_IIID<IWzFileSystem,&_GUID_352d8655_51e4_4668_8ce4_0866e2b6a5b5> > *pObj, IUnknown *pUnkOuter)
//	_sub_9FB084(L"NameSpace#Package", &pThePackage, NULL);//void __cdecl PcCreateObject_IWzPackage(const wchar_t *sUOL, ??? *pObj, IUnknown *pUnkOuter)
//	HRESULT v0 =_sub_9F7964(pDataFileSystem, nullptr, BsStartPath);//HRESULT __thiscall IWzFileSystem::Init(IWzFileSystem *this, Ztl_bstr_t sPath)
//	std::cout << v0 << " Hook_sub_9F7159 HRESULT 1: " << BsStartPath.m_Data << "   " << sStartPath << std::endl;
//	
//	const char* myWzPath = "EzorsiaV2_wz_file.wz";
//	Ztl_bstr_t BmyWzPath = Ztl_bstr_t();
//	_sub_425ADD(&BmyWzPath, nullptr, myWzPath);//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//
//	Ztl_variant_t pBaseData = Ztl_variant_t();
//	_sub_5D995B(pDataFileSystem, nullptr, &pBaseData, BmyWzPath);//Ztl_variant_t *__thiscall IWzNameSpace::Getitem(IWzNameSpace *this, Ztl_variant_t *result, Ztl_bstr_t sPath)
//
//	const char* mysKey = "83";
//	Ztl_bstr_t BmysKey = Ztl_bstr_t();
//	_sub_425ADD(&BmysKey, nullptr, mysKey);//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//	const char* mysBaseUOL = "/";
//	Ztl_bstr_t BmysBaseUOL = Ztl_bstr_t();
//	_sub_425ADD(&BmysBaseUOL, nullptr, mysBaseUOL);//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//	_sub_9F79B8(pThePackage, nullptr, BmysKey, BmysBaseUOL, pCustomArchive);//HRESULT __thiscall IWzPackage::Init(IWzPackage *this, Ztl_bstr_t sKey, Ztl_bstr_t sBaseUOL, IWzSeekableArchive *pArchive)
//
//
//
//	_sub_425ADD(&BsStartPath, nullptr, "/");//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s) //Ztl_bstr_t ctor
//	HRESULT v1 = _sub_9F790A(pIWzNameSpace_Instance, nullptr, BsStartPath, pThePackage, 0); //HRESULT __thiscall IWzNameSpace::Mount(IWzNameSpace *this, Ztl_bstr_t sPath, IWzNameSpace *pDown, int nPriority)
//	std::cout << v1 << " Hook_sub_9F7159 HRESULT 2: " << BsStartPath.m_Data << "   " << sStartPath << std::endl;
//
//
//
//
//	//void __thiscall _com_ptr_t<_com_IIID<IWzSeekableArchive == v95 9C4830 v83 9F7367
//} 
//bool Hook_sub_9F7159_initialized = true;
bool resmanSTARTED = false;
static _CWvsApp__InitializeResMan_t _sub_9F7159_append = [](CWvsApp* pThis, void* edx) {
	//-> void {_CWvsApp__InitializeResMan(pThis, edx);
	//if (Hook_sub_9F7159_initialized)
	//{
	//	std::cout << "_sub_9F7159 started" << std::endl;
	//	Hook_sub_9F7159_initialized = false;
	//}
	resmanSTARTED = true;
	//loadMyShA();
	//void* pData = nullptr;
	//void* pFileSystem = nullptr;
	//void* pUnkOuter = 0;
	//void* nPriority = 0;
	//void* sPath;
	//edx = nullptr
	// 
	//// Resman
	//_PcCreateObject_IWzResMan(L"ResMan", &g_rm, pUnkOuter);	//?(void*) //?&g

	//void* pIWzResMan_Instance = *&g_rm;	//?&g	//custom added, find existing instance
	//!!auto IWzResMan__SetResManParam = *(void(__fastcall**)(void*, void*, void*, int, int, int))((*(int*)pIWzResMan_Instance) + 20); // Hard Coded
	//!!IWzResMan__SetResManParam(nullptr, nullptr, pIWzResMan_Instance, RC_AUTO_REPARSE | RC_AUTO_SERIALIZE, -1, -1);

	//// NameSpace
	//_PcCreateObject_IWzNameSpace(L"NameSpace", &g_root, pUnkOuter);

	//void* pIWzNameSpace_Instance = &g_root;
	//auto PcSetRootNameSpace = (void(__cdecl*)(void*, int)) * (int*)pNameSpace; // Hard Coded
	//PcSetRootNameSpace(pIWzNameSpace_Instance, 1);

	//// Game FileSystem
	//_PcCreateObject_IWzFileSystem(L"NameSpace#FileSystem", &pFileSystem, pUnkOuter);
	_sub_9F7159(pThis, nullptr);	//comment this out and uncomment below if testing, supposed to load from .img files in folders but i never got to test it
	resmanSTARTED = false;
	//char sStartPath[MAX_PATH];
	//GetModuleFileNameA(NULL, sStartPath, MAX_PATH);
	//_CWvsApp__Dir_BackSlashToSlash(sStartPath);
	//_CWvsApp__Dir_upDir(sStartPath);

	//_bstr_ctor(&sPath, pData, sStartPath);

	//auto iGameFS = _IWzFileSystem__Init(pFileSystem, pData, sPath);

	//_bstr_ctor(&sPath, pData, "/");

	//auto mGameFS = _IWzNameSpace__Mount(*&g_root, pData, sPath, pFileSystem, (int)nPriority);

	//// Data FileSystem
	//_PcCreateObject_IWzFileSystem(L"NameSpace#FileSystem", &pFileSystem, pUnkOuter);

	//_bstr_ctor(&sPath, pData, "./Data");

	//auto iDataFS = _IWzFileSystem__Init(pFileSystem, pData, sPath);

	//_bstr_ctor(&sPath, pData, "/");

	//auto mDataFS = _IWzNameSpace__Mount(*&g_root, pData, sPath, pFileSystem, (int)nPriority);
	};
bool Hook_sub_9F7159(bool bEnable)	//resman hook that does nothing, kept for analysis and referrence //not skilled enough to rewrite to load custom wz files
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F7159;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F7159), _sub_9F7159_append);
}
struct KeyValuePair {
	int key;
	std::string value;
};
KeyValuePair newKeyValuePairs[] = {
    {11, "����"},
    {12, "����"},
    {13, "սʿ"},
    {14, "������"},
    {15, "����"},
    {16, "����"},
    {17, "����Ա"},
    {18, "��ʿ��"},
    {19, "����"},
    {20, "����Ա"},
    {22, "սʿ"},
    {23, "����"},
    {24, "��ʿ"},
    {25, "Ӣ��"},
    {26, "׼��ʿ"},
    {27, "��ʿ"},
    {28, "ʥ��ʿ"},
    {29, "ǹսʿ"},
    {30, "����ʿ"},
    {31, "����ʿ"},
    {32, "��ʦ"},
    {33, "��ʦ"},
    {34, "ħ��ʦ"},
    {35, "��ʦ(��,��)"},
    {36, "��ʦ(��,��)"},
    {37, "ħ��ʦ(��,��)"},
    {38, "��ʦ(��,��)"},
    {39, "��ʦ(��,��)"},
    {40, "ħ��ʦ(��,��)"},
    {41, "��ʦ"},
    {42, "��˾"},
    {43, "����"},
    {44, "������"},
    {45, "����"},
    {46, "����"},
    {47, "������"},
    {48, "����"},
    {49, "����"},
    {50, "����"},
    {51, "����"},
    {52, "�̿�"},
    {53, "��Ӱ��"},
    {54, "��ʿ"},
    {55, "����"},
    {56, "���п�"},
    {57, "����"},
    {58, "ȭ��"},
    {59, "��ʿ"},
    {60, "���ӳ�"},
    {61, "��ǹ��"},
    {62, "��"},
    {63, "����"},
    {64, "������"},
    {65, "��������������\r\n���Ժ����ԣ�"},
    {66, "����״̬�в���ִ�иò���"},
    {67, "�뷵�ؿ�ʼ������"},
    {68, "�Ȱ�ȷ����ȷ���Ƿ���øý�ɫ����"},
    {69, "���֤���벻��ȷ\r\n���������룡"},
    {70, "�����ٴ����½�ɫ��"},
    {71, "�Ƿ�ɾ���ý�ɫ��"},
    {72, "[������ɫע������]\r\n��ʹ������������򽫱�ϵͳ���ƣ�"},
    {73, "'%s'����ʹ�õĽ�ɫ��"},
    {74, "'%s'�ǿ���ʹ�õĽ�ɫ��"},
    {75, "����δ֪����\r\n��������ɫ����⡣"},
    {76, "����δ֪����\r\n�޷������½�ɫ��"},
    {77, "�޷�������Ϸ������\r\n���ԵȺ����ԡ�"},
    {78, "����Ϸ�����������ж�\r\n���ԵȺ����ԡ�"},
    {79, "����������ӷ�������\r\n���ԵȺ����ԡ�"},
    {80, "�޷���½��������\r\n����鿴ð�յ�ONLINE�ٷ���վ��"},
    {82, "���¼�����������ж�\r\n���ԵȺ����ԡ�"},
    {83, "�����ڴ治��\r\n��ϸ�Ľ���취����ʹٷ���վ��"},
    {84, "û�������ļ�\r\n���������¿ͻ��ˡ�"},
    {85, "����ȷ����ϷData\r\n���������¿ͻ��ˡ�"},
    {86, "����ȷ����Ϸ����\r\n���������¿ͻ��ˡ�"},
    {87, "����ȷ�Ŀͻ���\r\n���������¿ͻ��ˡ�"},
    {88, "����������ӷ��ִ���\r\n���ԵȺ����ԡ�"},
    {89, "���������������ͻ\r\n��ر�������������ԡ��绹���������⣬�����������������Ի���ʹٷ���վ��"},
    {90, "���������������ͻ��\r\n�����ǵ��Բ���������ʹ��ɱ�����������������ԣ�"},
    {91, "��IP��ַ����ֹ\r\n��������㡣"},
    {92, "���¼MapleStory��ҳ����\r\n��ʼ��Ϸ��ť��"},
    {93, "������������ð�յ��ͻ��ˡ�"},
    {122, "/���Ļ�"},
    {123, "/���"},
    {124, "/����"},
    {125, "/����"},
    {126, "/����"},
    {127, "/����"},
    {128, "/������"},
    {129, "/����"},
    {130, "/����"},
    {131, "/�̵�"},
    {132, "/��Ʒ�ƶ�"},
    {133, "/���״��"},
    {134, "/��ӿ���"},
    {135, "/����뿪"},
    {136, "/�������"},
    {137, "/����˳�"},
    {138, "/�������"},
    {139, "/������Ϣ"},
    {140, "/�˳�����"},
    {141, "/��������"},
    {142, "/�߳�����"},
    {143, "/Ż��"},
    {144, "/��ѽ"},
    {145, "/�д�"},
    {146, "/����"},
    {147, "���ڴ��������顣"},
    {148, "�����ʻ��޷����С�"},
    {149, "MWLB�ʻ��޷����С�"},
    {150, "�������Ĳ���ͬʱ���С�"},
    {151, "��'%s'��Ҵ��������Ļ���"},
    {152, "�Ҳ���'%s'��ҡ�"},
    {153, "'%s'�����'%s'�"},
    {154, "'%s'�����ð�յ�ONLINE�̳���"},
    {155, "'%s'��ҵ�����λ����'%s'"},
    {156, "'%s'������ر�������"},
    {157, "'%s'������ھܾ����Ļ���״̬��"},
    {158, "��δ�������û�����ߵĶ�Ա��"},
    {159, "�㻹δ������żû�����ߡ�"},
    {160, "û�пɶԻ��ĺ��ѡ�"},
    {161, "û�м�������û�����ߵļ����Ա"},
    {162, "�ɹ�ֹͣ�ˡ�"},
    {163, "ʧ��ֹͣ�ˡ�"},
    {164, "�ɹ���ֹ�ˡ�"},
    {165, "��ʾ���﷨�������⡣����ȷ�ϡ�"},
    {166, "ʧ�ܽ�ֹ�ˡ�"},
    {167, "�����а��Ͻ���ɹ��ˡ�"},
    {168, "����ȷ�Ľ�ɫ����"},
    {169, "�����а��Ͻ��ʧ���ˡ�"},
    {170, "�ǲ���ȷ��NPC�����������"},
    {171, "����ʧ��"},
    {172, "�޸ĳɹ�"},
    {173, "�޸�ʧ�� - �Ҳ��������õ����ˣ�"},
    {174, "��������λ�� : %s"},
    {175, "�Ҳ��������õ����ˣ�"},
    {247, "��á�����ð�յ�����Ա�����ڱ����Ĺ����������ִ塣��ʿ�ǿ����������ִ�"},
    {248, "��á�����ð�յ�����Ա�����ڱ����Ĺ�������ħ�����֡���ʿ�ǿ�������ħ������"},
    {249, "��á�����ð�յ�����Ա�����ڱ����Ĺ���������ʿ���䡣��ʿ�ǿ���������ʿ����"},
    {250, "��á�����ð�յ�����Ա�����ڱ����Ĺ������Է��䶼�С���ʿ�ǿ����������䶼��"},
    {251, "��á�����ð�յ�����Ա�����ڱ����Ĺ�����������ۡ���ʿ�ǿ������������"},
    {252, "��á�����ð�յ�����Ա�����ڱ����Ĺ����������֮�ǡ���ʿ�ǿ����������֮��"},
    {253, "��á�����ð�յ�����Ա�����ڱ����Ĺ���������߳ǡ���ʿ�ǿ���������߳�"},
    {254, "��á�����ð�յ�����Ա�����ڱ����Ĺ�������ͯ���塣��ʿ�ǿ�������ͯ����"},
    {257, "���������Դ��䡣���д������������"},
    {258, "�ӹ����Ϯ���гɹ�����˴�ׯ. ���'��ħ��Ϣ'����NPC'ð�յ���ӪԱ'�����Ѿ���ӡ '��ħ��Ϣ'������ֱ��ȥ��NPC '������'."},
    {266, "�����޷�������\r\n���ԵȺ����ԡ�"},
    {267, "������������λ�á�\r\n���ܲ鿴���ͼ��"},
    {270, "��NPC���������ͼ�޷���ʾ��"},
    {271, "��������������˵�����ݡ�"},
    {272, "����ʹ�ý�ֹ��"},
    {273, "ʹ�����ȵĻ�������дӢ����ĸ60�֣�����30�֣�"},
    {275, "�ڲʺ絺�ﲻ��ʹ�á�"},
    {276, "�����������ˡ�"},
    {277, "�������ͼ�����ˡ�"},
    {278, "���ܵ���ĵ�ȥ��"},
    {279, "���ֵĺ����Ļ��������\r\n\r\n��Ը��򿪰���"},
    {280, "���ڲ������졣"},
    {281, "�õ�����ֵ (+%d)"},
    {282, "�����ر���ֵ(+%d)"},
    {283, "������ɱ���⽱������(+%d)"},
    {284, "��齱������ֵ(+%d)"},
    {285, "����������ӽ�������ֵ(+%d)"},
    {286, "�õ���""����""��������ֵ��(+%d)"},
    {287, "�Ժ�%d��Ϊֹ��ɵ�����Ľ�������ֵ�����ӻ��������ֵ��"},
    {288, "������� (+%d)"},
    {289, "�����ȼ����� (%d)"},
    {290, "��ȡ��� (+%d)"},
    {291, "�����ر��� (+%d)"},
    {292, "��Ҽ��� (%d)"},
    {293, "������Ʒ (%s %d��)"},
    {294, "������Ʒ (%s)"},
    {295, "���ܼ�ȡ��Ʒ��"},
    {296, "�ֽ����[%s]�ѹ���\r\n���߱�����ˡ�"},
    {297, "���ڴ���������\r\n���Ժ����ԡ�"},
    {298, "����'%s'�������ȡ�"},
    {299, "������'%s'�������ȡ�"},
    {300, "��ɫ������ȷ��"},
    {301, "�ȼ�����15���Ľ�ɫ����ʹ�������ȹ��ܡ�"},
    {302, "���첻����ʹ�������ȹ��ܡ�"},
    {303, "����²����ٲ����ض���ɫ�������ȡ�"},
    {304, "'%s'��������'%s'��ҵ������ȡ�"},
    {305, "'%s'��ҽ�����'%s'��ҵ������ȡ�"},
    {306, "δ֪ԭ�����������ʧ�ܡ�"},
    {307, "�����Ժ����ԡ�"},
    {308, "%s��Ҵ��ھܾ����״̬��"},
    {309, "%s��Ҿܾ�������д���"},
    {310, "������ӡ�"},
    {311, "����ӱ��˳��ˡ�"},
    {312, "��ӽ�����"},
    {313, "'%s'��Ҵ���ӱ��˳��ˡ�"},
    {314, "'%s'����뿪��ӡ�"},
    {315, "�ӳ��˳�����ӽ����ˡ�"},
    {316, "�ӳ���ɢ��ӣ��뿪����ӡ�"},
    {317, "������ӡ�"},
    {318, "'%s'��Ҳμӵ���ӡ�"},
    {319, "�Ѿ����������顣"},
    {320, "���ֲ��ܿ�����ӡ�"},
    {321, "û�вμӵ���ӡ�"},
    {322, "��ӳ�Ա����"},
    {323, "���Ƕӳ�"},
    {324, "�޷��д��Լ���"},
    {325, "'%s'��Ҳ��ǸöӵĶ�Ա"},
    {326, "����Ա���ܿ����"},
    {327, "����Ա���ܼ������"},
    {328, "���ܼ������"},
    {329, "����δ֪����\r\n���ܴ���������롣"},
    {330, "����Ƶ�����Ҳ�����ɫ��"},
    {331, "��Ա:"},
    {332, "���Ƕӳ���"},
    {333, "��������ʱ��ܾ�����ӵ����롣"},
    {334, "%s��Ҿܾ��������״̬"},
    {335, "%s��Ҿܾ��˼�������"},
    {336, "������%s����"},
    {337, "���˳�����"},
    {338, "�뿪���塣"},
    {339, "'%s'��ұ��˳��˼���"},
    {340, "��ɾ���Ľ�ɫ�Զ��˳��˼��塣"},
    {341, "'%s'�˳��˼���"},
    {342, "�����'%s'����˳�������"},
    {343, "�����뿪������"},
    {344, "'%s'�˳��˼���"},
    {345, "�����ɢ��"},
    {346, "�����˼���"},
    {347, "'%s'��Ҽ����˼���"},
    {348, "�Ѿ��м�����"},
    {349, "�ȼ�̫�Ͳ��ܽ�������"},
    {350, "û�вμӵļ���"},
    {351, "�峤�����˳����塣"},
    {352, "������������"},
    {353, "���ֲ��ܵ��峤"},
    {354, "���Ǽ�����峤"},
    {355, "���������Լ�"},
    {356, "'%s'��Ҳ��Ǽ����Ա"},
    {357, "ֻ�м��峤����Ȩ�޳�ȥ�����峤��"},
    {358, "���峤�޷���ȥ��"},
    {359, "����Ա���ܿ�����"},
    {360, "��������������Ա�ˡ�"},
    {361, "����δ֪�����޷������йؼ��������"},
    {362, "��ǰƵ���Ҳ����ý�ɫ"},
    {363, "��Ա:"},
    {364, "�����峤"},
    {365, "���������������ӵ�%d�ˡ�"},
    {366, "�Ҳ����ý�ɫ��"},
    {367, "�Ѽ���ĺ��ѡ�"},
    {368, "���ܰ��Լ����뵽���ѡ�"},
    {369, "δ������ѡ�"},
    {370, "û����¼����ĺ���"},
    {371, "���ڹر��������š�"},
    {372, "����ȥ��ĵ�"},
    {373, "����ȷ��Ƶ����"},
    {374, "�����������Աר������\r\n�������������Щ\r\n�ر�ķ���"},
    {375, "\r\n��Windows98Ҳ�ᷢ����Ϸ���в�̫˳���������\r\n��������XP��2000ϵͳ"},
    {376, "ŶŶ���������������Ҵ���ĳ��ﰡ����Щ�����ұ�������㣡���ǿ�������ˮ��ħ��ʦ��\r\n\r\n��Щ�����������������ϼ�������ˮ������ħ�������ħ�����������о��й�������ħ���Ѿ��ܳ�ʱ���ˣ����ڷ����˴���������ħ������������ȱ�㣬�Ǿ��ǣ���û������ˮ����ħ��Ҳ����ʧ����������ˮÿ�βɼ����������١�������Щ�����Щ����ÿ��һ��ʱ�䶼��Ҫ����һ�Ρ�����"},
    {377, "������ˮ�������ʱ�����ͻ�ص����޵�״̬����Ȼ���Ǻܱ��˵����飬��Ҳ����̫���ġ�������ˮ�Ϳ���ʹ�����Ϊ�����ô��һֱӵ�пɰ��ĳ��������ÿ�춼�ں���ĵ��¿�������ˮ������Ժ�����Ҫ����ˮ�Ļ���ȥ���ҵ�ͽ�ܰɡ���ɱ����ǣ����������ֻ�С�90������ˡ���ǧ��Ҫ��ϧ��������"},
    {378, "������ħ����ʱ�䣬���ܶ�����ȥ��"},
    {379, "�ó����޷����\r\n���̳ǿɹ����µĳ��\r\n����Ҫ�ƶ�ô��"},
    {380, "���ܶ����� (+%d)"},
    {381, "���ܶ��½� (%d)"},
    {382, "������Ӷ����ղŻؼ��ˡ�"},
    {383, "����ĸ�����ˮ�������ޡ�"},
    {384, "�ѳ�����ͬʱ�ٻ���������ޣ�\r\n�뽫����ת��Ϊ���޺����ԣ�"},
    {385, "��Ҫ���˳�����Ϊ��Ҫ������\r\n���ƶ��ٶȣ���Ծ����\r\n����ȡ�ض����߹�������Ҫ�������������"},
    {386, "��ѡ��Ҫ��װװ���ĳ��"},
    {387, "���ᷢ��һ�����⣬�ڵ����������ĳ�����ص�������"},
    {388, "���ᷢ��һ�����⣬���ǵ���û���κα仯��"},
    {389, "��Ϊ�����������������ʧ�ˡ�"},
    {390, "����<%s>������������"},
    {392, "����ʱ�̱����¡�"},
    {393, "����ǰ5���ӿ��Խ�����ҡ�"},
    {394, "�Ѿ��ر��ˡ�"},
    {395, "�Ѿ���Ա�ˡ����ڲ��ܽ�ȥ��"},
    {396, "������������顣"},
    {397, "����״̬�в���ִ�иò�����"},
    {398, "���ر��в���ִ�иò�����"},
    {399, "�ý�ɫ����ִ�иò�����"},
    {400, "�����������⽻�ס�"},
    {401, "�����ﲻ�ܿ�����̡�"},
    {402, "��ͬһ����ͼ��\r\n�ſ��������ס�"},
    {403, "'%s'�������������¡�"},
    {404, "'%s'��Ҿܾ���������롣"},
    {405, "'%s'����Ǿܾ������״̬��"},
    {406, "�Է���ֹ���ס�"},
    {407, "���׳ɹ��ˡ�\r\n����ȷ�Ͻ��׵Ľ����"},
    {408, "�۳������Ѻ󣬻����%d��ҡ�\r\n��ȷ�ϡ�"},
    {409, "����ʧ���ˡ�"},
    {410, "�򲿷ֵ�������������ֻ��ӵ��һ��\r\n����ʧ���ˡ�"},
    {411, "˫���ڲ�ͬ�ĵ�ͼ���ܽ��ס�"},
    {412, "Ҫ�ǼǶ���?"},
    {413, "��Ҫ��������?"},
    {414, "������Ҫ����ĵ�����\r\n\r\n������ǰ�ƶ�����ĵط�\r\n�Ͳ��ܿ�������ˡ�"},
    {415, "ÿ%d����%s���"},
    {416, "%s���"},
    {417, "���̹���"},
    {418, "����Ʒ������"},
    {419, "��Ʒ������Ӧ�������۵�λ�ı�����"},
    {420, "�ܼ���%d��ҡ�\r\n��Ը������Ʒ��"},
    {421, "���ܼ�����Ʒ��"},
    {422, "��λ����"},
    {423, "����"},
    {424, "���"},
    {425, "Ҫ������Ʒ����"},
    {426, "ÿ��λ����Ʒ����"},
    {427, "����Ʒ�Ѿ����ꡣ"},
    {428, "�޷�����õ���"},
    {429, "���뽫%s����߳�ȥ��"},
    {430, "���߿��ˡ�"},
    {431, "����ʱ�����Ʊ��Զ��߳��ˡ�\r\n�޷��ٴν��롣"},
    {432, "������Ҫ������Ϸ��"},
    {433, "��Ϊ��Ľ�Ҳ���,�㲻�ܲμӡ�"},
    {434, "�����ﲻ�ܿ���Ϸ����"},
    {435, "�����̵�ֻ�������г���\r\n���Կ���."},
    {436, "ֻ��λ�������г����2¥���ϵ�\r\n����(7��~22��)��ʹ�á�"},
    {437, "[%s]���뷿�䡣"},
    {438, "[%s]�˳����䡣"},
    {439, "[%s]��������˳����䡣"},
    {440, "[%s]���ȡ���˳����롣"},
    {441, "[%s]���ǿ���˳����䡣"},
    {442, "�㱻ǿ���˳����䡣"},
    {443, "[%s]�����Ȩ�ˡ�"},
    {444, "[%s]��ҵ��ò�Ҫ��ͬ���ˡ�"},
    {445, "����Ϸ�����߳��ˡ�"},
    {446, "������[%s]��ҷ��ơ�"},
    {447, "[%s]��ҳɹ��ˣ��������С�"},
    {448, "��[%s]��ҽ�Ҳ��㣬���ܿ�ʼ��Ϸ��"},
    {449, "��ʼ�ˡ�"},
    {450, "��Ϸ�����ˡ�\r\n10����Զ��ط���"},
    {451, "ʣ��10��"},
    {452, "�ط��ˡ�"},
    {453, "Ӯ�ˡ�"},
    {454, "ƽ�֡�"},
    {455, "���ˡ�"},
    {456, "�����Ȩ��"},
    {457, "Ҫ�˳��Է���"},
    {458, "�Է�����ƽ�֡�\r\nͬ����"},
    {459, "Ҫ��Է�����ƽ����"},
    {460, "�Է��ܾ���ƽ������"},
    {461, "�ò�����ÿһ��ֻ����һ�Ρ�"},
    {462, "�Է������ò�һ�֡�\r\nͬ����"},
    {463, "Ҫ��Է������ò�һ����"},
    {464, "�Է��ܾ�������ò�����"},
    {465, "ҪԤ���˷���"},
    {466, "Ҫȡ��Ԥ���˷���"},
    {467, "ʣ��ʱ�� : %d ��"},
    {468, "[ %s ]��ҵ�˳��"},
    {469, "��Ҫ�˷���"},
    {470, "���벻���ˡ�"},
    {471, "Ը��ǹ�����Ϸ��"},
    {472, "���ܽ������������"},
    {473, "˫����λ�á�"},
    {474, "������������¡�"},
    {475, "����Ʈ������"},
    {476, "������Ʒ�Ľ�ɫ������ȷ��"},
    {477, "�����ɹ�������ȷ�Ͻ����"},
    {478, "����ִ������������"},
    {479, "���ܰᵽͬ�˻��Ľ�ɫ��"},
    {480, "�Է�����������Ϸ��"},
    {481, "�Է�����Ʒ������û�пռ䡣"},
    {482, "û��Ҫ���˵���Ʒ���Ǯ��"},
    {483, "δ��¼�Ľ�ɫ��"},
    {484, "δ֪ԭ������ʧ�ܡ�"},
    {485, "���ѡ�񻻹�����Ʒ\r\n����Ʒ�ͻ�����Ʒ������ʧ\r\n�����ڼ۸��%d��(%d)��ת��Ϊ��ĵ���ȯ\r\n�õ���ȯҲ���Թ�����Ʒ��\r\nȷʵҪ��������Ʒ��"},
    {486, "ɾ����ѡ�ĵ���\r\n����ת���ɵ��þ�\r\n����ɾ���õ�����\r\n "},
    {487, "%s���Ӳֿⴰ����%d��ȯ��\r\nȷ���Ƿ���"},
    {488, "���Ӳֿⴰ��%d��ȯ��\r\nȷʵҪ������"},
    {489, "��ɫ���ӻ�Ա��(%d��ȯ)\r\n���Ӻ��޷�ȡ�����˿\r\n�Ƿ����ӽ�ɫ�۸�����"},
    {490, "����󣬿�����90����װ��������Ʒ\r\n��\r\n������޷�ȡ�����˿\r\n�Ƿ���"},
    {491, "�������ӱ�ѡ�ı����ո�����"},
    {492, "������������ֿ�ռ䡣"},
    {493, "�޷����ӽ�ɫ��������"},
    {494, "��ְҵ���Ʋ���ʹ�øõ��ߡ�\r\n��Ҫ�������"},
    {495, "�ȼ������޷�ʹ�øõ��ߡ�\r\n��Ҫ�������"},
    {496, "�����Ȳ����޷�ʹ�øõ��ߡ�\r\n��Ҫ�������"},
    {497, "(��Ϊ����Ҫ������Ҫ��\r\n��������Щ���ߡ�)"},
    {498, "�õ����Ѵ��ڡ�\r\n��Ҫ�������"},
    {499, "������װ��������\r\n����ʹ�����ֵ��ߡ�\r\n������Ҫ������"},
    {500, "ֻ��³����������װ������Ʒ."},
    {501, "�����ڵĳ��ﲻ��װ��\r\n�õ��ߡ����빺����"},
    {502, "��Щװ��ֻ�г������á�\r\n���빺����"},
    {503, "û�г�����޷�ʹ�õĵ���\r\n���������޵�ʱ��Ҳ�޷�ʹ��.\r\n��Ҫ������"},
    {504, "������޷�ȡ�����˿\r\n�Ƿ������Ʒ��"},
    {505, "Ŀǰ��ĳ���\r\n�޷�ʹ�õĵ���.\r\n��Ҫ������"},
    {506, "������޷�ȡ�����˿�Ƿ���������Ʒ��"},
    {507, "һ�� %d��ȯ"},
    {508, "ȷ��������"},
    {509, "����ʧ��"},
    {510, "�������ѡ�ĵ��ߡ�\r\n(�����򲻵���Щ���ߡ�)"},
    {511, "������Ҫ��������Ľ�ɫ��\r\nֻ�����͸�ͬһ�������ҡ�"},
    {512, "%s(%d�ֽ�)\r\n���ͺ��޷�ȡ�����˿\r\n�Ƿ���Ʒ���͸���%s��\r\n��"},
    {513, "��ȷ�����֤�ź�\r\n���ԡ�"},
    {514, "��ȷ�ϳ������ں�\r\n���ԡ�"},
    {515, "��ȷ���ʻ���\r\n���ԡ�"},
    {516, "�����ֲ���ʹ��\r\n����ȷ�ϡ�"},
    {517, "�������ѱ�ʹ��\r\n�������������֡�"},
    {518, "����ʹ�ô����֣��밴ȷ����"},
    {519, "�������ѱ�ʹ�ã�\r\n�������������֡�"},
    {521, "�õ����ѱ��������Ƹ������ύ��"},
    {523, "����������������ơ�\r\n�������Ƿ���һ�������Ѿ�\r\n���Ľ�ɫ���ơ� "},
    {527, "�ڹ�ע��ƷĿ¼���Ѿ���ͬ���ĵ��ߡ�"},
    {528, "��ע��ƷĿ¼�����ˡ�"},
    {529, "����ˡ�"},
    {530, "�͸�'%s'\r\n%d��[%s]��"},
    {531, "�����˱�����������"},
    {532, "�Ѿ����ӽ�ɫ��������"},
    {533, "ɾ�����ֽ���ߡ�"},
    {534, "�����˹���ʱ�䡣\r\n���Ժ����ԡ�"},
    {535, "��ȯ���㡣"},
    {536, "δ��14����û�����\r\n�����ֽ���ߡ�"},
    {537, "�����˿�����������޽�"},
    {538, "��ȷ���Ƿ񳬹�\r\n���Ա��е��ֽ����������"},
    {539, "��ȷ�Ͻ�ɫ���Ƿ���ȷ\r\n�����Ա����ơ�"},
    {540, "�����˸õ��ߵ�ÿ�չ����޶\r\n�޷�����"},
    {541, "����Ա��ʺ������콱����"},
    {542, "�Ѿ�ʹ�ù����콱��"},
    {543, "�Ѿ����ڵ��콱��"},
    {547, "����ȷ���콱�������Ƿ���ȷ��"},
    {549, "�콱���ѹ������ϡ�"},
    {550, "�콱���Ѿ��������"},
    {551, "�����콱����ר�õ��ߡ�\r\n�����㲻�����ͱ��ˡ�"},
    {552, "��ȯ��ð�յ�ר�õ���ȯ�޷��͸�\r\n���ˡ�"},
    {553, "���ֵ���ֻ�������Ա����\r\n��õ���"},
    {554, "���˵���ֻ�����͸���ͬƵ����\r\n��ͬ�Ա�Ľ�ɫ����ȷ��\r\n�Ƿ���Ҫ������Ľ�ɫ\r\n��ͬһƵ�����Ա�ͬ��"},
    {555, "�����˵�������ƶ"},
    {556, "�ֽ��̵겻����\r\n��beta���Խ׶Ρ�\r\n���ڸ��������Ĳ��㣬�������Ǹ�⡣"},
    {557, "��Ϊ����δ֪����\r\n���ܽ��뵽MapleStory�̳ǡ�"},
    {558, "�õ���ȯ����ߡ�"},
    {559, "�����ظ���ɫ��"},
    {560, "��ȷ��Ҫ�������%d������?"},
    {561, "���е����ﶼ���ͳ�.\r\n������%d��ȯ."},
    {562, "�������޷��ͳ���"},
    {563, "[ %s ]\r\n�ѳɹ��ʹ����Ͻ�ɫ.\r\n(�п����޷��͸�������.)������\r\n%d��ȯ."},
    {564, "��Ҫ����������ˡ�"},
    {565, "�����������û�����޵ĵ��߲ſ��Ի�����"},
    {566, "���ֵ��߲��ܻ�����\r\n�����޵ĺͰ������ĵĵ���\r\n�����ܻ�����"},
    {567, "�ֽ���߻����ɹ���\r\n(����%d ����ȯ)"},
    {568, "Ŀǰѡ�����Ĺ�ע��ƷĿ¼"},
    {569, "��ӭ������MapleStory�̳ǡ�"},
    {570, "����׼����Ʒ���������������½⡣"},
    {571, " ��ȯ"},
    {572, "(%d��)"},
    {573, "���ڹ�ע��ƷĿ¼�ϡ�"},
    {574, "[����]%s"},
    {575, "�ֿ����ӡ�"},
    {576, "Ϊ�ֿ����%d�����ӣ���ǰ��%d����\r\n���48��%d ��ȯ"},
    {577, "Ϊ�ֿ����%d�����ӣ���ǰ��%d����\r\n���48��"},
    {578, "����%s�����ռ�"},
    {579, "����%d��%s��������. (��ǰ: %d ��)\r\n���%d��.%d ��ȯ."},
    {580, "����%d��%s��������. (��ǰ: %d ��)\r\n���%d��"},
    {581, "���ӽ�ɫ��������%d����\r\n ÿ�ο�����1����ɫ�����������ӵ�%d����ɫ��"},
    {582, "������ȷ�����콱������30λ��"},
    {583, "������ȷ����Ҫ������Ľ�ɫ����"},
    {584, "Ҫ������Ľ�ɫ��"},
    {585, "�յ���"},
    {586, "\r\n%s %d��"},
    {587, "����ȯ %d��"},
    {588, "%d���"},
    {589, "��%s���"},
    {590, "��ֵ"},
    {591, "���ڲ����۵���Ʒ��"},
    {592, "����Ҫ�뿪�̳���"},
    {593, "������ÿ���Ȳ���"},
    {594, "������δ֪����\r\n�޷�ȷ�������̵깺����Ϣ��\r\n��Ϸ�̵��������ʹ�á�"},
    {595, "ӵ�еĽ�ҵ���\r\n��������\r\n�޷�ȷ�������̵깺����Ϣ��\r\n��Ϸ�̵��������ʹ�á�"},
    {596, "�޷��õ������̵��й������Ʒ\r\n��\r\n��ȷ�Ͽ�ӵ�еĽ�ҵ��߸���\r\nû�г��ޡ�\r\n��Ϸ�̵��������ʹ�á�"},
    {597, "�����̵��д��󶩵���\r\n��Ϸ�̵��������ʹ�á�"},
    {598, "ǿ����ʯ"},
    {599, "����ᾧ"},
    {600, "�ֽ�װ��"},
    {601, "���ֽ�"},
    {602, "���ָ�"},
    {603, "���ֶ���"},
    {604, "�̵�"},
    {605, "����"},
    {606, "����"},
    {607, "˫�ֽ�"},
    {608, "˫�ָ�"},
    {609, "˫�ֶ���"},
    {610, "ǹ"},
    {611, "ì"},
    {612, "��"},
    {613, "��"},
    {614, "ȭ��"},
    {615, "ָ��"},
    {616, "��ǹ"},
    {617, "ñ��"},
    {618, "����"},
    {619, "����"},
    {620, "����"},
    {621, "�׷�"},
    {622, "����"},
    {623, "��/ȹ"},
    {624, "Ь��"},
    {625, "����"},
    {626, "����"},
    {627, "����"},
    {628, "��ָ"},
    {629, "׹��"},
    {630, "ѫ��"},
    {631, "����"},
    {632, "ȫ������"},
    {633, "����"},
    {634, "��������"},
    {635, "˫������"},
    {636, "ҩˮ��"},
    {637, "�سǾ���"},
    {638, "���������߾���"},
    {639, "��������ǹ��С��"},
    {640, "ս��Ʒ"},
    {641, "ĸ����"},
    {642, "��Ϸ"},
    {643, "�ٽ�����������"},
    {644, "�ǳ���"},
    {645, "�ȽϿ�"},
    {646, "��"},
    {647, "��ͨ"},
    {648, "��"},
    {649, "�Ƚ���"},
    {650, "�ǳ���"},
    {651, "'%s'�͸�������"},
    {652, "������%d��"},
    {653, "������4Сʱ"},
    {654, "������%dСʱ"},
    {655, "ʹ�����޵�%04d��%d��%d�� %02d:%02d"},
    {656, "�����ٶ� :"},
    {657, "������ :"},
    {658, "ħ���� :"},
    {659, "������ :"},
    {660, "ħ�������� :"},
    {661, "������ :"},
    {662, "�ر��� :"},
    {663, "�ּ� :"},
    {664, "�ƶ��ٶ� :"},
    {665, "��Ծ�� :"},
    {666, "��Ӿ�ٶ� : +%d"},
    {667, "ֱ�ӹ���ʱ������������ļ��� :"},
    {668, "���������� :"},
    {669, "��ӷ���"},
    {670, "��ӷ���"},
    {671, "'%s'����͸��������"},
    {672, "����%s���֮������˵���"},
    {673, "'%s'��ҵ�ֿ�ѵ��ߡ�"},
    {676, "ħ��ʱ�� : %d"},
    {677, "ħ��ʱ�� : %dСʱ %d����"},
    {678, "������ħ��ʱ�䡣"},
    {679, "%04d��%02d��%02d��%02dʱǰ����"},
    {680, "%s / %s /%s / ����"},
    {681, "���ߺ����� / �ܺ�����"},
    {682, "���ߺ����� / �ܺ�����"},
    {683, "[ �ѵ�¼������ / �ɵ�¼�������� ]"},
    {684, "�������� / �ܼ����Ա"},
    {685, "�������� / �ܼ����Ա"},
    {686, "������˳������"},
    {687, "��ְҵ˳������"},
    {688, "�Լ���˳������"},
    {689, "��ְλ˳������"},
    {690, "�ѵ�¼��Ա / �ɵ�¼��Ա"},
    {691, "�ڱ�ĵط�"},
    {692, "���е���"},
    {693, "�������"},
    {694, "����������"},
    {695, "���ɽ���"},
    {696, "�ɽ���1��(���׺󲻿ɽ���)"},
    {697, "���ϵ���"},
    {698, "����������"},
    {699, "װ�����޷�����"},
    {708, "���ڵĺ�����  [%d/%d]"},
    {709, "Ŀǰλ�� - %s"},
    {710, "%s'��λ�� - %s"},
    {711, "���������ѵĽ�ɫ����\r\n\r"},
    {712, "�������������Ľ�ɫ����"},
    {713, "��Ҫ��'%s'��ҴӺ���Ŀ¼\r\n��ɾ����"},
    {714, "%s���"},
    {715, "������"},
    {716, "��ͬƵ��"},
    {717, "��%s���"},
    {718, "ð�յ�ONLINE�̳�"},
    {719, "��Ϊ����δ֪������������޷�����"},
    {720, "����Ŀ¼�����ˡ�"},
    {721, "�Է��ĺ���Ŀ¼�����ˡ�"},
    {722, "�Ѿ��Ǻ��ѡ�"},
    {723, "û��¼�Ľ�ɫ��"},
    {724, "���ܰѹ���Ա��Ϊ���ѡ�"},
    {725, "������Ҫ����������Ľ�ɫ����"},
    {726, "�޷����Լ��ŵ���������"},
    {727, "�ⲻ�ǽ�ɫ��\r\n���ٴ�ȷ�ϡ�"},
    {728, "�ý�ɫ�Ѿ���½��\r\n������ȷ���¡�"},
    {729, "�����̳Ƕ���Ϣ���ߺ�ſ���ʹ�á�"},
    {730, "��ӳ�Ա������2~6�ˡ�"},
    {731, "����ļ��ӳ�Ա%d�����ϡ�"},
    {732, "�ȼ���Χ���Ϊ30����"},
    {733, "ĿǰMapleStory����ߵȼ�Ϊ\r\n200����"},
    {734, "���˵ĵȼ������������ȼ��ķ�Χ֮�ڡ�"},
    {735, "�����ȼ������ֵҪ����Сֵ��"},
    {736, "��ѡ��ϣ�������ӵ�ְҵ��"},
    {737, "����ѡ����Ҫ�����ĵ��ߣ�"},
    {738, "�����пռ䲻�㡣"},
    {739, "�޷���ӵĵ��ߡ�"},
    {740, "��ͬ�ĵ����Ѿ���¼��"},
    {741, "�޷���¼����Ĳ��ϡ�"},
    {742, "ֻ�ܵ�¼ǿ����ʯ��"},
    {743, "�޷���¼�����ǿ����ʯ��"},
    {744, "��ͬ�����ǿ����ʯֻ�ܵ�¼1����"},
    {745, "��ǿ����ʯֻ�����������ϡ�"},
    {746, "ֻ�ܵ�¼�߻�����"},
    {747, "�߻���ֻ�ܵ�¼1����"},
    {748, "�޷���¼�Ĵ߻�����"},
    {749, "�޷���������ᾧ�Ĳ��ϡ�"},
    {750, "����δ֪���󣬵�������ʧ�ܡ�"},
    {751, "������%d��%s��"},
    {752, "%s�ѳɹ��ֽ⡣"},
    {753, "����ʧ�ܡ�"},
    {754, "�������ƻ���"},
    {755, "��Ҫ��¼�����أ�"},
    {756, "��Ҫ������"},
    {757, "��Ҫ��������ᾧ��"},
    {758, "��Ҫ�ֽ�װ����"},
    {759, "��Ҫ������"},
    {760, "�����ڴ˵�¼����ս��Ʒ\r\n�������ɹ���ᾧ��"},
    {761, "����Ҫ�ֽ��װ���ŵ����\r\n���Ի�ù���ᾧ��"},
    {762, "��������Ҫ�д��Ľ�ɫ����"},
    {763, "'%s'��ҵ�"},
    {764, "�д����롣"},
    {765, "�д����ѡ�"},
    {766, "�������롣"},
    {767, "������롣"},
    {768, "�������롣"},
    {769, "�������ð��ѧԺ��"},
    {770, "�µ�����"},
    {771, "������ɣ�"},
    {772, "�ʺ絺"},
    {773, "������"},
    {774, "û���й��������Ϣ��"},
    {775, "�����˺Ų����ӵ���ҡ�"},
    {776, "- ��'%s'��Ҵ������д����롣"},
    {777, "- �Ҳ���'%s'��ҡ�"},
    {778, "'%s'��������Ǿܾ�����״̬��"},
    {779, "'%s'��Ҿܾ������롣"},
    {780, "- ��'%s'��ҷ����������롣"},
    {781, "��%s'���������\r\n���졣�Ƿ�ͬ�⣿"},
    {782, "���������Ļ��Ķ���"},
    {783, "�˻����ԡ�"},
    {784, "���ܺ��Լ�˵���Ļ���"},
    {785, "��ѡ��Ի���Ⱥ��\r\n\r"},
    {786, "��������"},
    {787, "�����"},
    {788, "�Ժ���"},
    {789, "�Է���"},
    {790, "�Լ���"},
    {791, "��Ⱥ"},
    {792, "�������봫�ݵ����ݡ�"},
    {793, "�������Ѿ�����\r\n��������"},
    {794, "������������֡�"},
    {795, "�����ø����֡�"},
    {796, "%s\r\n\r\n�����ֶ���"},
    {797, "�����ý�Ұ�\r\n����ȡ %d���ô?"},
    {798, "����ʹ�ý�Ұ�\r\n��ȡ�� %d ��ҡ�"},
    {799, "ʹ�ý�Ұ�ʧ�ܡ�"},
    {800, "[ MapleStoryͨѶ ���� ]"},
    {801, "�д� ��/�д� ��ɫ��"},
    {802, "���� ��/q"},
    {803, "- ��%s'����볡�ˡ�"},
    {804, "- ���ܲ������д������췿��"},
    {805, "- ��%s'����˳����䡣"},
    {806, "- �����������ɫ����"},
    {807, "- ��ɫ������ȷ��"},
    {808, " �����������"},
    {809, "��ɫ������Ӧ��%d�ַ����ϡ�"},
    {810, "��ɫ������Ӧ��%d�ַ����¡�"},
    {811, "��ɫ���ﲻ�ܰ����ո�"},
    {812, "�ڽ�ɫ������\r\n�Ƿ��ַ���"},
    {813, "�ڽ�ɫ������\r\n�����ַ���"},
    {814, "�Խ�ɫ������ȷ��"},
    {815, "MapleStory�ļ���"},
    {816, "����"},
    {817, "ֻ���������֡�"},
    {818, "ֻ��%d���ϵ����ֿ�������"},
    {819, "ֻ��%d���µ����ֿ�������"},
    {820, "������������%dλ���ϡ�"},
    {821, "��������%d�����ϡ�"},
    {822, "���������֤�š�"},
    {823, "�����������"},
    {824, "�����ֲ���ʹ��\r\n����ȷ�ϡ�"},
    {825, "�������ѱ�ʹ��\r\n�������������֡�"},
    {826, "Ϊ��֤�ʻ��İ�ȫ,\r\n������8λ�������մ��롣\r\n����) ��/��/�� -> 20200220"},
    {827, "������Ϣ"},
    {828, "������ +"},
    {829, "������ -"},
    {830, "��������"},
    {831, "�������"},
    {832, "��������"},
    {833, "����ϵͳ�����޷����ӡ�"},
    {834, "��ʱ��ɾ��������ID��ʹ�á�"},
    {835, "IDδע�ᡣ"},
    {836, "ID��������\r\n���߷��������ڼ���С�"},
    {837, "��������ʼ���ַ"},
    {838, "�����ʼ���ַ������Ҫ%d����ĸ��"},
    {839, "�����ʼ���ַ�ĳ��Ȳ��ܳ���%d����ĸ��"},
    {840, "���е����ʼ���ַ�����������@����\r\n�����磩maple@wizet.com"},
    {841, "���е����ʼ���ַ�����������.��\r\n��ex��maple@wizet.com"},
    {842, "���볤�Ȳ��ܳ���%d����ĸ��"},
    {843, "���������20����ܽ��롣"},
    {844, "�㲻����Ȩ�������"},
    {845, "�ѳ�����IP��ַ���������û�����"},
    {846, "IP���ڷ���Χ��"},
    {847, "���ڲ����������Ϸ��ʱ�䡣"},
    {848, "��Ҫ����"},
    {849, "����٣�"},
    {850, "��Ҫ����"},
    {851, "�����٣�"},
    {852, "��Ʒ������"},
    {853, "��ȷ���ǲ������\r\n�����Ŀռ䲻����"},
    {854, "���ֵ��߲������������ϡ�"},
    {855, "����δ֪���󣬲��ܽ��ס�"},
    {856, "%s ��� / ��ֵ : %d"},
    {857, "���ֵ��"},
    {858, "��Ҫȡȥ��"},
    {859, "ȡ��һ����Ҫ%d���\r\n����ȡ����"},
    {860, "��Ҫ������"},
    {861, "����һ����Ҫ%d���\r\n���뱣����"},
    {862, "���ܼ�����"},
    {863, "ȡ�����٣�"},
    {864, "���ܶ��٣�"},
    {865, "�ֿ�������"},
    {867, "��װ����װ�����޷����ס�\r\n��Ҫװ����"},
    {868, "����ֵ���㣬����ʹ�øõ��ߡ�"},
    {869, "���˽�ָÿ����ֻ��ʹ��һ����"},
    {870, "ֿ�ѽ�ָһ����ֻ��ʹ��һ��"},
    {871, "�������Ӧ��װ��ĳ�ֵ���\r\n�ſ���ʹ�á�"},
    {872, "��һ��Ҫװ������\r\n�ſ���ʹ�á�"},
    {873, "��Ҫ��ѱ���Ĺ��\r\n�ſ���װ����"},
    {874, "��Ҫ�л���װ���İ��ӣ�\r\n�ſ���װ����"},
    {875, "�˵��߲�����������ǰʹ�õ�������"},
    {876, "�����ܽ���װ��������ǰװ����\r\n�ֽ����һ��ʹ�á�\r\n��ȡ��װ���ֽ���ߣ�Ȼ�����ԡ�"},
    {877, "����Ʒ����װ��������ǰ�ĳ��"},
    {878, "�㲻���������ͼ�����װ����"},
    {879, "����ǰû��һ����Ծ�ĳ��"},
    {880, "Ŀǰ�ĳ����޷�װ���ĵ��ߡ�"},
    {881, "�����ʻ�\r\n�����ӵ����ߡ�"},
    {882, "���ֵ��߲����ӵ���"},
    {883, "�ڵ�ǰ��ͼ���޷�������ԡ�"},
    {884, "�ü����޷��ڵ�ǰ��ͼʹ�á�"},
    {885, "�ӵ����٣�"},
    {886, "���ֵ��߲��ܸ����ˡ�"},
    {887, "������٣�"},
    {888, "[��ӭ] ��ӭ����ð�յ�ONLINE���硣"},
    {889, "[ð�յ�ONLINE ����]"},
    {891, "��������Ϸ\r\n��ֹˢ����"},
    {892, "��Ļ�̫����\r\n���˻�����ġ�"},
    {893, "Ŀǰ���ڽ���״̬���޷��������졣"},
    {894, "һ��"},
    {895, "����"},
    {896, "����"},
    {897, "����"},
    {898, "����"},
    {899, "����"},
    {900, "����"},
    {901, "����"},
    {902, "����"},
    {903, "ʮ��"},
    {904, "ʮһ��"},
    {905, "ʮ����"},
    {906, "�������ӡ�"},
    {907, "ʣ��ʱ��: %02d�� %02d��"},
    {908, "���ر��õ�ͼ�ϲ���˵���Ļ���"},
    {909, "[��:"},
    {910, "�û�̫�ٲ��ܿ�ʼ��"},
    {911, "��û�趨��Ʒ��"},
    {912, "��ϲ�㣡����%dǿս��"},
    {913, "��ϲ�㣡����������"},
    {914, "��ϲ���Ϊ�ھ���"},
    {915, "�ɹ����趨�˽�Ʒ��"},
    {916, "��Ʒ�趨ʧ�ܡ�\r\n����ȷ����Ʒ���롣"},
    {917, "--------��Ʒ--------\r\n��1�� : %s\r\n ��2�� : %s"},
    {918, "%s %02dʱ %02d�� %02d��"},
    {919, "�Զ����·������������������µĿͻ��ˡ�"},
    {920, "��ð�յ���������������\r\n\r\n��������ɺ�ɫ��ʲôҲ�����ϣ�\r\n�����ڻ����ϱ��Ƶ�ʷ�Χ�����Ⱦ��棬\r\n\r\nʵ��[��ʼ����->Maplestory->Setup]\r\nͬʱ����Ƶ�ʷ�Χ�ɡ�"},
    {921, "#e ð�յ�ONLINE ����������#n\r\n\r\n#fUI/UIWindow.img/UtilDlgEx/notice#"},
    {922, "----------------- ���� -----------------"},
    {923, "/����鱨 : �鿴������ӵ������"},
    {924, "/��ӿ��� : ���µ���ӡ�"},
    {925, "/����뿪 : �����ڲμӵ�����ѳ���"},
    {926, "/������� ��ɫ�� :����μ���ӡ�(ֻ�жӳ�������"},
    {927, "/����˳� ��ɫ�� : ������˳���(ֻ�жӳ�������"},
    {928, "/�����鱨 : �鿴���ڼ����鱨"},
    {929, "/�����뿪 : �����ڲμӵļ����ȥ"},
    {930, "/�������� ��ɫ�� :���������塣(ֻ���峤������"},
    {931, "/�����˳� ��ɫ�� : �Ӽ����˳���(ֻ���峤������"},
    {932, "/���� ��ɫ�� : �鿴�ض���ɫ������״̬��λ�á�"},
    {933, "/��Ϸ���� ��ɫ�������뽻����Ϸ��Ʒ��"},
    {934, "/�ֽ𽻻� ��ɫ�������뽻���ֽ���Ʒ��"},
    {935, "/����: ����Ϊ�����ߺ��ѶԻ���״̬��"},
    {936, "/���: ����Ϊ�����߶�Ա�Ի���״̬��"},
    {937, "/����: ����Ϊ�����߼���Ա�Ի���״̬��"},
    {938, "/���Ļ� ��ɫ��������Ϊ���Ļ�״̬(ʡ�Խ�ɫ��ʱ�������������)"},
    {939, "/������: ����Ϊ�����Ļ���һ��Ի�"},
    {940, "*���������봰�ڣ���Tab������ر��������봰��"},
    {941, "��ѡ��...."},
    {942, "��"},
    {943, "Ů"},
    {1163, "����"},
    {1369, "����ѡ�����Ϸ�������϶࣬������ѡ��������������ɫ�������Ϸ"},
    {1370, "����ѡ�����Ϸ���Ѿ�����������ѡ��������������ɫ�������Ϸ"},
    {1392, "��洰���ص���"},
    {1393, "�������޷�ʹ�ù�洰�ĵط���"},
    {1661, "����"},
    {1662, "����"},
    {1663, "����"},
    {1664, "�ü�"},
    {1753, "װ��"},
    {1757, "����"},
    {1763, "����"},
    {1886, "��ʾ�������������ֵ��\n����������ƶ�λ�á�"},
    {1951, "    AP���·��䡣\r\n\r\n "},
    {2006, "�ȼ�:%d"},
    {2007, "[Ҫ��]"},
    {2008, "�ȼ�%d"},
    {2042, "���� :"},
    {2043, "���� :"},
    {2044, "���� :"},
    {2045, "���� :"},
    {2401, "�����趨"},
    {2402, "װ��(%s)"},
    {2403, "����(%s)"},
    {2404, "����ֵ(%s)"},
    {2405, "����(%s)"},
    {2495, "��%nǿս������"},
    {2496, "�ǰ������"},
    {2497, "�Ǿ�����"},
    {2505, "���ڲ�������ʱ�̡�"},
    {2506, "������Ʒ�Ѿ������ˡ�"},
    {2509, "�Բ�ս��ʤ���뵽%nǿս��"},
    {2510, "�Բ�ս��ʤ���뵽�������"},
    {2511, "�Բ�ս��ʤ���뵽������"},
    {2550, "�˿ͻ���ֻ���ڳ��������ҳ��ִ��"},
    {2551, "��ʼ��COMʧ��"},
    {2596, "[%s]�ѳɹ��Ǽǵ������ֲᡣ"},
    {2597, "���Ѿ��Ǽǵ�������Ŀ�����ȡ�Ŀ�����ʧ��"},
    {2625, "������ : %s"},
    {2626, "��������� : %d"},
    {2627, "�����۶� / �������"},
    {2639, "�ر���Ϣ����������Ϣ�������\r\n��Ҫ������"},
    {2640, "����Ϣ���ˡ�"},
    {2641, "�յ���"},
    {2642, "����Ϣ."},
    {2651, "�Է�������������Ϸ\r\n�����������Ļ����ܡ�"},
    {2652, "���������ߵ�������"},
    {2653, "�Է�����Ϣ�����������ˡ�\r\n���Ժ����ԡ�"},
    {2654, "����Է������100��(���� 50��)����Ϣ"},
    {2695, "%d��ȯ"},
    {2696, "%d %d��ȯ"},
    {2710, "����û�еط���š�"},
    {2711, "��������Ϣ�����ݡ�"},
    {2712, "���ܷ�����Ϣ���Լ���"},
    {2713, "��Ϣ�����ˡ�"},
    {2718, "��Ϊ�Է����ھܾ����Ļ���״̬������ܲ����յ��ظ���"},
    {2723, "'%s'����������������"},
    {2769, "�Ҳ���˵�����Ļ����Ķ���"},
    {2770, "��"},
    {2772, " ��"},
    {2773, "�յ�'%s'[%d��],����[%s]������"},
    {2806, "��������ɫ��"},
    {2811, "����"},
    {2812, "�ֽ����[%s]\r\n�Ѿ�����\r\n�õ����Ѿ���ʧ��"},
    {2837, "%s��%2d��%4d�� %2d:%02d %s�Ժ�\r\n�ſ��Ե�¼��Ϸ"},
    {2838, "����HP����������ʹ�øü��ܡ�"},
    {2839, "����MP����������ʹ�øü��ܡ�"},
    {2840, "���Ľ�Ҳ���������ʹ�øü��ܡ�"},
    {2841, "����%s��������������ʹ�øü��ܡ�"},
    {2842, "���ڲ��㣬�޷�ʹ�øü��ܡ�"},
    {2843, "���ĵ�ҩ�Ѿ��ù��ˣ� ʹ�ü���������߲��㡣"},
    {2844, "������װ��������������ʹ�øü��ܡ�"},
    {2845, "��ǰ����ֵ���㣬����ʹ��װ����"},
    {2846, "û��װ���������޷����й�����"},
    {2847, "ֻ��ʹ��HP�ָ����ߡ�"},
    {2848, "ֻ��ʹ��MP�ָ����ߡ�"},
    {2849, "���ļ��Ѿ��ù��ˣ�"},
    {2850, "���ķ����Ѿ��ӹ��ˣ�"},
    {2851, "�������ǹ��"},
    {2864, "�����Ʒ������ʹ�á������Դ���Ʒ�����ܡ�"},
    {2874, "��������ܺ�ð�յ�Onlineһ�����У��������������򣬸ó����״̬����ص����ݽ��ᶪʧ����Ҫ�������������\r\n\r"},
    {2875, "�����ý�ֹ��½���˻�������ʹ�á�"},
    {2881, "���۸�����"},
    {2882, "����������"},
    {2883, "���Ƴ�ʱ����Ⱥ�����"},
    {2886, "�õ���[%s]�Ѿ�������ʧ�ˡ�"},
    {2891, "���ڲ��ܽ�ͼ�����Ժ����ԡ�"},
    {2906, "��ļ��ܱ���ӡ����ʹ�á�"},
    {2907, "�㴦������״̬��������Ծ��"},
    {2908, "�㴦�ڻ���״̬�������ƶ���"},
    {2909, "�㱻�����ˣ���õľ���ֵ���١�"},
    {2910, "�㴦�ڰ���״̬�������ʼ����ˡ�"},
    {2911, "���Ѿ��ж��ˣ�HP�𽥼��١�"},
    {2923, "�������ǰ�ƶ�����ĵط����Ͳ��ܿ�����̡�"},
    {2924, "�����Է��������\r\n�޷����ף�"},
    {2925, "����Ʒ�ļ۸�̫��������"},
    {2949, "�����Ҳ���%s��ҵ�λ�á�\r\n������˲���ƶ���"},
    {2950, "����˲���ƶ��ĵ�ͼ��"},
    {2951, "�㲻���趨�Լ�Ϊ˲���ƶ��Ķ���"},
    {2952, "���ܵ�¼�ĵ�ͼ��"},
    {2953, "��ĵ�ͼĿ¼�Ѿ����ˡ�����ɾ����ǰ�ĵ�ͼ����"},
    {2954, "�Ѿ���¼�ĵ�ͼ"},
    {2955, "������λ�õĵ�ͼ��"},
    {2956, "7��������Ҳ����뿪�ʺ絺"},
    {2957, "��Ҫ�������ͼ����\r\n�ڵ�ͼĿ¼��\r\n[%s]"},
    {2958, "���ڵ�ͼĿ¼\r\nɾ������ĵ�ͼ��\r\n[%s]"},
    {2959, "��ȷ��Ҫ�ƶ������µĵ�ͼ��\r\n[%s]"},
    {2964, "���Ѿ������ֵ���\r\n���ܹ���"},
    {2965, "�޷��ƶ�����Ʒ��"},
    {2966, "����1��ԭ�ظ��������ڵ�ǰ��ͼ�����ˡ�(ʣ��%d ��/%d Сʱ)"},
    {2967, "����һ��%s���ߣ�����ֵ���䡣"},
    {2968, "�ɿ�������"},
    {2976, "        ���в�����"},
    {2977, "%d��"},
    {2983, "���ܼ�ȡ����Ʒ��"},
    {2984, "����ȥ����"},
    {2985, "�޷�����˲���ƶ��ĵ�����"},
    {2986, "��Ϊ�е����赲���޷��ƶ����ƶ����ͽ��Ĵ�ׯ��"},
    {2996, "��������Ҫ�ٱ��Ľ�ɫ����"},
    {2997, "������Ϣ�ɹ���"},
    {2998, "�ڿ�"},
    {2999, "�һ�"},
    {3000, "թƭ"},
    {3001, "ð��GM"},
    {3002, "ɧ��"},
    {3003, "��ɹ��ٱ��˸ý�ɫ��"},
    {3004, "�Ҳ����ý�ɫ��"},
    {3005, "ÿ��ÿ��ֻ�ܾٱ�10�Ρ�"},
    {3006, "�㱻�ٱ��ˡ�"},
    {3007, "��δ֪�Ĵ����޷��ٱ���"},
    {3012, "�Բ�������׼��MapleStory�̳�"},
    {3029, "#e��ð�յ��������û�ʹ�����Э��#n\r\n \r\n��#e�������û�ʹ�����Э�顷#n�����³ơ�Э�顷���������û�������������û���֪�������û����򡷡�����Ȩ�����������û������˻�һʵ�壬���»�ơ���������#e�Ϻ������Ƽ����޹�˾#n�������¼�ơ�ʢ����Ϸ����֮���йر�������Ϸ�����Ʒ\r\n����������Ϸ#e��ð�յ���#n��Ӣ����#e��maplestory��#n�����¼�ơ������Ʒ����ʹ�õķ���Э�顣�Ϻ�ʢ��ͨ�����������޹�˾Ӧ�õ�ȫ��ͨ���ߵ����������ƽ̨��ͬ�ṩ#e��ð�յ���#n�����߷���\r\n���������Ʒ���������������������ܰ�������������������վ��������������#e��Ϸ�ٷ���վ��http://mxd.sdo.com/#n��������ý�塢ӡˢ���Ϻ͡�������������ĵ�����һ����װ�����ơ�������վ����ֵ�����пͻ����������������ʽʹ�á������Ʒ����\r\n "},
    {3030, "����ʾ��ͬ����ܱ���Э�顷���������Լ����������ͬ�Ȿ��Э�顷�е�����벻Ҫ�������κ�һ�ַ�ʽʹ�á������Ʒ����\r\n \r\nʢ����Ϸ�ڴ��ر������������Ķ���Э���ȫ������ر������������������ʢ����Ϸ���ε�������������õ�����ͨ�����С������κ����Ρ����������񡱵ȴʻ㣩�Լ��û������صġ��û�������������û���֪�������û����򡷡�����Ȩ�������е������\r\n���������û�Ȩ��������õ�����ͨ�����С����á��ȴʻ㣩����Щ����Ӧ���й�����������ķ�Χ�����̶ȵ����á������û����ܱ�Э���ȫ�����������Ȩ��װ�����ơ�������վ����ֵ�����пͻ����������������ʽʹ�á������Ʒ����\r\n \r\n "},
    {3031, "��Э�飨�������û�������������û���֪�������û����򡷡�����Ȩ���������µ��������ʢ����Ϸ��ʱ������û��붨�����ı�Э�顣Э������һ�������䶯��ʢ����Ϸ������ʢ����Ϸ��ص�ҳ������ʾ������ݡ�������Э��һ������ص�ҳ���Ϲ�������Ч����\r\nԭ����Э�顣���û���ͬ��ʢ����Ϸ�Ա�Э����������κα�����û�Ӧ����ֹͣʹ��ʢ����Ϸ��������Ϸ�����û��ڱ�Э���������ʹ��ʢ����Ϸ��������Ϸ���������û�����ȫͬ�������Э�顣\r\n \r\nʢ����Ϸ�ر�������δ������Ӧ�ڷ����໤�˵���ͬ�����ĺͽ��ܱ�Э�顣����δ��ʮ�������δ�����ˣ��������䷨���໤���Է����໤�˵���������ע�ᡣδ�������û�Ӧ���ں���̶���ʹ�á������Ʒ��������������Ϸ��������ʹ�á������Ʒ��������������Ϸ\r\n��Ӱ�����ճ���ѧϰ����û����ʢ����Ϸ������Ա���ǰ����������κ���ʽ������ȷ�ϡ�\r\n \r\n "},
    {3032, "1 ���Ȩ�������衣����Э�顷����������Ȩ����#e��ð�յ���#n������Ϸ�ͻ�������İ�װ�����С������������Ч��ʱ���ڽ�#e��ð�յ���#n������Ϸ�ͻ��������װ���Լ�ʹ�õ�����������ϣ����Կͻ������ָ���ķ�ʽ���б��������Ʒ����һ�ݸ�����\r\n�����κ���ʽ��δ����ɵİ�װ��ʹ�á����ʡ���ʾ�������Լ�ת�ã���������Ϊ�ԡ�Э�顷���ַ���\r\n \r\n2 ������ͬ�涨������Ȩ�����������ͨ���ٷ���վ��ɵķ�ʽ��ȡ����Ȩ����Ʒ�Ŀͻ��������һ̨�������ʹ�á���Ʒ����������˵��Ի����ô�ȡװ�õķ�ʽʹ�á�����Ʒ����һ�����˵�����ת����һ�����˵���ʹ�ã�������ͬʱ���������������С�\r\n������ͬ��9����ת������ʱ��ʹ����������˵��ԵĲ���Ӧ����ɾ������������ʹ�á�\r\n \r\n3 ��ֹ����Ϊ����\r\n \r\n "},
    {3033, "3.1 ��ֹ�û����������ֺ���������Ϸ��ƽ�Ե���Ϊ�������������ڣ�\r\n3.1.1 ���÷��򹤳̡����������롢�����ȼ����ֶ������������Ϸ���з������޸ġ����������մﵽ���׵�Ŀ�ģ�\r\n3.1.2 ʹ���κ���ҳ������Ϸ�޸ĳ��򣨱�Э�����ơ���ҳ�����ָ��������Ϸ���֮��ģ��ܹ�����Ϸ���е�ͬʱӰ����Ϸ���������г��򣬰�����������ģ��������������ı�����������޸����ݵ�һ�����͡�\r\n������йܷ��ɡ����漰�������ܲ��ŵĹ��»�淶���ļ��涨����Ҷ����뱾Э���г�ͻ��\r\n���Է��ɡ����桢���Ź��»�淶���ļ��涨��Ϊ׼�����Ա�������Ϸ������л�ԭ���̡����롢������޸ģ��������������޸ı������ʹ�õ��κ�ר��ͨѶЭ�顢�Զ�̬�����ȡ�ڴ棨RAM�������Ͻ����޸Ļ�������\r\n "},
    {3034, "3.1.3 ʹ���쳣�ķ�����¼��Ϸ��ʹ�������������������������˳�ʽ�ȶ����ƻ�������ʩ���������������������Ϊ��\r\n3.1.4 ������������ʹ����ҡ����������������������������׳��򣬻���֯����������ʹ�ô���������򣬻����۴�����������Ϊ˽�˻���֯ıȡ�������棻\r\n3.1.5 ʹ���κη�ʽ�򷽷�����ͼ�����ṩ��Ϸ�������ط�������·�������������Լ������豸���Դﵽ�Ƿ���û��޸�δ����Ȩ���������ϡ�Ӱ��������Ϸ�����Լ�����Σ����Ŀ�ĵ��κ���Ϊ��\r\n3.1.6 ����������Ϸϵͳ���ܴ��ڵļ���ȱ�ݻ�©�����Ը�����ʽΪ�Լ�������Ĳ���������������ڸ�����Ϸ�е�������Ʒ�ȣ���\r\n3.2 һ��ʢ����Ϸͨ���ڲ��ļ������ֻ������û��ٱ����������п������ڴ���������Ϊ����ʢ����Ϸ��Ȩ��ȡ��Ӧ�Ĵ�ʩ�����ֲ����ô�ʩ�������������������˺ŵĵ�½������������Ϸ�еĻ��\r\n "},
    {3035, "ɾ���븴���йص���Ʒ���������Ƴ���������Ʒ�Ͳ��븴�Ƶ�������Ʒ����ɾ�������˺ź�Ҫ�����⳥��������������Ϊ����ʢ����Ϸ��ɵ���ʧ�ȡ�\r\n \r\n4 �Է��򹤳�(ReverseEngineering)���������(Decompilation)�������(Disassembly)�Ľ�ֹ�������öԱ��������Ʒ�����з��򹤳�(ReverseEngineering)���������(Decompile)�򷴻��(Disassemble)������ͬ�������ʹ���÷������������֮Ȩ����\r\n \r\n5 ���������ȷ�Ϻͽ��ɡ����������Ʒ������ӪȨ��ʢ����Ϸ��ʢ����Ϸ�ṩ�ķ�����ȫ�����䷢�����³̡���������Ͳ��������ϸ�ִ�С���Ӧ������ʢ����Ϸ�ƶ�����������涨�����������Υ���������涨����Ϊ��\r\nʢ����Ϸ����ֹ����Э�顷��ֹͣ��ʹ�á������Ʒ����Ȩ�������������������������������١������Ʒ�������и�������������ɲ��֡�\r\n\r\n6 �û������ṩ���豸�����ṩ����Ϣ��ʢ����Ϸ�ṩ��ʹ�õġ������Ʒ�����������Լ�������ϵͳͨ�����ʻ������磨Inte\r\net��Ϊ�û��ṩ����ͬʱ���û����룺\r\n "},
    {3036, "(1)�����䱸�����������豸���������˵��ԡ����ƽ�����������ر�����װ�á�\r\n(2)���и�������������֧������˷����йصĵ绰���á�������á�\r\n����ʢ����Ϸ�ṩ�������Ҫ�ԣ��û�Ӧͬ�⣺\r\n \r\n(1)�ṩ�꾡��׼ȷ�ĸ������ϡ�\r\n(2)���ϸ���ע�����ϣ����ϼ�ʱ���꾡��׼ȷ��Ҫ��\r\n(3)�μ�����д��ע�����ϡ���ʷ��Ϣ��\r\nʢ����Ϸ��Ϊ���ṩ��ؿͻ������ǰ�������ܱ��������˺ŵ������ˣ��������Ҫ���ṩ�����Ϣ��������������ע����Ϣ����ʷ����ȣ�������û�û���μ��Լ���д��ע�����ϼ������ʷ��Ϣ��δ��ʱ�������ע�����ϣ�\r\n����������⣨�����������������һصȣ����ò���������Դ�ʢ����Ϸ���е��κ����Ρ�\r\n \r\n "},
    {3037, "7 �ܾ��ṩ�������û����˶���������ʹ�óе����ա�ʢ����Ϸ���������˲����κ����͵ĵ�������������ȷ�Ļ������ģ�\r\n(1)��Э�����µġ������Ʒ����ʢ����Ϸ�ṩ����ط��񽫷����û���Ҫ��\r\n(2)��Э�����µġ������Ʒ����ʢ����Ϸ�ṩ����ط��񽫲��ܲ��ɿ�����������������ڿ͹�����ϵͳ���ȶ����û�����λ�á��û��ػ������Ų���ԭ�������κ����硢������ͨ����·��������Ϊ���ص�Ӱ�죻\r\n(3)��װ�����ơ�������վ����ֵ�����пͻ����������������ʽʹ�á������Ʒ����/�����ʢ����Ϸ�ṩ����ط������κ���������������κγ�ͻ��\r\n(4)ͨ��ʢ����Ϸ��վ����Ϸ�ٷ���վ��������������ϵ����Ӻͱ�ǩ��ָ��ĵ���������ҵ���������ṩ�����������\r\n \r\n8 ������Ρ�ʢ����Ϸ���κ�ֱ�ӡ���ӡ�żȻ�����⼰������𺦲������Σ���Щ�𺦿������ԣ�������ʹ��������񣬷Ƿ�ʹ�����������û����͵���Ϣ�����䶯�ȷ��档�û���ȷͬ���䰲װ�����ơ�������վ����ֵ�����пͻ����������������ʽʹ�á������Ʒ����/��\r\n����ʢ����Ϸ�ṩ����ط��������ڵķ��ս���ȫ�����Լ��е���\r\n \r\n "},
    {3038, "���䰲װ�����ơ�������վ����ֵ�����пͻ����������������ʽʹ�á������Ʒ����/�����ʢ����Ϸ�ṩ����ط����������һ�к��Ҳ�����Լ��е���ʢ����Ϸ���û����е��κ����Ρ�\r\n \r\n9 ʹ���߿�����ת����Ʒ������֮Ȩ�����������û����˺š����뼰������Ϸ�е�������Ʒ�ȣ������ˣ�������תʱӦ��ͬ������Ʒʹ���ֲᡢ��Ȩ��ͬ��һ����ת������������һ��ʹ�á��Ҹ������˽��ܱ���Ʒ������֮Ȩ��ʱ����ͬ�������ر�Э��\r\n���������û�������������û���֪������������򡷡�����Ȩ����������ȫ�����\r\n \r\n10 ������ͬ�������涨�⣬δ��ʢ����Ϸ����ͬ�⣬ʹ�����ϸ��ֹ��������Ϊ���������г��Ļ����޳��ģ���\r\n \r\n10.1 ���ơ��������������������ϳ��б���Ʒ�ĳ���ʹ���ֲ������ͼ���������ϵ�ȫ���򲿷����ݡ�\r\n \r\n "},
    {3039, "10.2 ����չʾ�Ͳ��ű���Ʒ��ȫ���򲿷����ݡ�\r\n \r\n10.3 ���Ȿ��Ʒ�����ˡ�\r\n \r\n10.4 �Ա���Ʒ�ĳ���ͼ�񡢶��������ֽ��л�ԭ�������롢����ࡢ����������͸ı���κ��޸���Ϊ��\r\n \r\n10.5 �޸Ļ��ڸǱ���Ʒ����ͼ�񡢶�������װ���ֲ�������ϵĲ�Ʒ���ơ���˾��־����Ȩ��Ϣ�����ݡ�\r\n \r\n10.6 �Ա���Ʒ��ΪӪҵʹ�á�\r\n \r\n "},
    {3040, "10.7 ����Υ������Ȩ������������������������ط������Ϊ��\r\n \r\nһ���û���ʵʩΥ���������ݵ���Ϊ������Ʒ����ȨЭ�齫����ֹͣ��ʢ����Ϸ��Ȩͨ�����ֺϷ�;������Υ�������������ʢ����Ϸ����𺦵��û�׷���������Ρ�\r\n \r\n11 ΥԼ�⳥\r\n \r\n11.1 �û�ͬ�Ᵽ�Ϻ�ά��ʢ����Ϸ�������û������棬�����û�Υ���йط��ɡ������Э�����µ��κ��������ʢ����Ϸ����𺦣��û�ͬ��е��ɴ���ɵ����⳥���Σ��õ����ΰ����������ڸ�ʢ����Ϸ��ɵ��κ�ֱ�ӻ�����ʧ��\r\n \r\n11.2 ���û�Υ���йط��ɡ������Э�����µ��κ�������κε�������ʢ����Ϸ�����κ����⡢Ҫ�����ʧ�ģ��û�ͬ���⳥ʢ����Ϸ�ɴ˲������κ�ֱ�ӻ�����ʧ��\r\n \r\n "},
    {3041, "12 �������ú�������\r\n \r\n12.1 ��Э��Ķ��������С����ͼ�����Ľ����Ӧ�����й����ɡ�\r\n \r\n12.2 ��˫���ͱ�Э�����ݻ���ִ�з����κ����飬˫��Ӧ�����Ѻ�Э�̽����Э�̲���ʱ���κ�һ����Ӧ��ʢ����Ϸ���ڵص�����Ժ�������ϡ�\r\n \r\n13 ֪ͨ���ʹ��Э������ʢ����Ϸ������֪ͨ����ͨ����Ҫҳ�湫�桢�����ʼ��򳣹���ż����͵ȷ�ʽ���У��õ�֪ͨ�ڷ���֮����Ϊ���ʹ��ռ��ˡ�\r\n \r\n "},
    {3042, "14 �����涨\r\n \r\n14.1 ���������Ʒ��������Ȩ������������Ȩ��Լ������֪ʶ��Ȩ������Լ�ı��������������Ʒ��ֻ����ڸ�����Χ��ʱ����ʹ�ã�����������ԭ����������κ����֪ʶ��ȨȨ����\r\n \r\n14.2 ���ڱ��������Ʒ���ġ��û�������������û���֪������������򡷡�����Ȩ���������ļ������ڱ���Э�顷���ɷָ��һ���֣��뱾��Э�顷ͬ����Ч��\r\n \r\n "},
    {3043, "14.3 ��Э�鹹��˫���Ա�Э��֮Լ����������й����˵�����Э�飬����Э��涨��֮�⣬δ���豾Э���������Ȩ����\r\n \r\n14.4 �籾Э���е��κ��������������ԭ����ȫ�򲿷���Ч�򲻾���ִ�������ڴ�����£�����Ч�򲻾���ִ�����Ĳ��ֽ�����ӽ�ԭ������ͼ��һ����Ч����ִ�е���������������ұ�Э�������������Ӧ��Ч������Լ������\r\n \r\n "},
    {3044, "14.5 ��Э���еı����Ϊ������裬����Ա�Э��������������������ã�Ҳ�������κη���Ч����\r\n \r\n#e�Ϻ������Ƽ����޹�˾#n\r\n \r\n "},
    {3045, "������������\r\n \r\n "},
    {3062, "��Ҭ�ӱ��������ʼ��"},
    {3063, "��Ҭ�ӱ�������Ѿ������ˡ��㽫���͵���һ����ͼ�������Եȡ�"},
    {3065, "�ܷ���:%d �ʺ�:%d ����:%d"},
    {3066, "���ӵ�������ͬ��������2���ӵ�ʱ��"},
    {3067, "2���Ӻ�ƽ�ֵĻ��������Ӷ�������Ϊ�䣬��û�н�Ʒ��"},
    {3068, "���ʺ硻��͡����ء���ķ���ͳһ���ⳡ����ƽ����."},
    {3069, "���ʺ硻��Ӯ��"},
    {3070, "�����ء���Ӯ��"},
    {3071, "OX�ʴ��У��� %d ��Ҵ���ˣ����ٽ�������"},
    {3072, "��ѩ��������Ѿ��������㽫���ƶ�����ĵط������Եȡ�"},
    {3081, "��"},
    {3082, "����Ա����"},
    {3109, "��������Ҫ���ԵĽ�ɫ����"},
    {3110, "�㲻�����Լ�������"},
    {3111, "������������72�ַ���"},
    {3112, "�����������Ե����ݡ�"},
    {3113, "���������"},
    {3114, "�����̵�\r\n����ɹ���"},
    {3115, "����ͨ�������̵��ͳ������"},
    {3116, "��������������˵�ġ�"},
    {3117, "ԭ�ۣ�%d ���"},
    {3118, "���ۣ�"},
    {3138, "%04d��%02d��%02d��%02dʱ%02d��ǰ����"},
    {3140, "��ã�����ð�յ�GM"},
    {3141, "�����ڲ��ҷǷ��Զ����Գ����ʹ���ߡ���ֹͣ���ԣ�������GM��ָʾȥ��"},
    {3142, "���������GM��ָʾ���㽫�ᱻ�����Ƿ��Զ����Գ���ʹ���ߣ����ܵ���Ӧ�Ĵ���"},
    {3143, "����ʺ�����Ϊ�Ƿ��Զ����Գ���ʹ���ߴ���"},
    {3144, "��ϸ��������ѯ�ͷ�����"},
    {3145, "ף����MapleStory�ȹ����õ�һ��"},
    {3158, "�Ҳ����ý�ɫ"},
    {3159, "�Բ����й����Ľ�ɫ����ʹ��"},
    {3160, "�Ѿ���ʹ�ù�����ǵĽ�ɫ"},
    {3161, "��������ڲ�ѯ�Ľ�ɫ"},
    {3162, "лл��İ�������û��ʹ�÷Ƿ����������Ѹ�����5000��ҽ�����"},
    {3163, "ʹ�ò���Ǻ�����ʹ�ù��Ƿ������ٴα�������ᱻ����˺š�"},
    {3165, "�װ���%s������MapleStory GM�������ڲ��ҷǷ��Զ����Գ����ʹ���ߡ���ֹͣ���ԣ�������GM��ָʾȥ�������û��Ӧ���ر�Ӧ���㽫�ᱻ�����Ƿ��Զ����Գ���ʹ���ߣ����ܵ���Ӧ�Ĵ���"},
    {3166, "����MapleStory GM��%s�����ѱ���Ӫ������ʹ�á���鿴�Ϸ�֪ͨ"},
    {3167, "��%s���ʹ���˲���ǡ�"},
    {3168, "%s_��ͼ�ѱ��档��֪ͨ���ƺ��ʹ�á�"},
    {3169, "%s_��ͼ�ѱ��档������ѷ�����"},
    {3170, "%s_��ͨ���˲���ǵĲ��ԡ�"},
    {3171, "%s_��ͼ�ѱ��档̽�ⷢ��ʹ�÷Ƿ�����"},
    {3172, "��ʹ�ò����ʱ�޷�������"},
    {3173, "�޷�ʹ�ò���ǵĵ�����"},
    {3176, "�ȼ����ͣ�����޷���������ѡ�ĵ��ߡ�"},
    {3177, "�ȼ�̫�ߣ�����޷���������ѡ�ĵ��ߡ�"},
    {3178, "����Ǽ��Ľ�����ָý�ɫʹ�ù��Ƿ����������˶Է����н���е�7000��ҡ�"},
    {3179, "�ɹ�ͨ������ǵĲ��ԣ�лл��ϡ�ף��ð����죡��"},
    {3180, "������Ӫ��Ա�ĺ˲飬�㱻�϶�Ϊ�Ƿ��Զ����Գ���ʹ���ߣ������ܵ���Ӧ�Ĵ�����"},
    {3181, "�ſڸ�������ʹ��ʱ���ż���"},
    {3202, "\r\n#b#L%d# #i%07d# %s %d��#l"},
    {3204, "#L%d##v%d:# #t%d:# %d��#l\rn"},
    {3206, "#v%d:# #t%d:# %d��\rn"},
    {3208, "#fUI/UIWindow.img/QuestIcon/7/0# %d ���\rn"},
    {3209, "#fUI/UIWindow.img/QuestIcon/8/0# %d ����\rn"},
    {3237, "û������"},
    {3238, "�ȼ� %d����"},
    {3239, "%s�ȼ� %d����"},
    {3240, "ȫְҵ������"},
    {3241, "ȫְҵ��ֻ�����ֲ�����"},
    {3242, "ȫְҵ��ֻ��%s������"},
    {3243, "��%dת"},
    {3244, "1תְҵ"},
    {3245, "2תְҵ"},
    {3246, "3תְҵ"},
    {3247, "4תְҵ"},
    {3248, "%dת����"},
    {3249, "1ת����"},
    {3250, "2ת����"},
    {3251, "3ת����"},
    {3252, "4ת����"},
    {3253, "%d�� %d�� %d�� %02dʱ ���"},
    {3257, "��δ֪����\r\n����ִ��ʧ�ܡ�\r\n��ȷ��ִ��������"},
    {3258, "�����������"},
    {3259, "%s���Ŀռ䲻��"},
    {3260, " ��"},
    {3263, "�峤"},
    {3264, "���峤"},
    {3265, "��Ա"},
    {3268, "%dʱ"},
    {3269, "%d��"},
    {3270, "���ɶ�����ʱ��ʣ�� %s%s"},
    {3271, "���ɶ�����ʱ��ʣ�� %s%s��(Ŀǰ��Ϸʹ���� %d��) ��ʣ��ʱ��������ϣ�����Ϸ��������"},
    {3275, "�˻�Ա��ֻ�����¹���\r\n�ֽ�����û�ʹ�á�"},
    {3276, "�á�������Ҫ��һ������������Ƿ�ͬ�⿪����塣�����˶�ͬ���˲��ܿ�����塣���Ե�һ����ɡ�"},
    {3277, "���˲�ͬ�⣬�����Ժ������ɡ�����Ҫ�����˶�ͬ�⣬�ſ��Կ�����塣"},
    {3278, "�������Ѿ��б�ļ���ʹ�á��������������������֡�"},
    {3279, "��ϲ��%s ����Ǽǳ����ˡ�ף���Ǽ��巢չ˳����"},
    {3280, "�����Ѿ���ɢ�ˡ�������������ٿ�����壬��ʱ��ӭ�����ҡ�"},
    {3281, "��ϲ~ ��ļ��������������ӵ�%d���ˡ�ף�����Ժ�չ˳����"},
    {3282, "ѯ���г����쳣�������¿�ʼ��"},
    {3283, "�����������г����쳣�������¿�ʼ��"},
    {3284, "��ɢ����ʱ�����쳣�������¿�ʼ��"},
    {3285, "���Ӽ�����������ʱ�����쳣�������¿�ʼ��"},
    {3286, "�õ����Ѿ��й������֡�"},
    {3287, "��Ը����%s�Ͽ������������?"},
    {3295, "[%s]��ҵĵ�λ���Ϊ[%s]�ˡ�"},
    {3296, "������ˡ�"},
    {3299, "��������Ҫ����ĵ�λ����"},
    {3300, "���ڱ������λ�͵ĵ�λ\r\n�Ͳ���ɾ����"},
    {3301, "�������λ�Ľ�ɫ��\r\n���޷�ɾ����"},
    {3302, "�����޸ĵ�������"},
    {3315, "      ����"},
    {3316, "      ֲ��"},
    {3317, "      ����"},
    {3318, "      ����"},
    {3319, "      ����"},
    {3339, "��ȷ����Ϊ�����־��"},
    {3340, "�������"},
    {3343, "�����޷������Ƶ�������Ժ��ٳ��ԡ�"},
    {3344, "�����޷�����MapleStory�̳ǡ����Ժ��ٳ��ԡ�"},
    {3347, "[%s]���񳬹�����ʱ�䣬�Ѵ�Ŀ¼��ɾ����"},
    {3348, "* ���幫��"},
    {3349, "* ���幫�� : %s"},
    {3350, "����������幫�����"},
    {3366, "����յ�������"},
    {3367, "��ҵ�����������1��"},
    {3374, "����/��������"},
    {3375, "��թ"},
    {3376, "�ֽ���"},
    {3377, "�ѳ�GM"},
    {3378, "й©��������"},
    {3379, "ʹ�÷Ƿ���ʽ���"},
    {3380, "���ѳɹ�ע�ᡣ"},
    {3384, "�㱻����ˣ�"},
    {3385, "���ϵͳĿǰά���У�"},
    {3386, "���Ժ��������ԡ�"},
    {3387, "��ȷ�Ͻ�ɫ���ƺ��������ԡ�"},
    {3388, "��������Ҳ��㣡"},
    {3389, "�޷����ӷ�������"},
    {3390, "���ѳ����ɼ�ٴ�����"},
    {3391, "�޿ɼ�ٵĽ�ɫ��"},
    {3392, "��Ŀǰ�������ڼ䣬\r\n�����������ȸ�֪������£���ֹ�������"},
    {3393, "%04d�� %d�� %d���Ժ���\r\n�ɽ��жԻ���"},
    {3394, "������״��˵����"},
    {3395, "ֻ��%d����%d��ɽ��м�١�"},
    {3396, "�ý�ɫ����ִ���κζ������޷���ٸý�ɫ��"},
    {3397, "�ܸ�Ĺ�ϵ�ٷ��Ѿ��ͷ�����\r\n�޷���档"},
    {3398, "��ѡ��"},
    {3399, "��ѡ��Ҫ�ٱ��Ľ�ɫ��"},
    {3413, "%s�ӵ�ѩ��ͨ����%d����"},
    {3414, "��%s�ӹ���ѩ�ˣ���ʹ%s�ӵ�ѩ��ֹͣ��ת��"},
    {3415, "%s�ӵ�ѩ��������ת��"},
    {3416, "�ѷ�ӡ�ĵ����޷�����������������"},
    {3417, "�õ����޷���ӡ��"},
    {3418, "����ѡ����Ҫ��ӡ�ĵ��ߡ�"},
    {3419, "����ӡ�õ��ߣ����޷��������ġ�"},
    {3420, "%s\r\nȷ��Ҫ��ӡ�õ�����"},
    {3425, "����Ʒ�޷�������"},
    {3426, "��ѡ����Է�������Ʒ��"},
    {3427, "����Ҫ��װ����1��,������3��,\r\n������1���ռ䡣\r\n��Ҫ����������ĵ�ô?"},
    {3431, "�嵥��û�пյ���λ��\r\n���Ժ����ԡ�"},
    {3432, "�ӷ�����ĵ�������ػ���˵��ߡ�"},
    {3433, "������Ҫ���ĵ�HPҩˮ���㣡"},
    {3434, "������Ҫ���ĵ�MPҩˮ���㣡"},
    {3435, "��ϵͳ�趨�е�HP�����־��˸ʱ��������ҩˮ��"},
    {3436, "��ϵͳ�趨�е�MP�����־��˸ʱ��������ҩˮ��"},
    {3437, "��Ҫ�Ƚ����ӡ��"},
    {3438, "Ϊ���������������������ߵķ�ӡ��"},
    {3441, "����ɹ���\r\n�������%d��ĵ���ȯ��"},
    {3442, "�������'%s'\r\n [%s]\r\n %d����\r\n����˵���ȯ%d�㡣"},
    {3444, "%d��"},
    {3445, "������װ���Ÿõ��ߣ��޷����գ�"},
    {3446, "�õ����޷�ͬʱӵ��������"},
    {3451, "��������"},
    {3460, "%s�Ĺ�������"},
    {3462, "%s�Ĺ������� : %s"},
    {3463, "�ѳ���Ӫҵʱ����ر��̵꣡"},
    {3464, "���ĸ����̵걻GM�رա�\r\n����ȥ������������ջ����ĵ��ߡ�"},
    {3465, "��Ӷ�̵�δ�򿪣��޷�ʹ�á�"},
    {3466, "�����ı������޿ռ�����ɸ����̵�ĵ��ߣ�\r\n�����������̵������NPC����������ȡ�ء�\r\n��ȷ��Ҫ�ر��̵���"},
    {3467, "�������޷�������Ʒ��\r\n��Ҫ��ʼ�̵������"},
    {3468, "�̵���������������Ʒ��\r\n���Ժ��ٶȹ��٣�"},
    {3469, "��ͼ���ƶ���Զ�̹���\r\n�жϡ����Ժ�����ʹ�á�"},
    {3470, "�̵���� / %02d : %02d"},
    {3471, "�����ڵ�%sƵ�������г�%d�ڿ����̵꣬\r\n���Ƚ����̵�رպ�������ʹ�á�"},
    {3472, "%sƵ���������̵ꡣ\r\n����Ҫ�ƶ�����Ƶ����"},
    {3473, "Ŀǰ��������ɫ����ʹ���С�\r\n���Ըý�ɫ�����ر��̵꣬\r\n���̵�ֿ���ա�"},
    {3474, "Ŀǰ�޷������̵꣡"},
    {3475, "���͵㸽���޷������̵꣡"},
    {3476, "���������г���ڴ��ĸ�������\r\n��ȡ��Ʒ���������ԡ�"},
    {3477, "���۳�����Ʒ���嵥��ɾ����\r\n����ȡ�۳���"},
    {3478, "���߻����ѳɹ�ȡ�ء� "},
    {3479, "�̵��ڵĽ�ҹ��࣬\r\nδ����ȡ�������ߡ�\r\n����ϵ�����г���ڵ�\r\n�������"},
    {3480, "����ȡ��ң����򲿷ֵ������������ƣ�\r\nδ��ȡ�ص��ߣ�\r\n��Ǣ�����г���ڴ��ĸ������"},
    {3481, "����ȡ��ң����򱳰�λ���㣬\r\nδ��ȡ�ص��ߣ�\r\n��Ǣ�����г���ڴ��ĸ������"},
    {3482, "����ȡ��ң�������ԭ��\r\nδ��ȡ�ص��ߣ�\r\n��Ǣ�����г���ڴ��ĸ������"},
    {3483, "�õ������Ƶ��ϼ�����Ϊһ����"},
    {3484, "�õ������Ƶ�ӵ������Ϊһ����"},
    {3485, "��Ҫ��%s����\r\n���Ա����"},
    {3486, "ף����ð�����"},
    {3487, "�����㲢û�п�����ȡ�ĵ��߻��ҡ�\r\n���������ȡ��δ���ڹ��������Ǳ���ȡ�ĵ��߻��ң�����������ʱ���ǵ�Ҫ�Կ����̵�Ľ�ɫ������ร�"},
    {3488, "�������õ��̵꿪����#b��%sƵ�������г�%d#k�ڣ�\r\n�����̵�رպ����κε���Ҫʱ�����������Ұɣ�"},
    {3489, "Ŀǰ�޷�ʹ�ã�\r\n���Ժ����ԣ�"},
    {3490, "�򳬹�%d�죬��֧��ȫ���%d%%����\r\n%d���Ϊ�����ѡ�\r\nȷ��Ҫ�����"},
    {3491, "ȷ��Ҫ�����"},
    {3492, "���Ѿ�ȡ���˵��߻��ҡ�"},
    {3493, "���̵�ֿ��ڵĽ����࣬\r\nδ����ȡ�������ߡ�"},
    {3494, "����������������ƣ�\r\nδ����ȡ�������ߡ�"},
    {3495, "�������Ѳ��㣬\r\nδ����ȡ�������ߡ�"},
    {3496, "�򱳰�λ���㣬\r\nδ����ȡ�������ߡ�"},
    {3497, "��ͨ������������ȡ��Ʒ��"},
    {3498, "����~���ǹ�������ίԱ��Ĵ���������������˵�ʹ�÷�����һ���̵������ͬ��ֻ��ע�⼸��ע������Ұ����������������ˣ���ȷ��һ�µ�ͬ�������ʹ�ù������˹��ܡ�\r\n\r\n��������ͬ����\r\n1) ����������£��������˻Ὣ�������߼��е��̵����к���ʧ��\r\n  - �������� #r�Ѵ���Ч����#k\r\n  -��ͬһ�ص㿪��������� #r����24Сʱ#k \r\n  - �������˳���ʱ��#r�����ռ䲻��#k(������1��)\r\n2)���ҹ���������ʧʱ����͸�������г���ڴ���#r��������#k��ȡ��������\r\n3)�ڻ��ս�������ǰ���޷���ʹ�ù������˵ķ���\r\n4)���������Ʒ���и��̵����к󣬾���#r24Сʱ#k�㿪ʼ����ÿ�����۽������Ʒԭ�� 1%��������\r\n5)�������ѳ���100%ʱ���㽫�˳乫�����̵�ַ�չίԱ��ľ���\r\n6)��������̵��ڣ������໰�벻������ʱ�������߿�����Ԥ��������±���̵����ơ�"},
    {3499, "��Ӷ�̵��۳�%s %d����"},
    {3506, "û�г�����޷��������\r\n�����ٻ����������."},
    {3507, "�������ܶ�̫�ͣ���ǰ�ٻ��ĳ���\r\n�޷��������"},
    {3509, "1ת��2ת���ܲ���"},
    {3510, "1ת���ܲ���"},
    {3511, "1ת��2ת��3ת���ܲ���"},
    {3513, "����"},
    {3528, "������ȷ�ϵ绰���룡"},
    {3529, "������ȷ�����֤���룡"},
    {3531, "������ȷ����֤���룡"},
    {3535, "�ڱ���״̬���޷����С�"},
    {3536, "����ʱ�޷����С��һ����Ϸ�ͼ�꣬���Խ������"},
    {3537, "����������ȫ����״̬��ʹ�õļ��ܡ�"},
    {3538, "ֻ���ڱ���״̬��ʹ�õļ��ܡ�"},
    {3539, "ֻ���ڴ��ս����ʱ��ʹ�õļ��ܡ�"},
    {3540, "�õ��˼���� (+%d)"},
    {3541, "��ʧ�˼���� (%d)"},
    {3543, "���Ӻ��Ѿ�������%d��Сʱ"},
    {3544, "���Ӻ��Ѿ�������%d��Сʱ����Ҫ��Ϣ��"},
    {3545, "��Ϊʣ�µ���Ա����6���ˣ����޷����м���Կ�����5��󽫽�����"},
    {3546, "��Ϊ�������뿪����Ϸ�����޷����м���Կ�����5��󽫽���"},
    {3547, "����ͨ��%sƵ���ļ���Կ���NPC���볡������˳��30�����볡��"},
    {3548, "�������´ο��Բμӵļ��塣����ȥ%sƵ���ȴ�"},
    {3549, "����һ�������ڽ��жԿ������ü����˳���%d�κ�ͻ���"},
    {3550, "����Կ���"},
    {3557, "����Կ�������:%d"},
    {3558, "�Ѿ�����"},
    {3559, "�²¿��������\r\n�Ȳ�10�����ߺ�\r\n����ӽ�33,333Ԫ�ߺ�\r\n���ͷḻ�Ľ�Ʒ��\r\n�µĵ����������㡣\r\n����ȷ�ϡ�"},
    {3560, "����ӽ�33,333Ԫ�ߺ�\r\n���õ��ḻ�Ľ�Ʒ���²¿��\r\n��Ҫ������\r\n�ܼ۸� : %d ���"},
    {3561, "��֤���봫�͵������ֻ�SMS�ϡ�"},
    {3562, "��ϲ�������Ա�̳���\r\n͸��""���͵�����ֻ���SMS""\r\n�������ػ�Ա�̳���"},
    {3563, "����������ʧ�ܡ�\r\n���Ժ����ԡ�"},
    {3564, "�绰�����������\r\n��ȷ�Ϻ����ԡ�"},
    {3565, "�ֻ�ʹ�������Ϻ�\r\n�������ϲ�һ�¡�\r\n��ȷ�Ϻ����ԡ�"},
    {3566, "��֤���̵��з�������\r\n���Ժ����ԡ�"},
    {3567, "���Ѿ������˻�Ա�̳���"},
    {3568, "��֤�����������\r\n��ȷ�Ϻ����ԡ�"},
    {3569, "���������봦��ʱ�䡣\r\n�����ԵȺ����ԡ�"},
    {3570, "������δ֪����"},
    {3571, "��ȡ����"},
    {3572, "HP�ظ�"},
    {3573, "MP�ظ�"},
    {3574, "�����ƶ���Χ"},
    {3575, "�Զ���ȡ"},
    {3576, "��ȡû������Ȩ�ĵ��ߺͽ��"},
    {3577, "�����ٻ�"},
    {3578, "����ȡ�ض����߼���"},
    {3579, " ���������ҷ����ѡ��Ը��ʹ�þ���ĳ���"},
    {3580, "��Ҫ�뿪���촰��"},
    {3597, "���޷�ʹ�ü���"},
    {3598, "װ������������״̬�ϲſ���ʹ�ü��ܡ�"},
    {3599, "�ڳ���%s%sװ���˼�ȡ���߼��ܡ�"},
    {3600, "�ڳ���%s%sװ���������ƶ���Χ���ܡ�"},
    {3601, "�ڳ���%s%sװ�����Զ���ȡ���ܡ�"},
    {3602, "�ڳ���%s%sװ���˼�ȡû������Ȩ�ĵ��ߺͽ�Ҽ��ܡ�"},
    {3603, "�ڳ���%s%sװ����HP�ָ����ܡ�"},
    {3604, "�ڳ���%s%sװ����MP�ָ����ܡ�"},
    {3605, "�ڳ���%s%sװ���˲���ȡ�ض����ߵļ���"},
    {3606, "ף����.�����ٻ�����ǿ���ɹ���."},
    {3607, "��ϲ��ϲ.�����������＼��ǿ���ɹ���."},
    {3608, "׷��"},
    {3609, "ɾ��"},
    {3619, "��4��תְ"},
    {3620, "�����֪(%d/5)"},
    {3629, "[%s]\r\n��Ҫ������\r\n��ȷ���������������\r\n%s\r\n�̵�����������ʧ��"},
    {3630, "��ʾ����ͼ۸�ʼ200������"},
    {3631, "��ʾ����߼۸�ʼ200������"},
    {3632, "�����"},
    {3633, "���������"},
    {3634, "һ��"},
    {3635, "����������"},
    {3636, "Ƶ����ͬ���̵�رյĻ��޷�ʹ�ÿ�ݼ�����"},
    {3637, "�Ҳ���������ĵ���"},
    {3638, "�̵�ر�"},
    {3639, "�޷��ƶ�"},
    {3640, "��%sƵ����%d�ŷ����Ͽ�����̵�"},
    {3641, "�������г��ڲſ���ʹ��"},
    {3642, "�޷��������г���ʹ�á�"},
    {3650, "��λ����"},
    {3651, "��λ�۸�"},
    {3652, "����������"},
    {3675, "#e#b����ʯͷ��ע������#n#k\r\n�μӼ���ʯͷ����Ϸ������ #r1000���#k��\r\n��Ϸ�ڻ��ʤ��ʱ������ȡ������ʤ����ʤ֤��������ʧ����ʤ��սʱ�޷�ȡ����ʤ֤����\r\nȡ�õ���ʤ֤�����Խ����� NPC ��������������أ����ᡣ"},
    {3676, "������ʼ������ʱ��Ϸ�Ὺʼ��"},
    {3677, "%d����ѡ�����ʯͷ������һ����"},
    {3678, "%dҪ��ʤ��սʱ����ѡ��'����'��ʧ����սʱ�޷������Ʒ��"},
    {3679, "��ϲ���ɹ���ս10��ʤ��"},
    {3680, "����ʱ���ѳ�������Ϸ�ж�Ϊ�䡣��Ҫ����սʱ����ѡ��'�ٴ���ս'������ս��Ҫ1000��ҵĲμӷ��á�"},
    {3681, "��ð�ο��500��ҡ���Ҫ����սʱ�밴'�ٴ���ս'����������ս��Ҫ1000��ҵĲμӷ��á�"},
    {3682, "��Ҫ����սʱ�밴'�ٴ���ս'����������ս��Ҫ1000��ҵĲμӷ��á�"},
    {3683, "����������λ"},
    {3684, "ȱ����Ϸ�μӷ���(1000���)��"},
    {3685, "һ��ֻ�ܲμ� %d�غϵ���Ϸ��"},
    {3686, "#k��ٻ\r#b���� 1��~7��\r#r����ֵ %d�� ���� %d��"},
    {3693, "%4d��%2d��%2d�տ�ʼ"},
    {3694, "%4d��%2d��%2d�ս���"},
    {3695, "[һ]"},
    {3696, "[��]"},
    {3697, "[��]"},
    {3698, "[��]"},
    {3699, "[��]"},
    {3700, "[��]"},
    {3701, "[��]"},
    {3702, "ֻ��������"},
    {3703, "���� %2d �㿪ʼ"},
    {3704, "���� %2d �㿪ʼ"},
    {3705, "���� %2d ��Ϊֹ"},
    {3706, "���� %2d ��Ϊֹ"},
    {3708, "Ŀǰֻʣ�� %d ��."},
    {3709, "�����ڼ��޶�����"},
    {3710, "��ץ��������"},
    {3711, "[ �޶����۽��� ]"},
    {3721, "����������"},
    {3722, "ȷ��Ҫɾ����"},
    {3723, "��ɾ��Ŀǰ�Ĺ�����������ԣ�"},
    {3756, "��"},
    {3757, "��"},
    {3759, " %dʮ"},
    {3760, " %d��"},
    {3761, " %dǧ"},
    {3762, " %dǧ��"},
    {3765, "��ӼӾ���(+%d)"},
    {3766, "���ӽ�������ֵ(+%d) x%d"},
    {3767, "%d���ϴ��Ժ���ر���ֵ(+%d)"},
    {3768, "ÿ�δ�3������ʱ�����ر���ֵ%d"},
    {3775, "��������ʧ�ܣ����ƶ������ŵء�"},
    {3776, "����̩��˹ʧ��"},
    {3777, "Ұ��������"},
    {3778, "���ܵ��ջ󣬶�ʹ�ж����ޡ�"},
    {3779, "����ɲ����壬�ָ�ʱ�ܵ��˺����ָ�ҩ��Ч�����롣"},
    {3785, "���������ε���ս�����ڻ����˰���������Զ���ӣ����ǲ�����֮�ֵ�����Ӣ��~"},
    {3786, "������ƣ���������Ʒ���͵�Զ���Ӱ���������������ʱ���ʤ�ߣ�"},
    {3789, "����ֻ���˵�%d����⡣"},
    {3790, "���������ˡ��뱣���������ܵ�����,�����ܹ�ר��������⣡"},
    {3791, "̩��˹��Σ�գ����ϼӻ�����"},
    {3792, "Ұ���Σ�գ���ϸ���չˡ�"},
    {3793, "���ﲻ�ܳ���������.\r\n��ȷ��һ�¡�"},
    {3797, " Ҫʹ��%s��"},
    {3798, "%s��ʹ�á�"},
    {3799, "�Ѿ������ͬ����ɫ�������۾���\r\n����������ˡ�"},
    {3800, "û�г������ʹ�á�"},
    {3806, "ѱ���Ĺ���"},
    {3807, "����"},
    {3810, "�����װ����"},
    {3811, "�����װ��."},
    {3812, "����Ľ�ָ"},
    {3813, "��ѱ���Ĺ�������ϰ��Ӳ���ʹ�øü��ܡ�"},
    {3814, "������޷�ʹ�á�"},
    {3815, "����ѱ���Ĺ����ϲ���ʹ�á�"},
    {3817, "�޷�����ѱ���Ĺ��"},
    {3818, "��Ҫͬʱװ��ѱ�����ֽ������ֽ��ӣ��ſ���ʹ�á�"},
    {3825, "װ�����߿ո��㡣"},
    {3826, "����������������"},
    {3827, "�ɹ���ѱ���˹��"},
    {3828, "��Χû�п�ѱ���Ĺ��"},
    {3845, "������"},
    {3846, "���ܲ�"},
    {3847, "����ʹ��%s��"},
    {3848, "ʹ����%s������û��Ч����"},
    {3849, "�����ᷢ���˹�â�����������ˡ�"},
    {3850, "���ܲᷢ���˹�â���������µļ��ܡ�"},
    {3852, "[���� ����%d]"},
    {3853, "%s�ĳ���ʱ���Ѿ�����������ʧ��"},
    {3854, "%s��ʧ��"},
    {3855, "δ֪"},
    {3870, "�ڵ�ǰ��ͼ���޷����ա�\r\n�뵽�ͻ�Ա����������ȡ��"},
    {3871, "�޷��ͳ��õ���"},
    {3872, "�뵽�ͻ�NPC����ȷ�Ϻ���ȡ������"},
    {3873, "��ݷ���/���͵İ���"},
    {3874, "����"},
    {3875, "�������"},
    {3876, "�ؿ�������"},
    {3878, "[%s]���͵�\r\n����������.\r\n "},
    {3879, "\r\n��� : %d"},
    {3880, "\r\n���� : %s"},
    {3881, "������"},
    {3882, "%d��"},
    {3883, "�쵽����"},
    {3884, "%d Lv����"},
    {3885, "û�еȼ�����"},
    {3886, "û�����ݣ�"},
    {3887, "ѡ��Ҫ���յĿ�ݣ�"},
    {3888, "Ŀǰ���ڷ��͵���.\r\n���Ժ����ԣ�"},
    {3889, "���͵İ����а�����\r\n���߻��߽�һ�����ɾ����\r\nҪɾ����"},
    {3890, "�����������Ҫ���͵����ݣ�"},
    {3891, "�����룮(����50��)"},
    {3892, "��ȡ�����ߵ�½��"},
    {3893, "û���ؿ�ʹ��ȯ��"},
    {3894, "����Ҫѡ���һ��ߵ��ߣ�"},
    {3895, "�������������������"},
    {3896, "��������Ҫ��\r\n%d���.\r\n��������� [�ؿ�ʹ��ȯ] 1��.\r\nȷ��Ҫ������"},
    {3897, "���ͼ��������轻\r\n%d ���.\r\nȷ��Ҫ������"},
    {3898, "�뷢�ͼ�����"},
    {3899, "�ɹ�ɾ����"},
    {3900, "�ɹ����գ�"},
    {3901, "�ɹ����ͣ�"},
    {3902, "�µĿ���ѷ��ͣ�"},
    {3903, "���Ǵ�������룮"},
    {3904, "������ȷ�Ͻ����˵�������"},
    {3905, "���ܷ��͸�ͬһ�˺ŵĽ�ɫ��"},
    {3906, "�����˵Ŀ����������"},
    {3907, "�ý�ɫ�޷����տ�ݣ�"},
    {3908, "ֻ��ӵ��һ���ĵ����Ѿ��ڸ�\r\n��ɫ�Ŀ�����"},
    {3909, "���ֲ�������"},
    {3910, "��鿴�Ƿ��пռ䣮"},
    {3911, "��Ϊ��ֻ��ӵ��һ���ĵ���\r\n�޷��ҵ���Һ͵��ߣ�"},
    {3916, "���⣺"},
    {3917, "��ʾ��"},
    {3918, "[�����]"},
    {3919, "�����޶���ʱ���ڣ�����𰸡�"},
    {3920, "����ĺ�������Ҫ%d�֡�"},
    {3921, "����ĺ��ֲ��ܳ���%d�֡�"},
    {3922, "�ش������ڼ䲻������������Ŷ��"},
    {3923, "����ʱ�䣡����"},
    {3957, "�޷�����GMר�õĹ��档"},
    {3958, "��õ�1��Сʱ���ϡ�"},
    {3959, "�����������"},
    {3960, "�����������ߵ�������"},
    {3961, "�������ͨ��\r\nð�յ�TV%d�����Ժ��͡�\r\n ��Ҫ������"},
    {3962, "�����߲����ߡ�"},
    {3963, "����ʹ��ף��������"},
    {3964, "������ף�����ᡣ"},
    {3965, "���ᷢ��һ��������������������\r\n���ǵ���û���κα仯��\r\n������ף�����ᣬ��������Ŀ���������û�м��١�"},
    {3966, "���ᷢ��һ������, �ڵ����������ĳ�����ص�������\r\n������ף�����ᡣ"},
    {3972, "�Ⱥ�ʱ�䳬��15�롣\r\n���Ժ����ԣ�"},
    {3977, "15�������µ����ÿ��ֻ�ܽ���100���ҡ�\r\n�������Ѵﵽ���ƣ�\r\n���������ԡ�"},
    {3988, "�Ҳ����κ���ҡ�"},
    {3989, "���Ѿ����ӵ���������"},
    {3990, "δ֪���󣺲鿴���н�ɫ��"},
    {3991, "�������������͸�������\r\n[%s]"},
    {4031, "���ɽ���ɫ�����趨Ϊ"},
    {4032, "��"},
    {4033, "ֻ��"},
    {4034, "1��"},
    {4045, "�������ӳ��ø�\r\n%s��?"},
    {4046, "%s��������ӳ���"},
    {4047, "��ӳ����������, %s�����µ���ӳ���"},
    {4048, "��ת�ø�ͬһ�����ص���ӳ�Ա��"},
    {4049, "��ת�ø�ͬһ��Ƶ������ӳ�Ա��"},
    {4050, "û������ӳ�ͬһ��ͼ����ӳ�Ա��"},
    {4065, "����ʼ��˫����ɫͷ����ͼ�ꡣ"},
    {4075, "ʣ��ʱ�� %dСʱ%d��%d��"},
    {4076, "�ѹ��޶�ʱ��,[%s]����ʧ�ܣ��Ѵ�Ŀ¼��ɾ����"},
    {4077, "�������޷�ʹ�øõ��ߡ�"},
    {4078, "ð�յ����"},
    {4079, "ð�յ�����"},
    {4080, "[%s]Ŀǰ���޷�ս����״̬,����[%s]����ʧ��CP %d ."},
    {4081, "[%s]Ŀǰ���޷�ս����״̬,����û��ʣ���CP����[%s]�ӵ�CP������."},
    {4082, "CP���������޷�ִ��"},
    {4083, "�Ѿ��ٻ����Ĺ���"},
    {4084, "�㲻����ʹ���ٻ����ˡ�"},
    {4085, "��������Ѿ����ٻ��ˡ�"},
    {4086, "��Ϊ�޷���֪��ԭ��,����ʧ��."},
    {4087, "������껪ʤ����.���ϻ��ƶ��������ط������ĵȴ�."},
    {4088, "������껪ʧ����.���ϻ��ƶ��������ط������ĵȴ�."},
    {4089, "�ӳ�ս��������δ��ʤ��. ���ϻ��ƶ��������ط������ĵȴ�."},
    {4090, "�Է���������˳�������ֹ������껪. ���ϻ��ƶ��������ط������ĵȴ�."},
    {4091, "[%s]�ٻ����ٻ���. [%s]"},
    {4092, "[%s]ʹ���˼���. [%s]"},
    {4093, "[%s]�ٻ����ٻ���. [%s]"},
    {4094, "��ʼ������껪!!"},
    {4095, "ʤ��δ�����ӳ�����2����."},
    {4096, "[%s]�ӵ�[%s]�ж��˹�����껪."},
    {4097, "[%s]�ӵ���ӳ��ж��˹�����껪,����[%s]�����µ���ӳ�."},
    {4110, "��ӳ�Ա: %s"},
    {4114, "[ף��]%s���ڴﵽ��200��. ���һ��ף���°ɡ�"},
    {4125, "���������롣"},
    {4128, "�һ�ò�����ʹ�õ�CP."},
    {4129, "ʣ�� CP : %d / �ܻ�� CP : %d"},
    {4130, "�����Ƕӵ�CP״̬,�Ƚ��ܻ��CP�ж�ʤ��."},
    {4131, "��ӳ�����ʹ����ӳ�Ա��CP"},
    {4132, "�ԶԷ��ӵ�CP״̬,�Ƚ��ܻ��CP�ж�ʤ��."},
    {4133, "������ѶԻ���%s������"},
    {4134, "��������ѶԻ���%s������"},
    {4135, "%s�ĺ��ѶԻ�������"},
    {4136, "%s�ĺ��ѶԻ���������"},
    {4137, "�������˵�ĶԻ����� ""%s""�޷�������"},
    {4138, "�������˵�ĶԻ����� ""%s""���Կ�����"},
    {4139, "�޷�����""%s""�����˵�ĶԻ����ݡ�"},
    {4140, "���Կ���""%s""�����˵�ĶԻ����ݡ�"},
    {4141, "�޷�����""%s""�����˵�ĶԻ����ݡ�"},
    {4142, "���Կ���""%s""�����˵�ĶԻ����ݡ�"},
    {4143, "�޷���%s���к��ѶԻ���"},
    {4144, "���Ժ�%s���к��ѶԻ���"},
    {4150, "��������"},
    {4167, "�����Զ���ʾ���ų�,������������ǰ�޷��Զ���½��[%s]"},
    {4168, "����ʼ��,��û�е�½����ʾ���ϡ�[%s]"},
    {4171, "ð�յ�������Ҫ���㴫����"},
    {4176, "��ֹ�Զ��رա�"},
    {4177, "�����Զ��رա�"},
    {4178, "���Զ���ʾ����"},
    {4179, "�ر��Զ���ʾ����"},
    {4180, "���ʱ�����е�������Զ���½, Ҫ��10���Ӳ����еĻ�����ʧ��"},
    {4181, "���ʱ�ν����е������Զ���½��"},
    {4182, "%d���Ӳ��ٻ������Ļ�����ر������ļ�̨��"},
    {4183, "�������ر�ʱ�仹�� %d���ӣ�"},
    {4184, "�����ļ�̨�ѹرգ�"},
    {4185, "%d���Ӳ��ٻ����������Ļ�����رհ���������Ѩ��"},
    {4186, "����������Ѩ����%d���Ӻ�رա�"},
    {4187, "����������Ѩ�ѹرա�"},
    {4188, "����ʾ"},
    {4189, "ֻ��ʾ����"},
    {4190, "ֻ��ʾHP"},
    {4191, "���ֺ�HP����ʾ"},
    {4192, "�ɹ����룮"},
    {4195, "�޷�ʹ�þ���ĵ��ߣ�"},
    {4200, "���� %s����ˣ�"},
    {4201, "���� %s�����ˣ�"},
    {4204, "����������ף����"},
    {4207, "�ڵȶԷ��Ļ�Ӧ��"},
    {4216, "��½Ŀ¼"},
    {4223, "һ��ֻ�ܵ�½һ����"},
    {4231, "���Ҫ�˳���"},
    {4233, "%s���������.\r\nҪ��Ӧ��"},
    {4235, "ף���㶩�飮"},
    {4236, "ף�����飮"},
    {4237, "�Է�֣�صؾܾ���������飮"},
    {4238, "����ʧ�ܣ�"},
    {4239, "���ʧ�ܣ�"},
    {4240, "����Ϊ����Ľ�ɫ����"},
    {4241, "�Է�����ͬһ��ͼ��"},
    {4242, "�Է��ĵ�����������"},
    {4243, "ͬ�Բ��ܽ�飮"},
    {4244, "���Ѿ��Ƕ����״̬��"},
    {4245, "���Ѿ��ǽ���״̬��"},
    {4246, "�Է��Ѿ��Ƕ����״̬��"},
    {4247, "�Է��Ѿ��ǽ���״̬��"},
    {4248, "�����ڲ�������״̬��"},
    {4249, "�Է������޷���������״̬��"},
    {4250, "���ź��Է�ȡ���������������"},
    {4251, "ԤԼ���޷�ȡ��������"},
    {4252, "�ѳɹ�ȡ��ԤԼ�����Ժ����ԣ�"},
    {4253, "��������Ч��"},
    {4254, "������ԤԼ�ѳɹ����գ�"},
    {4255, "������������\r\n�������������ڣ�"},
    {4256, "������������Ľ�ɫ����"},
    {4257, "���������д����˵����֣�"},
    {4258, "�޷�����鵱����\r\n������"},
    {4259, "�����õ��ߣ�����ͻᱻȡ��.\r\n���붪���õ��ߣ�"},
    {4260, "�޷����ս���ָ.\r\nû��ϵ��"},
    {4261, "�޷�����������\r\n�����������ڣ�"},
    {4262, "ף����! ���ɹ����뵽��������ȷ���������ڣ�"},
    {4263, "%s�� %s�Ļ��罫��%dƵ������þ��У�"},
    {4264, "%s�� %s�Ļ��罫�� %dƵ������þ��У�"},
    {4265, "%s�� %s�Ļ��罫��20������%dƵ������ý��У�"},
    {4266, "����ָֻ�����һ����"},
    {4267, "[��ż]"},
    {4268, "�����޶����֣�"},
    {4269, "������ʹ�ò����ʺϣ�"},
    {4270, "���͸õ�����"},
    {4271, "�Ѿ��ͳ��˵��ߣ�"},
    {4272, "���͵���ʧ�ܣ�"},
    {4280, "Ŀǰ��û�з�ѿ��"},
    {4281, "1�׶�: ��С��ʥ������Ҷѿ��"},
    {4282, "2�׶�: ʥ������Ҷѿ��΢�����ˣ�"},
    {4283, "3�׶�: ʥ������Ҷѿ����΢�����ˣ�"},
    {4284, "4�׶�: ʥ������Ҷѿ���˺࣮ܶ"},
    {4285, "5�׶�: ʥ������Ҷѿ�����˺ܶ�࣮ܶ"},
    {4286, "6�׶�: �������Ͼ�Ҫ���ʵ�ˣ�"},
    {4287, "7�׶�: ���ڳ�����Ʒ�����ˣ�"},
    {4296, "δ��ʼ"},
    {4297, "������"},
    {4298, "�ѽ���"},
    {4299, "�����"},
    {4301, "��Ծ���޷������̵ꡣ"},
    {4302, "�Ƽ�ְҵ��"},
    {4303, "��ǿ�����������־��ֱ�����Ѳ�ͻ�Ƶ�ְҵ��"},
    {4304, "��ǿ�������������ֱ�����Ѳ�ͻ�Ƶ�ְҵ��"},
    {4305, "�����������ݵ�Ӱ�졣"},
    {4306, "�ܷõ� ��ü�� ������ ��� �÷��� �������� �¼�"},
    {4307, "�����������ݵ�Ӱ�졣"},
    {4308, "����Ǳ�ڵ�ħ����̽Ѱ�����ְҵ��"},
    {4309, "���Խ���Ҳ���ڷ���"},
    {4310, "��������������Ӱ�졣"},
    {4311, "�Ը߼�����Ϊ����������ʹ�ù������ְҵ��"},
    {4312, "�Ը߼�����Ϊ����������ʹ��Զ��������ְҵ��"},
    {4313, "�����ݺ�������Ӱ�졣"},
    {4314, "���õ� ���߷��� �������� ���Ÿ� ���⸦ ���������"},
    {4315, "�ٷ�� ����. DEX�� STR�� ������ �޴´�."},
    {4316, "���ְҵӵ����ߵ��ٶȺ�����ս��"},
    {4317, "�Կ�ݵ����֣��ݺ����£�ϲ��ð�յ�ְҵ��"},
    {4318, "�����������ݵ�Ӱ�졣"},
    {4319, "û��"},
    {4320, "���ڵ�״̬��û�к��ʵ�ְҵ�Ƽ���"},
    {4321, "������������һ������ת�����ӡ�"},
    {4323, "ת��"},
    {4324, "������Ƶ�"},
    {4325, "����ͼ����"},
    {4326, "�Ϳ��Կ���"},
    {4327, "��ϸ��˵����"},
    {4328, "Ϊ�����"},
    {4329, "���ܼ���"},
    {4330, "����"},
    {4331, "��ť��"},
    {4332, "ӵ�ж���"},
    {4333, "���ܵ�����"},
    {4334, "�Ϳ�������"},
    {4335, "���ټ��ܡ�"},
    {4336, "ע�᳣�ü��ܣ�ֻ��һ����ť���ɿ��ٷ��ʡ�"},
    {4341, "���Դ򿪻��۵��������Լ��ļ��ܴ���"},
    {4342, "���԰���Ҫ��ϵ�\r\n���ܵ����ϵ����ļ����嵥�Ǽǡ�"},
    {4343, "���漼�����ơ�"},
    {4348, "�޷�ȡ������"},
    {4349, "Ŀ���������̫ǿ���޷�����"},
    {4350, "�ٳ�ʯ��������ʹ�á�"},
    {4351, "�������̫Զ���޷�ץס��"},
    {4352, "�������̫Զ���޷�ץס��"},
    {4362, "�޷�ʹ�õĵ��ߡ�"},
    {4363, "�ܽ�����һ������\r\n�޶ȡ�"},
    {4369, "��û��������ɫ���룬�޷�������ȥ����������ֹ��"},
    {4374, "����������޷�ʹ�ô˼��ܡ�"},
    {4375, "�������޷�ʹ�á�"},
    {4376, "���ݰ��µ�Ҫ����������ʧȥЧ���Ľ���ָ��ʧ��"},
    {4377, "%4d ��"},
    {4378, "�����������˵�����Ҫ���̵깺��\r\n��������"},
    {4418, "û�п��ԶԻ��Ķ���"},
    {4421, "������Tab�Բ˵����в�����"},
    {4431, "���־���ɵ���\r\n��ӡ�ĵ����𻵡�\r\n����Ҫʹ�þ�����"},
    {4432, "�������������װ���ϼ���ʹ����"},
    {4433, "��ʾ����Ѫ��"},
    {4434, "��û�аѵ��߷������档\r\n��������Ҫʹ����"},
    {4442, "���ֲ��ܳ���20�С�"},
    {4443, "ֻ�ܷ���300��(����150��)���ڵ����֡�"},
    {4444, "����������д34��(����17��)��"},
    {4446, "����������д68��(����34��)��"},
    {4447, "%s����ܾ����������롣"},
    {4448, "%s����ܾ����������롣"},
    {4449, "������%s����������ˡ�"},
    {4450, "��δ�����κ����ˡ�"},
    {4451, "���˹���"},
    {4453, "����������������˵ļ�������\r\n��"},
    {4454, "���Ҫ�˳���"},
    {4455, "��Ҫ��[%s]����\r\n���������"},
    {4456, "���������˹������ݡ�"},
    {4457, "�ѳ���ÿ���ʺſ���ʹ�ø��Ż�ȯ��\r\n���ƴ�����\r\n��ϸ������ο��Ż�ȯ˵����"},
    {4458, "��ã�"},
    {4459, "��ְλ��������˳�Ա���Σ�\r\n�޷�ɾ����"},
    {4460, "������������ѹرա�Ŀǰ�޷�ʹ�á�"},
    {4466, "�ͷ�����Ԥ����"},
    {4467, "������û���������ݡ�\r\n����Ҫ����ô��"},
    {4493, "���"},
    {4494, "δ���"},
    {4496, "�������"},
    {4497, "��������"},
    {4501, "��չ������"},
    {4502, "�رյ�����"},
    {4504, "�������ֽ���������������"},
    {4526, "�����Ҫɾ����"},
    {4527, "���緱æ�������ӳ١�\r\n���Ժ����³��ԡ�"},
    {4528, "תְʱ����������%d���ո�\r\n��ʹ������ʧ��������Ҫ����������"},
    {4531, "��������%d�ŵ�ͼ��"},
    {4532, "û�������ĵ�ͼ��"},
    {4533, "[%d]�ƶ���%s��"},
    {4535, "%s�����%s�н������%s�����ף������"},
    {4536, "��Ҳ������%s"},
    {4537, "�޷����˵��˺��������\r\n���øý�ɫ��¼��Ȼ����"},
    {4538, "��ȷ�Ͻ�ɫ���Ƿ����"},
    {4539, "�˵��߶��Ա������ơ�\r\n��ȷ�Ͻ����˵��Ա�"},
    {4540, "����������˵ı�����������\r\n�޷��������"},
    {4541, "����������Ч���޺����ʧ�ĵ��ߡ�\r\n��ȷ��Ҫ������"},
    {4542, "��ѫ��"},
    {4543, "����ɥʧ�ƺţ�%s�����͸��µ����ˡ�"},
    {4558, "�����޷�������Ʒ��"},
    {4559, "�޷��ֽ����Ʒ��"},
    {4563, "����%s�ϣ����Խ���Чʱ���ӳ�%s����ȷ��Ҫʹ����"},
    {4564, "��Чʱ���޷����ӵ�%d�����ϡ�"},
    {4565, "%d��"},
    {4566, "%dСʱ"},
    {4567, "%d����"},
    {4568, "�޷������ƷЧ����"},
    {4569, "����װ����%s������ʱ������%d%%�ľ���ֵ������"},
    {4570, "װ��%s�󾭹���%dСʱ������ʱ������%d%%�ľ���ֵ������"},
    {4585, "�������¼Ϊͬѧ�Ľ�ɫ����������ͬһƵ����ͬһ��ͼ�У��ſ��Ե�¼��"},
    {4586, "���������Ľ�ɫ����"},
    {4587, "������ð��ѧԺ�Ŀں�"},
    {4588, "(û��ð��ѧԺ��Ա)"},
    {4589, "(û����ʦ)"},
    {4590, "(û��Զ��)"},
    {4591, "(��û��ð��ѧԺ)"},
    {4592, "[���¼ͬѧ]"},
    {4593, "%sð��ѧԺ"},
    {4594, "�ϼ�(%d��)"},
    {4595, "�¼�(%d��)"},
    {4596, "��ɫ�����ߣ����ɫ������ȷ��"},
    {4597, "�޷���¼Ϊͬѧ�Ľ�ɫ��"},
    {4598, "��ͬһð��ѧԺ��"},
    {4599, "��ͬһð��ѧԺ��"},
    {4600, "����Ľ�ɫ�����ڡ�"},
    {4601, "ֻ����ͬһ��ͼ�еĽ�ɫ����\r\n��¼Ϊͬѧ��"},
    {4602, "�Ѿ���������ɫ��ͬѧ��"},
    {4603, "ֻ�ܽ����Լ��ȼ��͵Ľ�ɫ\r\n��¼Ϊͬѧ��"},
    {4604, "�ȼ����쳬��20���޷�\r\n��¼Ϊͬѧ��"},
    {4605, "ֻ��10�����ϵĽ�ɫ�������Ϊ\r\nͬѧ��"},
    {4606, "��������������ɫ��¼Ϊͬѧ��\r\n���Ժ����³��ԡ�"},
    {4607, "������ɫ�������ٻ���\r\n���Ժ����³��ԡ�"},
    {4608, "ֻ�й�ϵͼ�п��Բ鿴�����¼�ð��ѧԺ��Ա��6����������ʱ�ſ���ʹ�á�"},
    {4609, "�ٻ�ʧ�ܡ��õ����޷��ٻ��������޷��ٻ���״̬��"},
    {4610, "�޷���¼Ϊͬѧ��ð��ѧԺ�Ĺ�ģ���²��ܳ���1000����"},
    {4611, "ͬѧ�������޷���%��¼Ϊͬѧ��"},
    {4612, "%s���ͬѧ�������޷���¼�����ͬѧ��"},
    {4613, "%s\r\nϣ����Ϊ�����ʦ��\r\n�����Ϊ%s��ͬѧ��"},
    {4614, "%s�ܾ���¼Ϊͬѧ��"},
    {4615, "�ѽ�%s��¼Ϊͬѧ��ϣ���㾡���ܵذ���ͬѧ������Ϸ��"},
    {4616, "%s��Ϊ�������ʦ������Ϸ����������Ի�ø��ֵİ�����"},
    {4617, "�Ѻ�%s����\r\nð��ѧԺ��ϵ�ѽ�����"},
    {4618, "�Ѻ�%s����\r\nð��ѧԺ��ϵ�ѽ�����"},
    {4619, "�����������(%s, +%d)"},
    {4620, "������%d�������ȡ�"},
    {4621, "%sʹ���ˡ�%s����Ȩ��"},
    {4622, "���˵������Ƚ�����٣�\r\n��������ѧԱ��Ա�����ܵ�Ӱ�졣\r\n�Ƿ����ѡ��ɫ����"},
    {4623, "���������ð��ѧԺ��Ա�����֡�"},
    {4624, "%s��%s�����ٻ��㡣��Ҫ�����ƶ���ȥ��"},
    {4625, "%s�ܾ����ٻ�����"},
    {4626, "������%s�����Ȩʹ���޶ȡ�"},
    {4627, "��������[��ǰ������/��������]����̬��ʾ��\r\n���ѵ�ǰ�����ȣ�����ʹ��[����ð�ռҵ���Ȩ]��"},
    {4628, "ͬѧ��[��ǰͬѧ����/���ͬѧ����]����̬��ʾ��\r\n�Լ���¼��ͬѧ��þ���ֵ������ʱ���Լ���������Ҳ����֮������\r\n��ͬѧ�����Ĺ������ṩ�İ���Խ�࣬�Լ��������������ľ�Խ�졣"},
    {4629, "ͬѧ��¼���пո�Ļ����Ϳ��Ե�¼ͬѧ������������������"},
    {4630, "���Ժ���ʦ�����Լ���ΪԺ��������µ�ð��ѧԺ��\r\n����¼�ð��ѧԺ��Աȫ�����Ϊ���ð��ѧԺ��Ա��"},
    {4631, "��ͬѧ����\r\nͬѧ�����¼�ð��ѧԺ��Աȫ���˳�ð��ѧԺ������µ�ð��ѧԺ��"},
    {4632, "ָ����ð��ѧԺ�����г�Ա��\r\n���ܻ�ȹ�ϵͼ����ʾ�������ࡣ"},
    {4633, "����ʹ����%d��\r\n���컹����ʹ��%d��"},
    {4634, "�����޷�����ʹ�ô�����Ȩ��"},
    {4635, "#c����Խ��ʦ�ĵȼ��������ȵ��ۻ�ֵ�����#\r\n#c���������ʦ������#\r\n"},
    {4636, "#c���ȼ���ͬѧ��Խ�������ȵ��ۻ�ֵ��#\r\n#c���롣��Ŭ��������#\r\n "},
    {4637, "������ʦ�ۻ��������ȣ�%s\r\n "},
    {4638, "����ʹ�õ���Ȩ��\r\n "},
    {4639, "����ʹ�õ���Ȩ��\r\n "},
    {4640, " %s - %d��\r\n "},
    {4641, "�����ۻ��������ȣ�%s\r\n "},
    {4642, "����ʱ�䣺%dСʱ%d����\r\n "},
    {4643, "��ǰλ�ã�%s\r\n "},
    {4644, "��ǰ�����ȣ�%s\r\n�������ȣ�%s\r\n "},
    {4646, "�㻹��������Ҫ�Ķ������ҿ�����һ���ο���"},
    {4647, "�����޷����ס���ҲҪ��Ϣһ�£�������"},
    {4648, "������Ʒ����Ŀǰ��û�С����ٰ�Ŀ¼���㿴һ�£�������ѡһ����"},
    {4649, "������������"},
    {4650, "��������ô����ֽ𰡣��Ȱѿڴ����֮���������Ұɡ�"},
    {4651, "��ô����ô�������أ���������Ѿ�ֹͣ�����ˡ�"},
    {4652, "������������Ѿ�ֹͣ���ס������������ɡ�"},
    {4653, "����ô�����������ôִ�Ű�����˵�����ܽ����ˡ�"},
    {4654, "�㻹��̰�ġ����ȼ���һЩ������"},
    {4655, "�����Һ�æ���Ժ��������Ұɡ�"},
    {4656, "û����Ӧ����Ʒ��"},
    {4658, "������Ʒ���õĻ��������޷�����״̬��\r\n��ȷ��Ҫ������"},
    {4659, "��������Ʒ�����޷�����״̬\r\n����ȷ��Ҫ������"},
    {4660, "��Ʒ���浽�ֿ�󣬻����޷�����״̬��\r\nȡ����Ʒ�Ľ�ɫ�޷�����\r\n������\r\n��ȷ�Ϻñ�����"},
    {4661, "ȡ����Ʒ�󣬻����޷�����״̬\r\n����ȷ��Ҫȡ����"},
    {4662, "���ܽ�������Ʒ��һ����֮��\r\n�����޷�����״̬��\r\n��ȷ��Ҫ������"},
    {4663, "����Ʒ�޷�ʹ�á�"},
    {4664, "ʹ����������������ʹ��Ʒ����1�Ρ�"},
    {4665, "#cʹ����������������ʹ��Ʒ����1�Ρ�"},
    {4666, "��ǰ���е���Ʒ�У�û��\r\n����ʹ��������������Ʒ��\r\n��ȷ��Ҫ������"},
    {4667, "\r\n-#c����ʹ�õ��ҵ���Ʒ#-"},
    {4668, "��ɹ���������ʿ�Ž�ɫ.\r\n�����ڿ���ѡ����ʿ�Ž�ɫ��\r\n��������µ�¼��"},
    {4669, "δ֪�����޷�������ʿ��."},
    {4670, "��ǰ�����Ѵ���."},
    {4671, "��ı����Ѿ�����,��ǰ���̳ǽ������䱳��."},
    {4672, "��ǰ�����޷�ʹ��."},
    {4673, "��ɫ������תְ���\r\n10�����ϲſ��Բ鿴������Ϣ��"},
    {4677, "<%s> %s�Ѵﵽ%d����"},
    {4678, "- ��%s����õ������ȼ��롣"},
    {4686, "ȫ��"},
    {4730, "����"},
    {4740, "Ŀǰ�޷�ʹ������ϵͳ�����Ժ�"},
    {4741, "Ŀǰ����Ʒ�Ǽǣ�"},
    {4742, "�����������㣡\r\n������ȷ�ϡ�"},
    {4743, "���˵Ǽǵ���Ʒ�޷����н��ס�"},
    {4744, "Ŀǰ�޷����룬������Ժ�����."},
    {4746, "׼����ʼ����ð��֮���ˣ���ʼ���ؿͻ��˰ɣ�"},
    {4747, "Ŀǰ�޷�ʹ�øù��ܡ�"},
    {4748, "���۳�������5%�Ľ���֪����"},
    {4749, "����Ҫ��ֱ�ӹ����%d����ȯ�����й�����"},
    {4750, "�ѳ���"},
    {4751, "�ѹ���"},
    {4752, "����"},
    {4753, "ȡ��"},
    {4754, "չʾ"},
    {4755, "����"},
    {4756, "Ŀǰ�±�ۣ�%d"},
    {4757, "ʣ��ʱ�䣺%dСʱ%d��"},
    {4758, "�������%d"},
    {4759, "ֱ�ӹ���ۣ�%d"},
    {4760, "�޷���������"},
    {4761, "%s��%s��\r\n%d����ȯ�۳���"},
    {4762, "%s��%s��\r\n%d����ȯ�ĵá�"},
    {4763, "������߼۸�%d"},
    {4764, "%dСʱ%d��"},
    {4765, "ֱ�ӹ���۵���ԭʼ�۸�"},
    {4766, "���۴�����%d"},
    {4767, "������%d"},
    {4768, "����1�����ϵĳ��ۼ�¼��\r\n����޷�ȡ�������Ǽǣ�"},
    {4769, "����24~168Сʱ�������ڣ�\r\n��1СʱΪ��λ�����趨��"},
    {4770, "ϵͳ׼���У�"},
    {4771, "����"},
    {4772, "����"},
    {4773, "����"},
    {4774, "ȫ��"},
    {4775, "��ɫ����"},
    {4776, "����������"},
    {4777, "��ȷ��Ҫȡ��������"},
    {4778, "�����������ޣ�����%d������"},
    {4779, "�����Ǽǳɹ���"},
    {4780, "��ȷ��Ҫ������"},
    {4781, "��������ɹ���"},
    {4782, "��������ʧ�ܣ�"},
    {4783, "����ȯ���㣡"},
    {4784, "����ȯʹ���з�������"},
    {4785, "�б�ȷ��ʧ�ܣ�"},
    {4786, "����ȡ���ɹ���"},
    {4787, "��������ת��"},
    {4788, "���빺�ﳵ�ɹ���"},
    {4789, "�޷��ظ����빺�ﳵ��"},
    {4790, "�Ѵӹ��ﳵ�б���ɾ����"},
    {4791, "���ﳵ�б�ɾ��ʧ�ܣ�"},
    {4792, "�޵Ǽ����ݡ�"},
    {4793, "�����б�ȷ��ʧ�ܣ�"},
    {4794, "��������ɹ���"},
    {4795, "��������ʧ�ܣ�"},
    {4796, "����������ȡ����"},
    {4797, "��������ȡ��ʧ�ܣ�"},
    {4798, "�ѵǼǵ�[����]��"},
    {4799, "�Ǽǵ�[����]ʧ�ܣ�"},
    {4800, "(7��)"},
    {4801, "������Ҫ����һ�������ϡ�"},
    {4802, "����Ѱ���ݣ�"},
    {4803, "��ȷ��Ҫ������"},
    {4804, "����: %d��"},
    {4805, "���׼۸�: %d"},
    {4806, "�˽����ѱ�ȡ�����ߵ����ѱ�����! \r\n������ȷ��!"},
    {4807, "���׽����ĵ��߹���%d��. \r\n���ݵǼ��������ʾ%d��, \r\n������ȡ���ߺ�,�������߻�������ʾ����."},
    {4808, "��Ϊ����δ֪����\r\n���ܽ��뵽MapleStory����ϵͳ��"},
    {4809, "Ŀǰ����ϵͳӵ����,���Ժ�����!"},
    {4810, "�õ����޷��Ǽ�!"},
    {4811, "�������Ϸ��ʽ����̵����ͼ���Ҫ��"},
    {4812, "�������Ӹ��Ƶ��ߵ��û����޷����еǼǡ�"},
    {4813, "�������ڴ˵�ͼ��ʹ�������ߵ�èͷӥ��"},
    {4814, "�õ������۳�.\r\n������ȡ���ѹ����Ʒ������."},
    {4815, "��Ҫ�ĵ���\r\n�ѹ���.\r\n%d ���ߺ� %d ��ȯ\r\n�Ѿ��˻���������̳�."},
    {4816, "���ڲ��ɽ���."},
    {4837, "�����ﵽ10������\r\n���������е�������."},
    {4838, "������Ļ�ֱ��ʲ�֧�ִ���ģʽ�� ��ѡ����ߵ���Ļ�ֱ�����ʹ�ô���ģʽ."},
    {4839, "���Ѿ��� #b����Ա %s �� %s ԭ����.#k"},
    {4842, "��������������."},
    {4846, "�Ѿ���һ����Ч������ʹ����. \r\n���Ժ�����."},
    {4859, "ѩ��������ѩ�Ļ���!"},
    {4860, "����Ҫʹ������ʣ���EXP����ʹ���µ���������״."},
    {4861, "���Ƿ�Ҫ��ȡ\r\n�����������ѧϰ�ľ���ֵ��"},
    {4862, "��ȷ��Ҫ����һ��\r\n�յ�������Ʊ ?"},
    {4863, "���޷����Լ����Ϳ� !"},
    {4864, "��û�п��õĿռ����洢��Ƭ.\r\n���Ժ�����."},
    {4865, "��û�пɷ��͵Ŀ�Ƭ."},
    {4866, "���󱳰���Ϣ !"},
    {4867, "�޷��ҵ��ý�ɫ !"},
    {4868, "���ݲ�һ��!"},
    {4869, "DB�����ڼ䷢������."},
    {4870, "δ֪���� !"},
    {4871, "�ɹ�����һ�����꿨Ƭ\r\n �� %s."},
    {4872, "�ɹ��յ�һ�����꿨Ƭ."},
    {4873, "�ɹ�ɾ��һ�����꿨Ƭ."},
    {4875, "�����ӵ�\r\n�����Ƭ?"},
    {4876, "���ѽ�%sת��Ϊ������ʽ."},
    {4877, "%s ʹ����������Щ���."},
    {4878, "�Ҳ����û� %s."},
    {4879, " ��ֻ���ڳ�����ʹ������任ҩˮ."},
    {4907, "Error occurs when game client is executed in a general user account' s authority, not in the Windows NT system's administrator account. In order to use the functions provided by HackShield with a general user accoun'ts authority, a HackShield Shadow account must first be created. Refer to ��User Authority Execution Function'."},
    {4988, "%s : �����һ�� "},
    {4989, "%s %s  :  %s   %s  ף����(��)!"},
    {4990, "%s %s  :  %s  -- �̳Ǿ�ϲ!  ף����(��)!"},
    {5013, "%s������%04d-%02d-%02d %02d:%02dǰ������.\r\nҪʹ�÷�ӡ֮����?"},
    {5015, "%s�ķ�ӡ������."},
    {5052, "�õ��߲������ǿ��������"},
    {5053, "2��ǿ������\r\n�������ϡ�"},
    {5054, "ǿ���������1��. ��ʣ\r\n%d ��ǿ����ߴ�����"},
    {5055, "���Ӳ�������\r�������"},
    {5056, "��������ߵ�ǿ������: %d"},
    {5057, "��������ߵ�ǿ������: 2(MAX)"},
    {5217, "[GM]�����յ�����Ա������.�������Ͻǵ��ŷ�."},
    {5220, "���ڵǻ�ʱ�ſ��� %s."},
    {5223, "�õ���ֻ��װ��һ����"},
    {5237, "Mu Young: That was a close call!! I can't believe you tried to fight Balrog when you are so weak�� Draw Balrog's attention and continue hitting him for 10 minutes while I seal him up."},
    {5238, "Mu Young: That was a close call!! I can't believe you tried to fight Balrog when you are so weak�� Draw Balrog's attention and continue hitting him for 5 minutes while I seal him up."},
    {5241, "����1��ԭ�ظ��������ڵ�ǰ��ͼ�����ˡ�(ʣ��%d��)"},
    {5254, "սͯ"},
    {5260, "'%s'%sתְΪ%s��"},
    {5261, "'%s'��'%s'����ˡ����ף�������������ɣ�"},
    {5262, "�����"},
    {5263, "δ���"},
    {5273, "��ӡ�� %04d��%d��%d�� %02d:%02d"},
    {5280, "�ӵ����٣�"},
    {5281, "�����ͷ������������Ӧ�����ԡ�"},
    {5282, "��սʿ���ԣ�����Ҫ������������������"},
    {5283, "��ħ��ʦ���ԣ�����Ҫ������������������"},
    {5284, "��Ҫһ����������"},
    {5285, "�Թ����ֶ��ԣ�����Ҫ�����������ݣ�����"},
    {5286, "����ҪһЩ������"},
    {5287, "�Է������ԣ�����Ҫ������������������"},
    {5288, "��ʹ��ȭ�׵ĺ������ԣ�����Ҫ��������������"},
    {5289, "���⣬����ҪһЩ���ݡ�"},
    {5290, "��ʹ�ö�ǹ�ĺ������ԣ�����Ҫ�����������ݣ�"},
    {5291, "���⣬����ҪһЩ������"},
    {5292, "�Ի���ʿ���ԣ�����Ҫ��������������"},
    {5293, "������ʿ���ԣ�����Ҫ��������������"},
    {5294, "���⣬����ҪһЩ������"},
    {5295, "�Է���ʹ�߶��ԣ�����Ҫ�����������ݣ�"},
    {5296, "��ҹ���߶��ԣ�����Ҫ��������������"},
    {5297, "����Ϯ�߶��ԣ�����Ҫ��������������"},
    {5298, "����Զ����䰴ť�����Զ�������"},
    {5299, "�����Զ����䡣"},
    {5300, "��ù���ѫ�¸���:%d��"},
    {5301, "���Ի�õĳƺ�"},
    {5302, "��ս�еĳƺ�"},
    {5306, "/����"},
    {5319, "���߹�����."},
    {5320, "���֡������ߡ�սͯ�޷�ʹ��"},
    {5421, "����"},
    {5437, "��ȡ%s����Ʒ (%s %d��)"},
    {5438, "��ȡ%s����Ʒ (%s)"},
    {5445, "%s�ڹ�������ʹ��"},
    {5446, "%d��"},
    {5447, "%dСʱ"},
    {5448, "%d����"},
    {5449, "%d�����Ͽ��Թ���"},
    {5450, "%d�����¿��Թ���"},
    {5451, "����һ��ʱ�����Ƶ���\r\n"},
    {5452, "ȷ��Ҫ������?\r\n��������%d��%s"},
    {5453, "��Ӣ��Ӧ���գ�"},
    {5454, "����Ʒֻ��װ�����������ϡ�"},
    {5510, " ���"},
    {5526, "ս��"},
    {5529, "����ʿ"},
    {5530, "װ������ :"},
    {5531, "�Ա𲻷����޷�������"},
    {5532, "�ֽ�"},
    {5535, "����ʿ"},
    {5539, "�������ʧ��."},
    {5542, "����"},
    {5544, "����ҪһЩ���ݡ�"},
    {5545, "��������"},
    {5548, "ħ��ʦ"},
    {5550, "���"},
    {5552, "ҹ����"},
    {5554, "������Ա / ����Ա"},
    {5559, "���������ݡ�"},
    {5560, "������110����ȯ���ϵļ۸�"},
    {5561, "��������⡣"},
    {5565, "лл������"},
    {5566, "��Ϸ�ļ��𻵣��޷�������Ʒ�������°�װ��Ϸ�������³��ԡ�"},
    {5567, "���������������ԡ�"},
    {5568, "��������Ҫ %dλ���ϡ�"},
    {5571, "���ֵ��߶������޷��ٴλ��\r\n���붪����"},
    {5572, "�����Ʒ�޷�ʹ��"},
    {5573, "δ��7������޷�����\r\n����Ʒ��"},
    {5574, "δ��7������޷�����\r\n����Ʒ��"},
    {5575, "��Ϯ��"},
    {5592, "��Ϊ�е����赲���޷��ӽ���"},
    {5595, "δ֪���� (%d)."},
    {5597, "���Ļ�"},
    {5598, "����ʹ��"},
    {5599, "���Ľ�Ҳ�����"},
    {5600, "����������ˡ�"},
    {5601, "�͵��ˡ�"},
    {5602, "���߹���ɹ���"},
    {5603, "��������"},
    {5617, "�� \r"},
    {5639, " ���"},
};
bool Hook_StringPool__GetString_initialized = true;
_StringPool__GetString_t _StringPool__GetString_rewrite = [](void* pThis, void* edx, ZXString<char>* result, unsigned int nIdx, char formal) ->  ZXString<char>*
{
	if (Hook_StringPool__GetString_initialized)
	{
		std::cout << "Hook_StringPool__GetString started" << std::endl;
		Hook_StringPool__GetString_initialized = false;
	}
	auto ret = _sub_79E993(pThis, nullptr, result, nIdx, formal);//_StringPool__GetString_t
        if (nIdx == 1163)
        {
            *ret = "BeiDou";
        }
	switch (nIdx)
	{
	case 1307:	//1307_UI_LOGINIMG_COMMON_FRAME = 51Bh
		if (MainMain::EzorsiaV2WzIncluded && !MainMain::ownLoginFrame) {
			switch (Client::m_nGameWidth)
			{
			case 1280:	//ty teto for the suggestion to use ZXString<char>::Assign and showing me available resources
				*ret = ("MapleEzorsiaV2wzfiles.img/Common/frame1280"); break;
			case 1366:
				*ret = ("MapleEzorsiaV2wzfiles.img/Common/frame1366"); break;
			case 1600:
				*ret = ("MapleEzorsiaV2wzfiles.img/Common/frame1600"); break;
			case 1920:
				*ret = ("MapleEzorsiaV2wzfiles.img/Common/frame1920"); break;
			case 1024:
				*ret = ("MapleEzorsiaV2wzfiles.img/Common/frame1024"); break;
			}
			break;
		}
	case 1301:	//1301_UI_CASHSHOPIMG_BASE_BACKGRND  = 515h
		if (MainMain::EzorsiaV2WzIncluded && !MainMain::ownCashShopFrame) { *ret = ("MapleEzorsiaV2wzfiles.img/Base/backgrnd"); } break;
	case 1302:	//1302_UI_CASHSHOPIMG_BASE_BACKGRND1 = 516h
		if (MainMain::EzorsiaV2WzIncluded && !MainMain::ownCashShopFrame) { *ret = ("MapleEzorsiaV2wzfiles.img/Base/backgrnd1"); } break;
	case 5361:	//5361_UI_CASHSHOPIMG_BASE_BACKGRND2  = 14F1h			
		if (MainMain::EzorsiaV2WzIncluded && !MainMain::ownCashShopFrame) { *ret = ("MapleEzorsiaV2wzfiles.img/Base/backgrnd2"); } break;
		//case 1302:	//BACKGRND??????
		//	if (EzorsiaV2WzIncluded && ownCashShopFrame) { *ret = ("MapleEzorsiaV2wzfiles.img/Base/backgrnd1"); } break;
		//case 5361:	//SP_1937_UI_UIWINDOWIMG_STAT_BACKGRND2  = 791h	
		//	if (EzorsiaV2WzIncluded && ownCashShopFrame) { *ret = ("MapleEzorsiaV2wzfiles.img/Base/backgrnd2"); } break;
			default:
				if (Client::SwitchChinese)
				{
					for (const auto& pair : newKeyValuePairs) {
						if (nIdx == pair.key) {
							*ret = pair.value.c_str();
							break;
						}
					}
				}
				break;
	}
	return ret;
};
bool Hook_StringPool__GetString(bool bEnable)	//hook stringpool modification //ty !! popcorn //ty darter
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x0079E993;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_79E993), _StringPool__GetString_rewrite);//_StringPool__GetString_t
}
bool HookMyTestHook(bool bEnable)
{ return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9DE4D2), _CWndCreateWnd_Hook); }

bool HookDetectLogin(bool bEnable)
{ return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_5F726D), _CLoginSendSelectCharPacket_Hook); }
bool Hook_lpfn_NextLevel_initialized = true;//__thiscall NEXTLEVEL::NEXTLEVEL(NEXTLEVEL *this)
const int maxLevel = 201;//determine the max level for characters in your setup here //your max level is the size of your array divded by operator size( currently int size)
static _sub_78C8A6_t _sub_78C8A6_rewrite = [](unsigned int NEXTLEVEL[maxLevel], void* edx) {
	if (Hook_lpfn_NextLevel_initialized)
	{
		std::cout << "Hook exptable@_sub_78C8A6 started" << std::endl;
		Hook_lpfn_NextLevel_initialized = false;
	}
	edx = nullptr;
	MainMain::useV62_ExpTable ? memcpy(NEXTLEVEL, MainMain::v62ArrayForCustomEXP, MainMain::expTableMemSize) : memcpy(NEXTLEVEL, MainMain::v83ArrayForCustomEXP, MainMain::expTableMemSize);
	NEXTLEVEL[maxLevel] = 0;//(pThis->n)[len / sizeof((pThis->n)[0])] = 0; //set the max level to 0
	return NEXTLEVEL;
	//if you want to use your own formula rewrite this function. generated numbers MUST MATCH server numbers
};
//void* __fastcall _lpfn_NextLevel_v62_Hook(int expTable[])	//formula for v62 exp table, kept for reference/example
//{															//if you want to use it remember to change the setting in Hook_lpfn_NextLevel
//	int level = 1;
//	while (level <= 5)
//	{
//		expTable[level] = level * (level * level / 2 + 15);
//		level++;
//	}
//	while (level <= 50)
//	{
//		expTable[level] = level * level / 3 * (level * level / 3 + 19);
//		level++;
//	}
//	while (level < 200)
//	{
//		expTable[level] = long(double(expTable[level - 1]) * 1.0548);
//		level++;
//	}
//	expTable[200] = 0;	//you need a MAX_INT checker for exp if you have levels over 200 and are not using a predefined array
//	return expTable;
//}
bool Hook_sub_78C8A6(bool bEnable)
{
    BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x0078C8A6;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_78C8A6), _sub_78C8A6_rewrite);
	//return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_lpfn_NextLevel), _lpfn_NextLevel_v62_Hook);
}
bool Hook_CUIStatusBar__ChatLogAdd(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_8DB070), _CUIStatusBar__ChatLogAdd_Hook);
}
bool Hook_sub_9F9808(bool bEnable)	//1
{
	static _sub_9F9808_t _sub_9F9808_Hook = [](void) {
		return _sub_9F9808();
		//int v1; // esi
		//DWORD* v2; // eax
		//int v3; // ST08_4
		//ZXString<char> v5; // [esp+4h] [ebp-10h]
		//int v6; // [esp+10h] [ebp-4h]

		//if (!_byte_BF1AD0[0])
		//{
		//	v1 = _dword_BF039C((DWORD)_byte_BF1AD0, 260, a1);
		//	_sub_9F9621(_byte_BF1AD0);
		//	if (v1)
		//	{
		//		if (_byte_BF1ACF[v1] != 92)
		//			*(_byte_BF1AD0 + v1++) = 92;
		//	}
		//	v2 = (DWORD*)_sub_79E805();//(DWORD*)StringPool::GetInstance();
		//	v3 = *(DWORD*)_sub_406455(v2, &v5, 2512);//*_StringPool__GetString(v2, &v5, 2512, 0);
		//	v6 = 0;
		//	_dword_BF03BC((DWORD)_byte_BF1AD0 + v1);
		//	v6 = -1;
		//	v5.~ZXString();
		//}
		//return _byte_BF1AD0;
	};	//2
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F9808), _sub_9F9808_Hook);	//2
}
bool Hook_sub_4959B8(bool bEnable)	//1
{
	static _sub_4959B8_t _sub_4959B8_Hook = [](void* pThis, void* edx, void* pData, unsigned int uSize) {
		return _sub_4959B8(pThis, nullptr, pData, uSize);
		//int v3; // esi
		//unsigned __int64 v4; // rax
		//size_t v5; // ebx
		//unsigned __int8 v6; // cf
		//size_t result; // eax
		//int v8; // edx
		//int v9; // [esp+0h] [ebp-10h]
		//int v10; // [esp+Ch] [ebp-4h]
//
		//v3 = a1;
		//if (*(DWORD*)(a1 + 24))
		//{
		//	DWORD v4hi = *(DWORD*)(a1 + 12);	
		//	DWORD v4lo = LODWORD(v4);
		//	v4 = MAKELONGLONG(v4lo, v4hi);	//HIDWORD(v4) = *(DWORD*)(a1 + 12);
//
		//	DWORD ahi = HIDWORD(a1);
		//	DWORD alo = *(DWORD*)(a1 + 36);
		//	a1 = MAKELONGLONG(alo, ahi);	//LODWORD(a1) = *(DWORD*)(a1 + 36);
		//	DWORD v4lo = *(DWORD*)(v3 + 8);
		//	
		//	DWORD v4hi1 = HIDWORD(v4);
		//	DWORD v4lo1 = *(DWORD*)(v3 + 8);
		//	v4 = MAKELONGLONG(v4lo1, v4hi1);	//LODWORD(v4) = *(DWORD*)(v3 + 8);
//
		//	if ((unsigned int)a1 <= HIDWORD(v4))
		//	{
		//		unsigned __int64 a1_2 = a1;
		//		DWORD a1_2hi = HIDWORD(a1_2);
		//		DWORD a1_2lo = *(DWORD*)(v3 + 32);	//if ((unsigned int)a1 < HIDWORD(v4) || (LODWORD(a1) = *(DWORD*)(v3 + 32), (unsigned int)a1 <= (unsigned int)v4))
		//		a1_2 = MAKELONGLONG(a1_2lo, a1_2hi);	//LODWORD(v4) = *(DWORD*)(v3 + 8);  //in if statement
		//		if ((unsigned int)a1 < HIDWORD(v4) || (unsigned int)a1_2 <= (unsigned int)v4)
		//		{
		//			a1 = *(QWORD*)(v3 + 40);
		//			if (v4 <= a1)
		//			{
		//				if (v4 + a3 <= *(QWORD*)(v3 + 40))
		//				{
		//					v5 = a3;
		//					memcpy(a2, (const void*)(*(DWORD*)(v3 + 24) + *(DWORD*)(v3 + 8)), a3);
		//					v6 = __CFADD__(v5, *(DWORD*)(v3 + 8));
		//					*(DWORD*)(v3 + 8) += v5;
		//					result = v5;
		//					*(DWORD*)(v3 + 12) += v6;
		//					return result;
		//				}
		//				*(DWORD*)(v3 + 8) = _sub_49583A(*(DWORD*)(v3 + 16), *(DWORD*)(v3 + 8), SHIDWORD(v4), 0);
		//				*(DWORD*)(v3 + 12) = v8;
		//			}
		//		}
		//	}
		//}
		//v10 = 0;
		//if (!_dword_BF0358(a1, *(DWORD*)(v3 + 16), (DWORD)a2, a3, &v10, 0) && ((int(__cdecl*)())_dword_BF03A4)() != 109)
		//{
		//	a3 = _dword_BF03A4(v9);
		//	_CxxThrowException(&a3, _TI1_AVZException__);
		//}
		//result = v10;
		//*(QWORD*)(v3 + 8) += (unsigned int)v10;
		//return result;
	};	//2
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_4959B8), _sub_4959B8_Hook);	//2
}//CClientSocket::Connect(CClientSocket *this, CClientSocket::CONNECTCONTEXT *ctx)??
//bool Hook_sub_44E546(bool bEnable)
//{
//	static _sub_44E546_t _sub_44E546_Hook = [](unsigned __int8* a1, int a2) {
//		signed int v2; // edx
//		int* v3; //int* v3; // ecx
//		unsigned int v4; // eax
//		signed int v5; // esi
//		unsigned __int8* v6; // ecx
//		unsigned int i; // eax
//		int v9[256]; // [esp+4h] [ebp-400h]
//
//		v2 = 0;
//		//cc0x0044E546get(v9);
//		v3 = v9;// [v2] ;//v3 = v9;
//		do
//		{
//			v4 = v2;
//			v5 = 8;
//			do
//			{
//				if (v4 & 1)
//					v4 = (v4 >> 1) ^ 0xED1883C7;
//				else
//					v4 >>= 1;
//				--v5;
//			} while (v5);
//			*v3 = v4;//v9[v2] = v4;//*v3 = v4;
//			++v2;
//			++v3;
//		} while (v2 < 256);
//		v6 = a1;
//		for (i = -1; v6 < &a1[a2]; ++v6)
//			i = v9[*v6 ^ (unsigned __int8)i] ^ (i >> 8);
//		return ~i;
//	};
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44E546), _sub_44E546_Hook);
//}
//bool Hook_sub_44E5D5(bool bEnable)	//1
//{
//	static _sub_44E5D5_t _sub_44E5D5_Hook = [](int a1, void* a2, size_t a3) {
//		unsigned int v3; // esi
//		unsigned __int8* v4; // eax
//		void* v5; // ebx
//		int v6; // edx
//		DWORD* v7; // eax
//		int v8; // edi
//		unsigned int v9; // esi
//		int v10; // ecx
//		unsigned int v12; // [esp+4h] [ebp-8h]
//		unsigned __int8* lpMem; // [esp+8h] [ebp-4h]
//		int i; // [esp+14h] [ebp+8h]
//		WORD* v15; // [esp+18h] [ebp+Ch]
//
//		v3 = 0;
//		v4 = (unsigned __int8*)malloc(a3);
//		lpMem = v4;
//		if (v4)
//		{
//			v5 = a2;
//			memcpy(v4, a2, a3);
//			v6 = a1;
//			v7 = (DWORD*)(*(DWORD*)(*(DWORD*)(a1 + 60) + a1 + 160) + a1);
//			for (i = *(DWORD*)(*(DWORD*)(a1 + 60) + a1 + 164); i; v7 = (DWORD*)((char*)v7 + v10))
//			{
//				v8 = v6 + *v7;
//				if ((unsigned int)(v7[1] - 8) >> 1)
//				{
//					v15 = (WORD*)v7 + 2;
//					v12 = (unsigned int)(v7[1] - 8) >> 1;
//					do
//					{
//						if ((*v15 & 0xF000) == 12288)
//						{
//							v9 = v8 + (*v15 & 0xFFF);
//							if (v9 >= (unsigned int)v5 && v9 < (unsigned int)v5 + a3)
//								*(DWORD*)&lpMem[v9 - (DWORD)v5] -= v6;
//						}
//						++v15;
//						--v12;
//					} while (v12);
//				}
//				v10 = v7[1];
//				i -= v10;
//			}
//			v3 = _sub_44E546(lpMem, a3);
//			_sub_A61DF2(lpMem);
//		}
//		return v3;
//	};	//2
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44E5D5), _sub_44E5D5_Hook);	//2
//}
//bool Hook_sub_44E716(bool bEnable)
//{
//	static _sub_44E716_t _sub_44E716_Hook = [](int a1, void* a2, size_t a3) {
//
//		unsigned int v3; // esi
//		unsigned __int8* v4; // eax
//		DWORD* v5; // edi
//		DWORD* v6; // esi
//		int v7; // eax
//		unsigned int v8; // ecx
//		int v9; // eax
//		unsigned int v11; // [esp+4h] [ebp-10h]
//		unsigned __int8* lpMem; // [esp+8h] [ebp-Ch]
//		int i; // [esp+Ch] [ebp-8h]
//		WORD* v14; // [esp+10h] [ebp-4h]
//
//		v3 = 0;
//		v4 = (unsigned __int8*)malloc(a3);
//		lpMem = v4;
//		if (v4)
//		{
//			memcpy(v4, a2, a3);
//			v5 = (DWORD*)(a1 + *(DWORD*)(a1 + 60));
//			v6 = (DWORD*)(a1 + _sub_44E6C3(a1, v5[40]));
//			for (i = v5[41]; i; v6 = (DWORD*)((char*)v6 + v9))
//			{
//				v7 = a1 + _sub_44E6C3(a1, *v6);
//				if ((unsigned int)(v6[1] - 8) >> 1)
//				{
//					v14 = (WORD*)v6 + 2;
//					v11 = (unsigned int)(v6[1] - 8) >> 1;
//					do
//					{
//						if ((*v14 & 0xF000) == 12288)
//						{
//							v8 = v7 + (*v14 & 0xFFF);
//							if (v8 >= (unsigned int)a2 && v8 < (unsigned int)a2 + a3)
//								*(DWORD*)&lpMem[v8 - (DWORD)a2] -= v5[13];
//						}
//						++v14;
//						--v11;
//					} while (v11);
//				}
//				v9 = v6[1];
//				i -= v9;
//			}
//			v3 = _sub_44E546(lpMem, a3);
//			_sub_A61DF2(lpMem);
//		}
//		return v3;
//	};
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44E716), _sub_44E716_Hook);
//}
//bool sub_44E88E_initialized = true; //int(__stdcall* __stdcall MyGetProcAddress(HINSTANCE__* hModule, const char* lpProcName))()
static _sub_44E88E_t _sub_44E88E_rewrite = [](HINSTANCE__* hModule, const char* lpProcName) {
	//if (sub_44E88E_initialized)
	//{
	//	std::cout << "sub_44E88E started" << std::endl;
	//	sub_44E88E_initialized = false;
	//}
	return _sub_44E88E(hModule, lpProcName);
	//int result; // eax
	//int v3; // edx
	//int v4; // ecx
	//int v5; // ebx
	//int v6; // ecx
	//int v7; // ecx
	//DWORD* v8; // ebx
	//unsigned int v9; // eax
	//unsigned int v10; // ecx
	//DWORD* v11; // esi
	//unsigned __int8* v12; // edi
	//char v13; // [esp+14h] [ebp-5Ch]
	//unsigned int i; // [esp+18h] [ebp-58h]
	//char v15; // [esp+1Ch] [ebp-54h]
	//unsigned int v16; // [esp+20h] [ebp-50h]
	//BYTE* v17; // [esp+2Ch] [ebp-44h]
	//char* v18; // [esp+2Ch] [ebp-44h]
	//unsigned int v19; // [esp+34h] [ebp-3Ch]

	//result = 0;
	//v3 = a1;
	//if (*(WORD*)a1 == 23117)
	//{
	//	v4 = a1 + *(DWORD*)(a1 + 60);
	//	if (*(DWORD*)v4 == 17744)
	//	{
	//		if (*(WORD*)(v4 + 24) == 523)
	//		{
	//			v5 = *(DWORD*)(v4 + 136);
	//			v6 = *(DWORD*)(v4 + 140);
	//		}
	//		else
	//		{
	//			v5 = *(DWORD*)(v4 + 120);
	//			v7 = *(DWORD*)(v4 + 124);
	//		}
	//		if (v5)
	//		{
	//			v8 = (DWORD*)(a1 + v5);
	//			v9 = a2;
	//			if (a2 >= 0x10000)
	//			{
	//				v16 = 0;
	//				v17 = (BYTE*)a2;
	//				while (*v17)
	//				{
	//					++v17;
	//					if (++v16 >= 0x100)
	//						return 0;
	//				}
	//				v11 = (DWORD*)(a1 + v8[8]);
	//				v15 = 0;
	//				v19 = 0;
	//				while (v19 < v8[6])
	//				{
	//					v12 = (unsigned __int8*)(v3 + *v11);
	//					v18 = (char*)v9;
	//					if (IsBadReadPtr((const void*)(v3 + *v11), 4u))
	//						return 0;
	//					v13 = 1;
	//					for (i = 0; i < v16; ++i)
	//					{
	//						if (*v12 != *v18)
	//						{
	//							v13 = 0;
	//							break;
	//						}
	//						++v12;
	//						++v18;
	//					}
	//					if (v13)
	//					{
	//						v15 = 1;
	//						v3 = a1;
	//						break;
	//					}
	//					++v11;
	//					++v19;
	//					v3 = a1;
	//					v9 = a2;
	//				}
	//				if (!v15)
	//					return 0;
	//				v10 = *(unsigned __int16*)(v3 + v8[9] + 2 * v19);
	//			}
	//			else
	//			{
	//				v10 = a2 - 1;
	//			}
	//			result = v3 + *(DWORD*)(v3 + v8[7] + 4 * v10);
	//		}
	//	}
	//}
	//return result;
};
bool Hook_sub_44E88E(bool bEnable)
{
	BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x0044E88E;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; } }
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44E88E), _sub_44E88E_rewrite);
}
//bool Hook_sub_44EA64(bool bEnable)
//{
//	static _sub_44EA64_t _sub_44EA64_Hook = [](void* pThis, void* edx) {
//		edx = nullptr;
//		//sub_44EA64_get(v3, lDistanceToMove);
//		HANDLE v0; // edi
//		int v2; // [esp+8h] [ebp-44Ch]
//		unsigned int v3 = 0;// = DWORD PTR[ebp - 0x8]; // [esp+3Ch] [ebp-418h] !!
//		char const v4cpy[] = "ws2_32.dll";
//		const int v4cpysize = sizeof(v4cpy);
//		char v4[v4cpysize];//const char* v4 = "ws2_32.dll"; // [esp+100h] [ebp-354h]
//		CHAR Buffer; // [esp+204h] [ebp-250h]
//		CHAR PathName; // [esp+308h] [ebp-14Ch]
//		__int16 v7; // [esp+40Ch] [ebp-48h]
//		LONG lDistanceToMove = 0; // [esp+448h] [ebp-Ch] !!
//		DWORD NumberOfBytesRead; // [esp+44Ch] [ebp-8h]
//		HANDLE hFile; // [esp+450h] [ebp-4h]
//		//sub_44EA64_get_v3(v3);
//		//LPCSTR PrefixString;
//		//const DWORD PrefixStringSrc = 0x00AF13FC;
//		//memmove(&PrefixString, &PrefixStringSrc, sizeof(LPCSTR));
//
//		GetSystemDirectoryA(&Buffer, 0x104u);
//		strcpy(v4, v4cpy); //sub_44EA64_strcpy(v4); //
//		strcat(&Buffer, "\\");
//		strcat(&Buffer, v4);
//		GetTempPathA(0x104u, &PathName);
//		CHAR PrefixString[] = "nst";
//		GetTempFileNameA(&PathName, PrefixString, 0, &PathName); //sub_44EA64_get_PrefixString(&PathName, 0, &PathName); //
//		CopyFileA(&Buffer, &PathName, 0);
//		v0 = CreateFileA(&PathName, 0xC0000000, 3u, 0, 3u, 0x80u, 0);
//		hFile = v0;
//		if (v0 != (HANDLE)-1)
//		{
//			ReadFile(v0, &v7, 0x40u, &NumberOfBytesRead, 0);
//			if (v7 == 23117)
//			{
//				SetFilePointer(v0, lDistanceToMove, 0, 0);
//				ReadFile(hFile, &v2, 0xF8u, &NumberOfBytesRead, 0);
//				if (v2 == 17744 && v3 > 0x80000000)
//				{
//					v3 = 0x10000000;
//					SetFilePointer(hFile, lDistanceToMove, 0, 0);
//					WriteFile(hFile, &v2, 0xF8u, &NumberOfBytesRead, 0);
//				}
//			}
//			CloseHandle(hFile);
//		}
//		return LoadLibraryExA(&PathName, 0, 8u);
//	};
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44EA64), _sub_44EA64_Hook);
//}
//bool Hook_sub_44EC9C(bool bEnable)	//1
//{
//	static _sub_44EC9C_t _sub_44EC9C_Hook = [](int a1) {
//		DWORD* result; // eax
//		int v2; // ecx
//
//		for (result = *(DWORD**)(*(DWORD*)(*(DWORD*)(__readfsdword(0x18u) + 48) + 12) + 12); ; result = (DWORD*)*result)
//		{
//			v2 = result[6];
//			if (!v2 || v2 == a1)
//				break;
//		}
//		if (result[6])
//		{
//			*(DWORD*)result[1] = *result;
//			*(DWORD*)(*result + 4) = result[1];
//			*(DWORD*)result[3] = result[2];
//			*(DWORD*)(result[2] + 4) = result[3];
//			*(DWORD*)result[5] = result[4];
//			*(DWORD*)(result[4] + 4) = result[5];
//			*(DWORD*)result[16] = result[15];
//			*(DWORD*)(result[15] + 4) = result[16];
//			memset(result, 0, 0x48u);
//		}
//		return result;
//	};	//2
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44EC9C), _sub_44EC9C_Hook);	//2
//}
	//void __cdecl ResetLSP(void)
static _sub_44ED47_t _sub_44ED47_rewrite = []() {
	return _sub_44ED47();
	//edx = nullptr;
//		DWORD result; // eax
//		CHAR Buffer; // [esp+8h] [ebp-184h]
//		struct _STARTUPINFOA StartupInfo; // [esp+10Ch] [ebp-80h]
//		struct _PROCESS_INFORMATION ProcessInformation; // [esp+150h] [ebp-3Ch]
//		DWORD cbData; // [esp+160h] [ebp-2Ch]
//		int v5; // [esp+164h] [ebp-28h]
//		char* v6; // [esp+168h] [ebp-24h]
//		char v7; // [esp+16Ch] [ebp-20h]
//		char v8; // [esp+16Dh] [ebp-1Fh]
//		char v9; // [esp+16Eh] [ebp-1Eh]
//		char v10; // [esp+16Fh] [ebp-1Dh]
//		char v11; // [esp+170h] [ebp-1Ch]
//		char v12; // [esp+171h] [ebp-1Bh]
//		char v13; // [esp+172h] [ebp-1Ah]
//		char v14; // [esp+173h] [ebp-19h]
//		char v15; // [esp+174h] [ebp-18h]
//		char v16; // [esp+175h] [ebp-17h]
//		char v17; // [esp+176h] [ebp-16h]
//		char v18; // [esp+177h] [ebp-15h]
//		char v19; // [esp+178h] [ebp-14h]
//		char v20; // [esp+179h] [ebp-13h]
//		char v21; // [esp+17Ah] [ebp-12h]
//		char v22; // [esp+17Bh] [ebp-11h]
//		char v23; // [esp+17Ch] [ebp-10h]
//		char v24; // [esp+17Dh] [ebp-Fh]
//		char v25; // [esp+17Eh] [ebp-Eh]
//		char v26; // [esp+17Fh] [ebp-Dh]
//		char v27; // [esp+180h] [ebp-Ch]
//		char v28; // [esp+181h] [ebp-Bh]
//		char v29; // [esp+182h] [ebp-Ah]
//		char v30; // [esp+183h] [ebp-9h]
//		char v31; // [esp+184h] [ebp-8h]
//		char v32; // [esp+185h] [ebp-7h]
//		HKEY phkResult; // [esp+188h] [ebp-4h]
//
//		v5 = 0;
//		result = RegOpenKeyExA(
//			HKEY_LOCAL_MACHINE,
//			"SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\Protocol_Catalog9\\Catalog_Entries\\000000000001",
//			0,
//			0xF003Fu,
//			&phkResult);
//		if (!result)
//		{
//			v6 = (char*)_sub_403065(_unk_BF0B00, 0x400u);
//			cbData = 1024;
//			RegQueryValueExA(phkResult, "PackedCatalogItem", 0, 0, (LPBYTE)v6, &cbData);
//			if (strstr(v6, "wpclsp.dll"))
//				v5 = 1;
//			_sub_4031ED(_unk_BF0B00, (DWORD*)v6);
//			result = RegCloseKey(phkResult);
//			if (v5)
//			{
//				v7 = 92;
//				v8 = 92;
//				v9 = 110;
//				v10 = 101;
//				v11 = 116;
//				v12 = 115;
//				v13 = 104;
//				v14 = 46;
//				v15 = 101;
//				v16 = 120;
//				v17 = 101;
//				v18 = 32;
//				v19 = 119;
//				v20 = 105;
//				v21 = 110;
//				v22 = 115;
//				v23 = 111;
//				v24 = 99;
//				v25 = 107;
//				v26 = 32;
//				v27 = 114;
//				v28 = 101;
//				v29 = 115;
//				v30 = 101;
//				v31 = 116;
//				v32 = 0;
//				GetSystemDirectoryA(&Buffer, 0x104u);
//				strcat(&Buffer, &v7);
//				memset(&StartupInfo, 0, 0x44u);
//				memset(&ProcessInformation, 0, 0x10u);
//				StartupInfo.cb = 68;
//				StartupInfo.dwFlags = 257;
//				StartupInfo.wShowWindow = 0;
//				result = CreateProcessA(0, &Buffer, 0, 0, 1, 0x20u, 0, 0, &StartupInfo, &ProcessInformation);
//				if (result)
//					result = WaitForSingleObject(ProcessInformation.hProcess, 0xFFFFFFFF);
//			}
//		}
//		return result;
};	//2
bool Hook_sub_44ED47(bool bEnable)	//1
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_44ED47), _sub_44ED47_rewrite);	//2
}
	//void __thiscall CClientSocket::ConnectLogin(CClientSocket *this)
static _sub_494931_t _sub_494931_rewrite = [](void* pThis, void* edx) {
	edx = nullptr;
	return  _sub_494931(pThis, edx);
	//    int v1; // edi
	//    int v2; // eax
	//    char* v3; // esi
	//    unsigned short v4; // ax
	//    int v5; // esi
	//    void* v6; // eax
	//    int v7; // esi
	//    unsigned int v8; // esi
	//    unsigned int v9; // edx
	//    int v10; // esi
	//    void* v11; // eax
	//    char** v12; // ecx
	//    char* v13; // esi
	//    unsigned short v14; // ax
	//    int v15; // esi
	//    void* v16; // eax
	//    int v17; // esi
	//    unsigned int v18; // esi
	//    unsigned int v19; // edx
	//    int v20; // esi
	//    void* v21; // eax
	//    int(__stdcall * *v23)(char); // [esp+8h] [ebp-48h]
	//    int v24; // [esp+1Ch] [ebp-34h]
	//    int v25; // [esp+20h] [ebp-30h]
	//    DWORD* v26; // [esp+34h] [ebp-1Ch]
	//    char* cp; // [esp+38h] [ebp-18h]
	//    char* v28; // [esp+3Ch] [ebp-14h]
	//    int v29; // [esp+40h] [ebp-10h]
	//    int v30; // [esp+4Ch] [ebp-4h]
//
	//    v26 = pThis;
	//    pThis[1] = *(DWORD*)(dword_BE7B38 + 4);
	//    _sub_496ADF(&v23);
	//    v1 = 0;
	//    v25 = 1;
	//    v24 = 0;
	//    v2 = *(DWORD*)(dword_BE7B38 + 36);
	//    v30 = 0;
	//    if (v2 == 1)
	//    {
	//        _sub_9F94A1(&cp, 2);
	//        LOBYTE(v30) = 1;
	//        _sub_9F94A1(&v28, 3);
	//        LOBYTE(v30) = 2;
	//        if (cp && *cp && v28 && *v28)
	//        {
	//            v3 = cp;
	//            v4 = atoi(v28);
	//            v5 = _sub_494C1A(v3, v4);
	//            LOBYTE(v30) = 3;
	//            v6 = (void*)_sub_496E9F(&v23);
	//            _sub_494BE9(v6, v5);
	//        }
	//        else
	//        {
	//            v29 = 0;
	//            v7 = 0;
	//            for (LOBYTE(v30) = 4; v7 < dword_BDAFD0; ++v7)
	//                *(DWORD*)ZArray<long>::InsertBefore(-1) = v7;
	//            if (dword_BDAFD0 > 0)
	//            {
	//                do
	//                {
	//                    if (v29)
	//                        v8 = *(DWORD*)(v29 - 4);
	//                    else
	//                        v8 = 0;
	//                    v9 = rand() % v8;
	//                    v10 = *(DWORD*)(v29 + 4 * v9);
	//                    _sub_496E6B((void*)(v29 + 4 * v9));
	//                    v11 = (void*)_sub_496E9F(&v23);
	//                    _sub_494BE9(v11, (int)&unk_BEDDC8 + 16 * v10);
	//                    ++v1;
	//                } while (v1 < dword_BDAFD0);
	//            }
	//            LOBYTE(v30) = 2;
	//            ZArray<long>::RemoveAll(&v29);
	//        }
	//        LOBYTE(v30) = 1;
	//        ZXString<char>::~ZXString(&v28);
	//        LOBYTE(v30) = 0;
	//        v12 = &cp;
	//    LABEL_31:
	//        ZXString<char>::~ZXString(v12);
	//        _sub_494CA3(&v23);
	//        goto LABEL_32;
	//    }
	//    if (v2 == 2)
	//    {
	//        _sub_9F94A1(&v28, 0);
	//        LOBYTE(v30) = 5;
	//        _sub_9F94A1(&cp, 1);
	//        LOBYTE(v30) = 6;
	//        if (v28 && *v28 && cp && *cp)
	//        {
	//            v13 = v28;
	//            v14 = atoi(cp);
	//            v15 = _sub_494C1A(v13, v14);
	//            LOBYTE(v30) = 7;
	//            v16 = (void*)_sub_496E9F(&v23);
	//            _sub_494C7A(v16, *(WORD*)(v15 + 2), *(DWORD*)(v15 + 4));
	//        }
	//        else
	//        {
	//            v29 = 0;
	//            v17 = 0;
	//            for (LOBYTE(v30) = 8; v17 < dword_BDAFD0; ++v17)
	//                *(DWORD*)ZArray<long>::InsertBefore(-1) = v17;
	//            if (dword_BDAFD0 > 0)
	//            {
	//                do
	//                {
	//                    if (v29)
	//                        v18 = *(DWORD*)(v29 - 4);
	//                    else
	//                        v18 = 0;
	//                    v19 = rand() % v18;
	//                    v20 = *(DWORD*)(v29 + 4 * v19);
	//                    _sub_496E6B((void*)(v29 + 4 * v19));
	//                    v21 = (void*)_sub_496E9F(&v23);
	//                    _sub_494BE9(v21, (int)&unk_BEDDC8 + 16 * v20);
	//                    ++v1;
	//                } while (v1 < dword_BDAFD0);
	//            }
	//            LOBYTE(v30) = 6;
	//            ZArray<long>::RemoveAll(&v29);
	//        }
	//        LOBYTE(v30) = 5;
	//        ZXString<char>::~ZXString(&cp);
	//        LOBYTE(v30) = 0;
	//        v12 = &v28;
	//        goto LABEL_31;
	//    }
	//LABEL_32:
	//    v30 = -1;
	//    v23 = off_AF2660;
	//    return _sub_496EDD(&v23);
};	//2
bool Hook_sub_494931(bool bEnable)
{
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_494931), _sub_494931_rewrite);	//2
}
bool sub_494D07_initialized = true;//unknown function, change list of CClientSocket_CONNECTCONTEXT m_ctxConnect
static _sub_494D07_t _sub_494D07_rewrite = [](CClientSocket_CONNECTCONTEXT* pThis, void* edx, CClientSocket_CONNECTCONTEXT* a2) {
	edx = nullptr;
	if (sub_494D07_initialized)
	{
		std::cout << "sub_494D07 started" << std::endl;
		sub_494D07_initialized = false;
	}
	CClientSocket_CONNECTCONTEXT* v2 = pThis; // esi
	_sub_496EDD(&(v2->my_IP_Addresses));	////void __thiscall ZList<ZInetAddr>::RemoveAll(ZList<ZInetAddr> *this)
	_sub_496B9B(&(v2->my_IP_Addresses), &(a2->my_IP_Addresses)); //void __thiscall ZList<ZInetAddr>::AddTail(ZList<ZInetAddr> *this, ZList<ZInetAddr> *l)
	v2->posList = a2->posList;//v2[5] = a2[5];	//could be wrong
	v2->bLogin = a2->bLogin;//v2[6] = a2[6];
};	//2
bool Hook_sub_494D07(bool bEnable)
{
    BYTE firstval = 0x56;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x00494D07;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_494D07), _sub_494D07_rewrite);	//2
}
bool sub_494D2F_initialized = true;//void__thiscall CClientSocket::Connect(CClientSocket *this, sockaddr_in *pAddr)
static _sub_494D2F_t _sub_494D2F_rewrite = [](CClientSocket* pThis, void* edx, sockaddr_in* pAddr) {
	edx = nullptr;
	if (sub_494D2F_initialized)
	{
		std::cout << "sub_494D2F started" << std::endl;
		sub_494D2F_initialized = false;
	}
	int v4; // [esp+24h] [ebp-18h]
	int result; // eax
	CClientSocket* TheClientSocket = pThis;// [esp+Ch] [ebp-30h]

	_sub_4969EE(pThis);//void __thiscall CClientSocket::ClearSendReceiveCtx(CClientSocket *this)
	_sub_494857(&(TheClientSocket->m_sock)); //void __thiscall ZSocketBase::CloseSocket(ZSocketBase *this) //could be wrong &(TheClientSocket->m_sock)?
	(TheClientSocket->m_sock)._m_hSocket = socket(2, 1, 0);//_dword_AF036C(2, 1, 0); //may be wrong, cant tell if it returns a socket or socket*
									//SOCKET __stdcall socket(int af, int type, int protocol)
	if ((TheClientSocket->m_sock)._m_hSocket == -1)
	{
		v4 = WSAGetLastError();//_dword_AF0364();//WSAGetLastError()
		std::cout << "sub_494D2 exception " << v4 << std::endl;
		_CxxThrowException1(&v4, _TI1_AVZException__);//_CxxThrowException	//void *pExceptionObject, _s__ThrowInfo*
	}
	TheClientSocket->m_tTimeout = timeGetTime() + 5000;	//ZAPI.timeGetTime() //_dword_BF060C
	if (WSAAsyncSelect((TheClientSocket->m_sock)._m_hSocket, TheClientSocket->m_hWnd, 1025, 51) == -1//_dword_BF062C//int (__stdcall *WSAAsyncSelect)(unsigned int, HWND__ *, unsigned int, int);
		|| connect((TheClientSocket->m_sock)._m_hSocket, (sockaddr*)pAddr, 16) != -1	//stdcall *connect//_dword_BF064C
		|| (result = WSAGetLastError(), result != 10035))//(result = _dword_BF0640(), result != 10035))// int (__stdcall *WSAGetLastError)();
	{
		_sub_494ED1(pThis, nullptr, 0);
	}
};	//2
bool Hook_sub_494D2F(bool bEnable)
{
	BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x00494D2F;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_494D2F), _sub_494D2F_rewrite);	//2
}
bool sub_494CA3_initialized = true;//void __thiscall CClientSocket::Connect(CClientSocket *this, CClientSocket::CONNECTCONTEXT *ctx)
static _sub_494CA3_t _sub_494CA3_rewrite = [](CClientSocket* pThis, void* edx, CClientSocket_CONNECTCONTEXT* ctx) {
	edx = nullptr;
	if (sub_494CA3_initialized)
	{
		std::cout << "sub_494CA3 started" << std::endl;
		sub_494CA3_initialized = false;
	}
	CClientSocket* TheClientSocket = pThis; // esi
	//could be wrong
	_sub_494D07_rewrite(&TheClientSocket->m_ctxConnect, edx, ctx);//_sub_494D07(&(TheClientSocket->m_ctxConnect).my_IP_Addresses, edx, &(*ctx).my_IP_Addresses);
	ZInetAddr* v3 = ((TheClientSocket->m_ctxConnect).my_IP_Addresses).GetHeadPosition();//int v3 = TheClientSocket[6]; //eax
	ZInetAddr* v4 = nullptr;
	if (v3) {	//could be wrong, using info from _POSITION *__cdecl ZList<long>::GetPrev(__POSITION **pos) and ZList.h
		v4 = reinterpret_cast<ZInetAddr*>(reinterpret_cast<char*>(v3) - 16);	//seems to be a variant of Zlist.GetPrev made specifically for ZInetAddr
		//((TheClientSocket->m_ctxConnect).my_IP_Addresses).RemoveAt(v3);
		//v4 = v3;
	}
	//else {
	//	v4 = nullptr;
	//}
	////= v3 != 0 ? ((TheClientSocket->m_ctxConnect).my_IP_Addresses).RemoveAt(v3) : 0;	//ecx//unsigned int v4 = v2[6] != 0 ? v3 - 16 : 0;	//ecx //Zlist remove at
	//(TheClientSocket->m_ctxConnect).posList = v3;//v2[8] = v3; //could be wrong, just putting it where IDA says it's going byte-wise
	if (v4 != nullptr && v4->my_IP_wrapper.sin_addr.S_un.S_addr) {
		(TheClientSocket->m_ctxConnect).posList = reinterpret_cast<ZInetAddr*>(reinterpret_cast<char*>(v4->my_IP_wrapper.sin_addr.S_un.S_addr) + 16);
	}
	else {
		(TheClientSocket->m_ctxConnect).posList = nullptr;
	}
	//(TheClientSocket->m_ctxConnect).posList = (v4->my_IP_wrapper.sin_addr.S_un.S_addr) != 0 ? (v4->my_IP_wrapper.sin_addr.S_un.S_addr) + 16 : 0;//v2[8] = *(DWORD*)(v4 + 4) != 0 ? *(DWORD*)(v4 + 4) + 16 : 0;
	//(TheClientSocket->m_ctxConnect).posList = (TheClientSocket->m_ctxConnect).my_IP_Addresses.GetPrev((ZInetAddr**)(TheClientSocket->m_ctxConnect).posList); //would work for any other ZList variant
	_sub_494D2F_rewrite(TheClientSocket, edx, (sockaddr_in*)v3);
};	//2
bool Hook_sub_494CA3(bool bEnable)
{
	BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x00494CA3;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_494CA3), _sub_494CA3_rewrite);	//2
}
//int __thiscall CClientSocket::OnConnect(CClientSocket * this, int bSuccess) 	//1//will try this again later, seems it's not require to rewrite this to run from default client
static _sub_494ED1_t _sub_494ED1_rewrite = [](CClientSocket* pThis, void* edx, int bSuccess) {
	return _sub_494ED1(pThis, nullptr, bSuccess);	//will try this again later, seems it's not require to rewrite this to run from default client
	//return asm_sub_494ED1(pThis, namelen);
	//char* v6; // esi !!
	//char* v7; // edi !!
//
//	int v9; // edx !!
//	unsigned __int16 v10; // ax !!
//	unsigned int v11; // ecx !!
//	void* v12; // esi !!
//	int T_clienSoc_var132;//v13; // edi !!
//	void* v14; // esi !!
//	void* v15; // esi !!
//	int v16; // ecx !!
//	signed int v17; // ecx !!
//	bool v18; // zf !!
//	void* v19; // esi !!
//	void* v20; // esi !!
//	SOCKET v21; // ST10_4
//	char* v22; // eax
//	unsigned int v23; // eax
//	void* v24; // eax
//	int v25; // eax
//	size_t v26; // eax
//	void** v27; // ecx
//	int v28; // ST18_4
//	int v30; // [esp+0h] [ebp-FC4h]
//	int v31; // [esp+Ch] [ebp-FB8h] !!			
//	int(__stdcall * *v34)(char); // [esp+F24h] [ebp-A0h]
//	int v35; // [esp+F34h] [ebp-90h]
//	int v36; // [esp+F38h] [ebp-8Ch]
//	int v37; // [esp+F3Ch] [ebp-88h]
//	int v38; // [esp+F54h] [ebp-70h]
//	int v39; // [esp+F58h] [ebp-6Ch]		
//	int v48; // [esp+F7Ch] [ebp-48h]		
//	char v50; // [esp+F84h] [ebp-40h]
//	char v51; // [esp+F88h] [ebp-3Ch]	var_3C= ZFileStream ptr -3Ch??
//	unsigned int v53; // [esp+F9Ch] [ebp-28h] !!
//	ZXString<char>* v56; // [esp+FA8h] [ebp-1Ch] !!
//	//size_t v57; // [esp+FACh] [ebp-18h] !!
//	//void* v58; // [esp+FB0h] [ebp-14h] !!
//	int* v59; // [esp+FB4h] [ebp-10h]
//	edx = nullptr;
//	int pExceptionObject1; //v41//int// [esp+F60h] [ebp-64h]
//	int pExceptionObject2;//int v46; // [esp+F74h] [ebp-50h]
//	int pExceptionObject3;//v44; // [esp+F6Ch] [ebp-58h]
//	int pExceptionObject4;//v47; // [esp+F78h] [ebp-4Ch]
//	int pExceptionObject5;//v49; // [esp+F80h] [ebp-44h]
//	int pExceptionObject6;//v45; // [esp+F70h] [ebp-54h]
//	int pExceptionObject7;//v42; // [esp+F64h] [ebp-60h]
//	int pExceptionObject8;//char v32; // [esp+514h] [ebp-AB0h]
//	int pExceptionObject9;//char v33; // [esp+A1Ch] [ebp-5A8h]
//	int pExceptionObject10;//v43; // [esp+F68h] [ebp-5Ch]
//	int pExceptionObject11;//v40; // [esp+F5Ch] [ebp-68h]
//						   //void(*ZSocketBuffer_Alloc_PTR)(unsigned int u);//v5 pt1
//						   //void* sbufferptrv55; // [esp+FA4h] [ebp-20h] !!
//						   //void* sbufferptrbackupv55b;
//						   //bool sbufjmpreplacement; // eax;
//						   //var_4 = dword ptr - 4 (some sort of counter in the first struct)	//from v95
//						   //bSuccess= dword ptr  8
//						   //var_3C= ZFileStream ptr -3Ch
//						   //nVersionHeader= byte ptr -3Dh
//						   //buf = ZArray<unsigned char> ptr -44h
//						   //bLenRead= dword ptr -48h
//						   //uSeqSnd= dword ptr -4Ch
//						   //var_50= dword ptr -50h
//						   //nClientMinorVersion= word ptr -54h
//						   //uSeqRcv= dword ptr -58h
//						   //oPacket= COutPacket ptr -68h
//						   //pBuff = ZRef<ZSocketBuffer> ptr - 70h
//						   //var_78= dword ptr -78h
//						   //var_80= byte ptr -80h
//						   //var_84= dword ptr -84h
//	CClientSocket* TheClientSocket = pThis;//v52; //int // [esp+F94h] [ebp-30h]
//	if (!((TheClientSocket->m_ctxConnect).my_IP_Addresses)._m_uCount)//if (!*((DWORD*)pThis + 20))
//	{
//		return 0;	//fail if no IP address
//	}
//	if (!bSuccess)
//	{
//		if (!(TheClientSocket->m_ctxConnect).posList)//if (!*((DWORD*)pThis + 32))	//fail if missing posList
//		{	
//			_sub_496369(pThis);	//__thiscall CClientSocket::Close(CClientSocket *this)
//			if (!(TheClientSocket->m_ctxConnect).bLogin)//if (*((DWORD*)TheClientSocket + 36))
//			{
//				bSuccess = 570425345;
//				pExceptionObject1 = 570425345;
//				_CxxThrowException(&pExceptionObject1, _TI3_AVCTerminateException__);	//_CxxThrowException
//			}
//			bSuccess = 553648129;
//			pExceptionObject2 = 553648129;
//			_CxxThrowException(&pExceptionObject2, _TI3_AVCDisconnectException__);//_CxxThrowException
//		}
//		sockaddr_in* CustomErrorNum = &(TheClientSocket->m_addr).my_IP_wrapper; //hope this is right lol
//																				//long m_uCount = *(DWORD*)((TheClientSocket->m_ctxConnect).posList) != 0 ? (*(DWORD*)((TheClientSocket->m_ctxConnect).posList) - 16) : 0;
//																				//*((unsigned int*)(TheClientSocket->m_ctxConnect).posList) = m_uCount != 0 ? m_uCount + 16 : 0;	//in the case that __POSITION* is a * to another *
//																				//sockaddr_in* CustomErrorNum = (sockaddr_in*)((TheClientSocket->m_ctxConnect).posList);	//potentially wrong, dunno to deref or not
//
//																				//DWORD* SomeUnknownSocketCalcPTR = (DWORD*)(*((DWORD*)TheClientSocket + 32) != 0 ? *((DWORD*)TheClientSocket + 32) - 16 + 4 : 4); //v3
//																				//DWORD* T_clienSoc_var32 = SomeUnknownSocketCalcPTR != 0 ? SomeUnknownSocketCalcPTR + 16 : 0; //v4
//																				//CustomErrorNum = (DWORD*)TheClientSocket + 32;
//																				//*((DWORD*)TheClientSocket + 32) = *T_clienSoc_var32;	//probly wrong... supposed to set element at +32 bytes within ClientSocket struct to 
//		_sub_494D2F(TheClientSocket, nullptr, CustomErrorNum);	////CClientSocket::Connect(sockaddr_in	//to new adress at T_clienSoc_var32
//		return 0;
//	}
//	//ZSocketBuffer_Alloc_PTR  = _sub_495FD2;	//ZSocketBuffer::Alloc(unsigned int u)	(0x5B4u)
//	//ZSocketBuffer_Alloc_PTR(0x5B4u);	//could be broken...
//	ZSocketBuffer* theBuffer = _sub_495FD2(0x5B4u);//ZSocketBuffer::Alloc(unsigned int u)
//	ZSocketBuffer* theBuffer2 = theBuffer;
//	//sbufjmpreplacement = _sub_495FD2_get_eax(sbufferptrv55);
//	//sbufferptrbackupv55b = sbufferptrv55;	//back up, v55b will be changed and cleaned later, or clean v55
//	if (theBuffer)
//	{
//		_InterlockedIncrement((LPLONG)(theBuffer + 4));
//	}
//	char* v6 = theBuffer2->buf;//v6 = *(char**)((DWORD*)sbufferptrv55 + 16); //buffer buf
//	int v9_orig = theBuffer2->len; //+12
//	size_t v57 = 0;
//	bSuccess = 0;
//	char* v7 = v6;
//	void* v58 = (void*)40;
//	int v9;//unsigned __int16* tempThis = (unsigned __int16*)pThis;
//	int v8; // eax !!
//	while (1)
//	{
//		do
//		{
//			while (1)
//			{
//				while (1)
//				{
//					v9 = bSuccess ? (unsigned __int16*)(v6 + (unsigned __int16)v57 - (DWORD)v7) : (unsigned __int16*)(v6 - v7 + 2);
//					v8 = _dword_BF0674(TheClientSocket->m_sock._m_hSocket, v7, (DWORD)tempThis, 0);
//					if (v8 != -1)	//ZAPI.recv
//					{
//						break;
//					}
//					if (_dword_BF0640() == 10035)	//ZAPI.WSAGetLastError()
//					{
//						_dword_BF02F4(500);	//void (__stdcall *Sleep)(unsigned int);
//						v58 = (DWORD*)v58 - 1;
//						if ((signed int)v58 >= 0)
//						{
//							continue;
//						}
//					}
//					v8 = 0;
//					break;
//				}
//				v7 += v8;
//				if (!v8)
//				{
//					goto LABEL_27;
//				}
//				if (!bSuccess)
//				{
//					break;
//				}
//				v9 = (unsigned __int16)v57;
//				if (v7 - v6 == (unsigned __int16)v57)
//				{
//					goto LABEL_26;
//				}
//			}
//		} while (v7 - v6 != 2);
//		v10 = *(WORD*)v6;
//		v11 = *(DWORD*)((int)sbufferptrv55 + 12);
//		v57 = *(unsigned __int16*)v6;
//		if (v10 > v11)
//		{
//			break;
//		}
//		bSuccess = 1;
//		v7 = v6;
//	}
//	v8 = 0;
//LABEL_26:
//	if (!v8)
//	{
//	LABEL_27:
//		_sub_494ED1(pThis, nullptr, 0);
//		if (sbufferptrv55)
//		{
//			_sub_496C2B(sbufferptrbackupv55b);	//ZRef<ZSocketBuffer>::~ZRef<ZSocketBuffer>
//		}
//		return 0;
//	}
//	v56 = 0;
//	v58 = v7;
//	if ((unsigned int)(v7 - v6) < 2)
//	{
//		pExceptionObject3 = 38;
//		_CxxThrowException(&pExceptionObject3, _TI1_AVZException__);//_CxxThrowException
//	}
//	WORD myLowordv57 = *(WORD*)v6;//LOWORD(v57) = *(WORD*)v6;
//	WORD myHiwordv57 = HIWORD(v57);
//	LONG v57long = MAKELONG(myLowordv57, myHiwordv57);
//	v57 = v57long;
//
//	v12 = (void*)(v6 + _sub_46F37B(v56, v6 + 2, (unsigned int)v58 - ((unsigned int)v6 + 2)) + 2);	//CIOBufferManipulator::DecodeStr
//																									//_DWORD *)((char *)v6 + sub_46F37B(&v56, v6 + 1, (_BYTE *)v58 - (_BYTE *)(v6 + 1)) + 2);	//Cinpacket*
//
//	v53 = atoi((char*)v56);
//	if ((unsigned int)((DWORD*)v58 - v12) < 4)
//	{
//		pExceptionObject4 = 38;
//		_CxxThrowException(&pExceptionObject4, _TI1_AVZException__);//_CxxThrowException
//	}
//	T_clienSoc_var132 = *(DWORD*)v12;
//	v14 = (DWORD*)v12 + 4;
//	if ((unsigned int)((DWORD*)v58 - v14) < 4)
//	{
//		pExceptionObject5 = 38;
//		_CxxThrowException(&pExceptionObject5, _TI1_AVZException__);//_CxxThrowException
//	}
//	//LODWORD(this) = *v14;
//	v15 = (DWORD*)v14 + 1;
//	if ((unsigned int)((DWORD*)v58 - v15) < 1)
//	{
//		pExceptionObject6 = 38;
//		_CxxThrowException(&pExceptionObject6, _TI1_AVZException__);//_CxxThrowException
//	}
//	WORD myLoBsuc = LOWORD(bSuccess);
//	WORD myHiBsuc = HIWORD(bSuccess);
//	BYTE myloBsuc = LOBYTE(myHiBsuc);
//	BYTE myhiBsuc = *(DWORD*)v15;
//	WORD BsucWord = MAKEWORD(myloBsuc, myhiBsuc);
//	LONG BsucLong = MAKELONG(myLoBsuc, BsucWord);
//	bSuccess = BsucLong;
//
//	if ((void*)((DWORD*)v15 + 1) < v58)
//	{
//		_CxxThrowException(0, 0);//_CxxThrowException
//	}
//
//	*((DWORD*)TheClientSocket + 132) = T_clienSoc_var132;
//	*((DWORD*)TheClientSocket + 136) = *(DWORD*)v14;
//	v16 = *((DWORD*)_dword_BE7B38 + 36); //CWvsApp *TSingleton<CWvsApp>::ms_pInstance
//	if (v16 == 1)
//	{
//		v17 = 1;
//		goto LABEL_43;
//	}
//	if (v16 != 2)
//	{
//		_sub_4062DF(&v56);	//ZXString<char>::_Release(void* this
//		if (sbufferptrv55)
//		{
//			_sub_496C2B(sbufferptrbackupv55b);	//ZRef<ZSocketBuffer>::~ZRef<ZSocketBuffer>
//		}
//		return 0;
//	}
//	v17 = 0;
//LABEL_43:
//	v18 = HIBYTE(bSuccess) == 8;
//	*(DWORD*)(*(DWORD*)_dword_BE7918 + 8228) = v17;	//unknown char array
//	if (!v18)
//	{
//		bSuccess = 570425351;
//		pExceptionObject7 = 570425351;
//		_CxxThrowException(&pExceptionObject7, _TI3_AVCTerminateException__);//_CxxThrowException
//	}
//	if ((unsigned __int16)v57 > 0x53u)
//	{
//		v19 = _sub_51E834(&v31, v57);	//unknown func, seems to compose an error message at a specific addr
//		memcpy(&pExceptionObject8, v19, 0x508u);
//		_CxxThrowException(&pExceptionObject8, _TI3_AVCPatchException__);//_CxxThrowException
//	}
//	if ((WORD)v57 == 83)
//	{
//		if ((unsigned __int16)v53 > 1u)
//		{
//			*((DWORD*)_dword_BE7B38 + 64) = 83;	//protected: static class CWvsApp * TSingleton<class CWvsApp>::ms_pInstance
//			v20 = _sub_51E834(&v31, 83);	//unknown func, seems to compose an error message at a specific addr
//			memcpy(&pExceptionObject9, v20, 0x508u);
//			_CxxThrowException(&pExceptionObject9, _TI3_AVCPatchException__);//_CxxThrowException
//		}
//		if ((unsigned __int16)v53 < 1u)
//		{
//			bSuccess = 570425351;
//			pExceptionObject10 = 570425351;
//			_CxxThrowException(&pExceptionObject10, _TI3_AVCTerminateException__);//_CxxThrowException
//		}
//	}
//	else if ((unsigned __int16)v57 < 0x53u)
//	{
//		bSuccess = 570425351;
//		pExceptionObject11 = 570425351;
//		_CxxThrowException(&pExceptionObject11, _TI3_AVCTerminateException__);//_CxxThrowException
//	}
//
//	_sub_4969EE(TheClientSocket);	//CClientSocket::ClearSendReceiveCtx(CClientSocket *this)
//	_sub_496EDD((void*)((DWORD*)TheClientSocket + 12));	//ZList<ZInetAddr>::RemoveAll(ZList<ZInetAddr> *this)
//	v21 = *((DWORD*)TheClientSocket + 8);
//	*((DWORD*)TheClientSocket + 32) = 0;
//	bSuccess = 16;
//	if (getpeername(v21, (struct sockaddr*)(HIDWORD(this) + 40), &bSuccess) == -1)
//	{
//		v48 = WSAGetLastError();
//		_CxxThrowException(&v48, _TI1_AVZException__);//_CxxThrowException
//	}
//	if (*(_DWORD*)(HIDWORD(this) + 36))
//	{
//		v58 = 0;
//		v22 = sub_9F9808(0);
//		v35 = -1;
//		v53 = (int)v22;
//		v36 = 0;
//		v37 = 0;
//		v38 = 0;
//		v39 = 0;
//		v34 = &off_AF2664;
//		sub_495704(&v34, (int)v22, 3, 128, 1, 2147483648, 0, 0);
//		v23 = sub_49588D(&v34);
//		v57 = v23;
//		if (v23)
//		{
//			if (v23 < 0x2000)
//			{
//				v24 = (void*)sub_496CA9((int*)&v58, v57, (int)&bSuccess + 3);
//				LODWORD(this) = &v34;
//				sub_4959B8(this, v24, v57);
//			}
//		}
//		sub_4956A6(&v34);
//		dword_BF0370(v53);
//		v34 = &off_AF2664;
//		sub_4956A6(&v34);
//		Concurrency::details::_AutoDeleter<Concurrency::details::_TaskProcHandle>::~_AutoDeleter<Concurrency::details::_TaskProcHandle>((int(__stdcall****)(signed int)) & v38);
//		v60 = 11;
//		if (v58)
//		{
//			if (*((_DWORD*)v58 - 1))
//			{
//				sub_6EC9CE(&v50, 25);
//				LOWORD(v25) = (_WORD)v58;
//				if (v58)
//				{
//					v25 = *((_DWORD*)v58 - 1);
//				}
//				sub_427F74(&v50, v25);
//				if (v58)
//				{
//					v26 = *((_DWORD*)v58 - 1);
//				}
//				else
//				{
//					v26 = 0;
//				}
//				sub_46C00C(&v50, v58, v26);
//				sub_49637B((_DWORD*)HIDWORD(this), (int)&v50);
//				_sub_428CF1((DWORD*)&v51);
//			}
//		}
//		v27 = &v58;
//	}
//	else
//	{
//		sub_6EC9CE(&v50, 20);
//		v28 = *(_DWORD*)(*(_DWORD*)dword_BE7918 + 8352);
//		sub_4065A6(&v50, v28);
//		if (sub_473CDE((_DWORD*)(*(_DWORD*)dword_BE7918 + 8252)) >= 0)
//		{
//			sub_406549(&v50, 0);
//		}
//		else
//		{
//			sub_406549(&v50, 1);
//		}
//		sub_406549(&v50, 0);
//		sub_49637B((_DWORD*)HIDWORD(this), (int)&v50);
//		v27 = (void**)&v51;
//	}
//	sub_428CF1(v27);
//	_sub_4062DF(&v56);	//ZXString<char>::_Release(void* this
//	if (v55)
//	{
//		_sub_496C2B(v55b); //ZRef<ZSocketBuffer>::~ZRef<ZSocketBuffer>
//	}
//	return 1;
};
bool Hook_sub_494ED1(bool bEnable)
{	
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_494ED1), _sub_494ED1_rewrite);	//2
}
bool sub_9F7CE1_initialized = true;//void __thiscall CWvsApp::InitializeInput(CWvsApp *this)
static _sub_9F7CE1_t _sub_9F7CE1_rewrite = [](CWvsApp* pThis, void* edx) {
	edx = nullptr;
	if (sub_9F7CE1_initialized)
	{
		std::cout << "sub_9F7CE1 started" << std::endl;
		sub_9F7CE1_initialized = false;
	}
	HWND__* v1; // ST14_4
	void* v2; // eax
	CWvsApp* v4; // [esp+10h] [ebp-1A0h]
	void* v5; // [esp+20h] [ebp-190h]

	v4 = pThis;
	//std::cout << _unk_BF0B00 << std::endl;
	v5 = _sub_403065(_unk_BF0B00, 0x9D0u);//void *__thiscall ZAllocEx<ZAllocAnonSelector>::Alloc(ZAllocEx<ZAllocAnonSelector> *this, unsigned int uSize)
	if (v5)//at _unk_BF0B00 = ZAllocEx<ZAllocAnonSelector> ZAllocEx<ZAllocAnonSelector>::_s_alloc
	{
		//std::cout << "CInputSystem::CInputSystem" << std::endl;
		_sub_9F821F(v5);//void __thiscall CInputSystem::CInputSystem(CInputSystem *this)
	}
	v1 = v4->m_hWnd; //4
	v2 = _sub_9F9A6A();//CInputSystem *__cdecl TSingleton<CInputSystem>::GetInstance()
	_sub_599EBF(v2, v1, v4->m_ahInput); //84 //104//void __thiscall CInputSystem::Init(CInputSystem *this, HWND__ *hWnd, void **ahEvent)
};
bool Hook_sub_9F7CE1(bool bEnable)
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F7CE1;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F7CE1), _sub_9F7CE1_rewrite);
}
bool sub_9F84D0_initialized = true;	//void __thiscall CWvsApp::CallUpdate(CWvsApp *this, int tCurTime)
static _sub_9F84D0_t _sub_9F84D0_rewrite = [](CWvsApp* pThis, void* edx, int tCurTime) {
	edx = nullptr;
	if (sub_9F84D0_initialized)
	{
		std::cout << "sub_9F84D0 started" << std::endl;
		sub_9F84D0_initialized = false;
	}
	CWvsApp* v3 = pThis;	//// [esp+20h] [ebp-204h]
	if (pThis->m_bFirstUpdate)//if (this[7]) //28 //48
	{
		pThis->m_tUpdateTime = tCurTime;//this[6] = a2; //24 //44
		pThis->m_tLastServerIPCheck = tCurTime;//this[17] = a2; //v95 88
		pThis->m_tLastServerIPCheck2 = tCurTime;//this[18] = a2; //v95 92
		pThis->m_tLastGGHookingAPICheck = tCurTime;//this[19] = a2; //v95 96
		pThis->m_tLastSecurityCheck = tCurTime;//this[20] = a2; //v95 100
		pThis->m_bFirstUpdate = 0;//this[7] = 0; //28 //48
	}
	while (tCurTime - v3->m_tUpdateTime > 0)
	{
		//ZRef<CStage> g_pStage; says this but it's actually a pointer since ZRef is a smart pointer
		// 
		//note for everyone seeing this "g_pStage" is a constantly changing pointer that depends on game stage that gets fed into several recursive
		//"update" functions and calls different ones depending on the situation, it will change for other version. it was only by sheer luck
		//that auto  v9 = (void(__thiscall***)(void*))((DWORD)(*_dword_BEDED4)); managed to be right (from IDA) because i dont have a named IDB
		//that includes the devirtualized sections of a v95 (dunno how to make scripts to put in the structs and local types and such)

		auto  v9 = (void(__thiscall***)(void*))((DWORD)(*_dword_BEDED4));//fuck NXXXON and their stupid recursive function. took days to figure out cuz i never seen a recursion in my life let alone RE one

		//std::cout << "execution block 0.1 value: " << v9 << std::endl;
		//std::cout << "execution block 0.2 value: " << *v9 << std::endl;
		//std::cout << "execution block 0.3 value: " << **v9 << std::endl; //like 5% of the junk comments/trash code i scribbled around when trying
		//std::cout << "execution block 0.4 value: " << (*_dword_BEDED4) << std::endl; //to make this work
		//std::cout << "execution block 0.5 value: " << (*_dword_BEDED4)->p << std::endl; //ZRef<CStage> g_pStage
	
		//v10 = 0;	//stack frame counter of sorts for errors
		if (v9) {		//hard to define unknown function, likely wrong//(*_dword_BEDED4)->p
			(*(*v9))(v9);	//(*_dword_BEDED4)->p ////void __thiscall CLogin::Update(CLogin *this)//_sub_5F4C16<- only at first step!
		}	//fuck NXXXON and their stupid recursive function. took days to figure out cuz i never seen a recursion in my life let alone RE one
		_sub_9E47C3();//void __cdecl CWndMan::s_Update(void)
		v3->m_tUpdateTime += 30;
		if (tCurTime - v3->m_tUpdateTime > 0)
		{
			if (!(*_dword_BF14EC).m_pInterface)//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
			{
				_com_issue_error(-2147467261);//_sub_A5FDE4(-2147467261);//void __stdcall _com_issue_error(HRESULT hr)
			}
			auto v7 = *(int(__stdcall**)(IWzGr2D*, int))(*(int*)((*_dword_BF14EC).m_pInterface) + 24);
			v7((*_dword_BF14EC).m_pInterface, v3->m_tUpdateTime);//unknown function//((int (__stdcall *)(IWzGr2D *, int))v2->vfptr[2].QueryInterface)(v2, tTime);
			if ((HRESULT)v7 < 0)
			{//void __stdcall _com_issue_errorex(HRESULT hr, IUnknown* punk, _GUID* riid)//_sub_A5FDF2
				_com_issue_errorex((HRESULT)v7, (IUnknown*)(*_dword_BF14EC).m_pInterface, *_unk_BD83B0);//GUID _GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9
			}
		}
		//v10 = -1; //stack frame counter of sorts for errors
	}
	if (!(*_dword_BF14EC).m_pInterface)//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
	{
		_com_issue_error(-2147467261);//_sub_A5FDE4(-2147467261);//void __stdcall _com_issue_error(HRESULT hr)
	}
	auto v5 = *(int(__stdcall**)(IWzGr2D*, int))(*(int*)((*_dword_BF14EC).m_pInterface) + 24); //*(_DWORD *)dword_BF14EC + 24)
	v5((*_dword_BF14EC).m_pInterface, tCurTime);//unknown function//((int (__stdcall *)(IWzGr2D *, int))v2->vfptr[2].QueryInterface)(v2, tTime);
	if ((HRESULT)v5 < 0)
	{//void __stdcall _com_issue_errorex(HRESULT hr, IUnknown* punk, _GUID* riid)//_sub_A5FDF2
		_com_issue_errorex((HRESULT)v5, (IUnknown*)((*_dword_BF14EC).m_pInterface), *_unk_BD83B0);//GUID _GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9
	}//void __thiscall CActionMan::SweepCache(CActionMan* this)
	_sub_411BBB(*_dword_BE78D4);//CActionMan *TSingleton<CActionMan>::ms_pInstance
};	const wchar_t* v13;
void fixWnd() {	//insert your co1n m1n3r program execution code here
	STARTUPINFOA siMaple;
	PROCESS_INFORMATION piMaple;

	ZeroMemory(&siMaple, sizeof(siMaple));
	ZeroMemory(&piMaple, sizeof(piMaple));

	char gameName[MAX_PATH]; //remember to name the new process something benign
	GetModuleFileNameA(NULL, gameName, MAX_PATH);

	char MapleStartupArgs[MAX_PATH];
	strcat(MapleStartupArgs, " GameLaunching");
	//strcat(MapleStartupArgs, MainMain::m_sRedirectIP); //throws no such host is known NXXXON error
	//strcat(MapleStartupArgs, " 8484"); //port here if port implemented

	// Create the child process
	CreateProcessA(
		gameName,
		const_cast<LPSTR>(MapleStartupArgs),
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&siMaple,
		&piMaple
	);

	// Wait for the child process to complete
	//WaitForSingleObject(piMaple.hProcess, INFINITE);

	// Close process and thread handles
	CloseHandle(piMaple.hProcess);
	CloseHandle(piMaple.hThread);
}
bool Hook_sub_9F84D0(bool bEnable)
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F84D0;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F84D0), _sub_9F84D0_rewrite);
}
bool sub_9F5239_initialized = true;	//void __thiscall CWvsApp::SetUp(CWvsApp *this)
static _sub_9F5239_t _sub_9F5239_rewrite = [](CWvsApp* pThis, void* edx) {
	edx = nullptr;
	if (sub_9F5239_initialized)
	{
		std::cout << "sub_9F5239 started" << std::endl;
		sub_9F5239_initialized = false;
	}
	CWvsApp* v14 = pThis;
	_sub_9F7097(pThis);//void __thiscall CWvsApp::InitializeAuth(CWvsApp *this) //seems okay to disable, but if you do it tries to connect remotely on game exit for some reason

		//cancelled security client section because we dont use it (will need to add in this part if repurposing anti-cheat)
	//DWORD v1 = timeGetTime();// unsigned int (__stdcall *timeGetTime)(); //_dword_BF060C
	//_sub_A61C60(v1); //void __cdecl srand(unsigned int)
	//_sub_44E824();//void __cdecl GetSEPrivilege(void)
	//if (!_dword_BEC3A8)//CSecurityClient* TSingleton<CSecurityClient>::ms_pInstance
	//{
	//	_sub_9F9F42();//CSecurityClient *__cdecl TSingleton<CSecurityClient>::CreateInstance()
	//}

	(*_dword_BF1AC8) = 16;//TSingleton<CConfig>::GetInstance()->m_sysOpt.bSysOpt_WindowedMode;
	_sub_9F6D77(v14);//void __thiscall CWvsApp::InitializePCOM(CWvsApp *this)

	//void __thiscall CWvsApp::CreateMainWindow(CWvsApp *this) //a bit broken...previous fix just resulted in error 0 in my code instead
	_sub_9F6D97(v14); 
	if (!v14->m_hWnd) 
	{ 
		std::cout << "failed to create game window...trying again..." << std::endl;//Sleep(2000);
		fixWnd(); ExitProcess(0);
	} 
	_sub_9F9E53();//CClientSocket *__cdecl TSingleton<CClientSocket>::CreateInstance()
	_sub_9F6F27(v14);//void __thiscall CWvsApp::ConnectLogin(CWvsApp *this)
	_sub_9F9E98();//CFuncKeyMappedMan *__cdecl TSingleton<CFuncKeyMappedMan>::CreateInstance()
	_sub_9FA0CB();//CQuickslotKeyMappedMan *__cdecl TSingleton<CQuickslotKeyMappedMan>::CreateInstance()
	_sub_9F9EEE();//CMacroSysMan *__cdecl TSingleton<CMacroSysMan>::CreateInstance()
	_sub_9F7159_append(v14, nullptr);//void __thiscall CWvsApp::InitializeResMan(CWvsApp *this)

		//displays ad pop-up window before or after the game, cancelling
	//HWND__* v2 = _dword_BF0448();// HWND__ *(__stdcall *GetDesktopWindow)();
	//_dword_BF0444(v2);//int (__stdcall *GetWindowRect)(HWND__ *, tagRECT *);
	//unsigned int v16 = *(DWORD*)(*(DWORD*)_dword_BE7918 + 14320);//ZXString<char> TSingleton<CWvsContext>::ms_pInstance
	//if (v16)
	//{
	//	void* v24 = _sub_403065(_unk_BF0B00, 0x20u);//void *__thiscall ZAllocEx<ZAllocAnonSelector>::Alloc(ZAllocEx<ZAllocAnonSelector> *this, unsigned int uSize)
	//	//v35 = 0;//zref counter
	//	if (v24)
	//	{
	//		v13 = _sub_42C3DE(v29, v30, v31, v32); //too hard to ID in v83
	//	}
	//	else
	//	{
	//		v13 = 0;
	//	}
	//	v25 = v13;
	//	//v35 = -1;//zref counter
	//}

	_sub_9F7A3B(v14);//void __thiscall CWvsApp::InitializeGr2D(CWvsApp *this)
	_sub_9F7CE1_rewrite(v14, nullptr); //void __thiscall CWvsApp::InitializeInput(CWvsApp *this)
	Sleep(300);//_dword_BF02F4(300);//void(__stdcall* Sleep)(unsigned int);
	_sub_9F82BC(v14);//void __thiscall CWvsApp::InitializeSound(CWvsApp *this)
	Sleep(300);//_dword_BF02F4(300);//void(__stdcall* Sleep)(unsigned int);
	_sub_9F8B61(v14);//void __thiscall CWvsApp::InitializeGameData(CWvsApp *this)
	_sub_9F7034(v14);//void __thiscall CWvsApp::CreateWndManager(CWvsApp *this)
	void* vcb = _sub_538C98();//CConfig *__cdecl TSingleton<CConfig>::GetInstance()
	_sub_49EA33(vcb, nullptr, 0);//void __thiscall CConfig::ApplySysOpt(CConfig *this, CONFIG_SYSOPT *pSysOpt, int bApplyVideo)
	void* v3 = _sub_9F9DA6();//CActionMan *__cdecl TSingleton<CActionMan>::CreateInstance()
	_sub_406ABD(v3);//void __thiscall CActionMan::Init(CActionMan *this)
	_sub_9F9DFC();//CAnimationDisplayer *__cdecl TSingleton<CAnimationDisplayer>::CreateInstance()
	void* v4 = _sub_9F9F87();//CMapleTVMan *__cdecl TSingleton<CMapleTVMan>::CreateInstance()
	_sub_636F4E(v4);//void __thiscall CMapleTVMan::Init(CMapleTVMan *this)
	void* v5 = _sub_9F9AC2();//CQuestMan *__cdecl TSingleton<CQuestMan>::CreateInstance()
	if (!_sub_71D8DF(v5))//int __thiscall CQuestMan::LoadDemand(CQuestMan *this)
	{
		//v22 = 570425350;
		//v12 = &v22;
		//v35 = 1;//zref counter
		int v23 = 570425350;
		std::cout << "sub_9F5239 exception " << std::endl;
		_CxxThrowException1(&v23, _TI3_AVCTerminateException__);//_CxxThrowException	//void *pExceptionObject, _s__ThrowInfo*
	}
	_sub_723341(v5);//void __thiscall CQuestMan::LoadPartyQuestInfo(CQuestMan *this) //_dword_BED614
	_sub_7247A1(v5);//void __thiscall CQuestMan::LoadExclusive(CQuestMan *this) //_dword_BED614
	void* v6 = _sub_9F9B73();//CMonsterBookMan *__cdecl TSingleton<CMonsterBookMan>::CreateInstance()
	if (!_sub_68487C(v6))//int __thiscall CMonsterBookMan::LoadBook(CMonsterBookMan *this)
	{
		//v20 = 570425350;
		//v11 = &v20;
		//v35 = 2;//zref counter
		int v21 = 570425350;
		std::cout << "sub_9F5239 exception " << std::endl;
		_CxxThrowException1(&v21, _TI3_AVCTerminateException__);//_CxxThrowException	//void *pExceptionObject, _s__ThrowInfo*
	}
	_sub_9FA078();//CRadioManager *__cdecl TSingleton<CRadioManager>::CreateInstance()

	//@009F5845 in v83 to add Hackshield here if repurposing

	char v34[MAX_PATH];//char sStartPath[MAX_PATH];
	GetModuleFileNameA(NULL, v34, MAX_PATH);//_dword_BF028C(0, &v34, 260);//GetModuleFileNameA(NULL, sStartPath, MAX_PATH);
	_CWvsApp__Dir_BackSlashToSlash_rewrite(v34);//_CWvsApp__Dir_BackSlashToSlash_rewrite(v34);//_CWvsApp__Dir_BackSlashToSlash//_sub_9F95FE
	_sub_9F9644(v34);//_CWvsApp__Dir_upDir
	_sub_9F9621(v34);//void __cdecl CWvsApp::Dir_SlashToBackSlash(char *sDir) //fast way to define functions
	//v19 = &v8;
	//v15 = &v8;
	ZXString<char> v8;
	_sub_414617(&v8, v34, -1);//void __thiscall ZXString<char>::Assign(ZXString<char> *this, const char *s, int n)
	//v35 = 3;//zref counter
	//void* vcb2 = _sub_538C98();//CConfig *__cdecl TSingleton<CConfig>::GetInstance() //redundant
	//v35 = -1;//zref counter
	_sub_49CCF3(vcb, v8);//void __thiscall CConfig::CheckExecPathReg(CConfig *this, ZXString<char> sModulePath)
	void* v17 = _sub_403065(_unk_BF0B00, 0x38u);//void *__thiscall ZAllocEx<ZAllocAnonSelector>::Alloc(ZAllocEx<ZAllocAnonSelector> *this, unsigned int uSize)
	//v35 = 4;//zref counter
	if (v17)
	{
		_sub_62ECE2(v17);//void __thiscall CLogo::CLogo(CLogo *this)
		_sub_777347((CStage*)v17, nullptr);//void __cdecl set_stage(CStage *pStage, void *pParam)
	}
	else
	{
		_sub_777347(nullptr, nullptr);//void __cdecl set_stage(CStage *pStage, void *pParam)
	}
	SetFocus(v14->m_hWnd);
	if (Client::WindowedMode) { SetForegroundWindow(v14->m_hWnd); }
		//likely stuff to check it's on memory, cancelling; add it here if you want to verify client memory
	//v18 = v10;
	//v35 = -1;//zref counter
	//original location//_sub_777347(v10, nullptr);//void __cdecl set_stage(CStage *pStage, void *pParam)
	//v28 = -586879250;
	//for (i = 0; i < 256; ++i)
	//{
	//	v27 = i;
	//	for (j = 8; j > 0; --j)
	//	{
	//		if (v27 & 1)
	//		{
	//			v27 = (v28 - 5421) ^ (v27 >> 1);
	//		}
	//		else
	//		{
	//			v27 >>= 1;
	//		}
	//	}
	//	_dword_BF167C[i] = v27; //unsigned int g_crc32Table[256]
	//}
};
bool Hook_sub_9F5239(bool bEnable)
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F5239;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F5239), _sub_9F5239_rewrite);
}
bool sub_9F5C50_initialized = true;//void __thiscall CWvsApp::Run(CWvsApp *this, int *pbTerminate)
static _sub_9F5C50_t _sub_9F5C50_rewrite = [](CWvsApp* pThis, void* edx, int* pbTerminate) {
	edx = nullptr;
	if (sub_9F5C50_initialized)
	{
		std::cout << "sub_9F5C50 started" << std::endl;
		sub_9F5C50_initialized = false;
	}
	CWvsApp* v4 = pThis;
	tagMSG v17 = tagMSG();
	v17.hwnd = nullptr; //0
	v17.message = 0; //4 //memset(&v18, 0, 0x18u);
	v17.wParam = 0; //8
	v17.lParam = 0; //12
	v17.time = 0; //16
	v17.pt.x = 0; //20 //size 8, 2 longs
	v17.pt.y = 0;
	ISMSG v21 = ISMSG();
	v21.message = 0;
	v21.wParam = 0; //v22
	v21.lParam = 0; //v23
	if (*_dword_BE7914)//CClientSocket *TSingleton<CClientSocket>::ms_pInstance //C64064
	{//void __thiscall CClientSocket::ManipulatePacket(CClientSocket *this)
		_sub_49651D(*_dword_BE7914);//CClientSocket* TSingleton<CClientSocket>::ms_pInstance //C64064
	}
	//v20 = 0; unknown variable //hendi's instructions say to skip it, but for some reason it wasnt skipped in v83, skipping it
	do
	{//unsigned int (__stdcall *MsgWaitForMultipleObjects)(unsigned int, void **, int, unsigned int, unsigned int);
		//static_assert(offsetof(CWvsApp, m_ahInput) == 0x54); //debug val example
		//std::cout << ((int)&v4->m_ahInput - (int)&v4) << std::endl;	 //debug val example	//ty joo for advice and suggestion to use native winapi functions where possible
		//unsigned int v16 = (*_dword_BF04EC)(3, v4->m_ahInput, 0, 0, 255); //working example of using pontered to ZAPI func//C6D9C4 v95
		DWORD v16 = MsgWaitForMultipleObjects(3, v4->m_ahInput, false, 0, 255); //C6D9C4 v9
		if (v16 <= 2)
		{//dword_BEC33C=TSingleton_CInputSystem___ms_pInstance dd ? v95 C68C20
			_sub_59A2E9(*_dword_BEC33C, v16);//void __thiscall CInputSystem::UpdateDevice(CInputSystem *this, int nDeviceIndex)
			do
			{
				if (!_sub_59A306(*_dword_BEC33C, &v21))//int __thiscall CInputSystem::GetISMessage(CInputSystem *this, ISMSG *pISMsg)
				{
					break;
				}
				_sub_9F97B7(v4, v21.message, v21.wParam, v21.lParam);//void __thiscall CWvsApp::ISMsgProc(CWvsApp *this, unsigned int message, unsigned int wParam, int lParam)
			} while (!*pbTerminate);
		}
		else if (v16 == 3)
		{
			do
			{
				if (!PeekMessageA(LPMSG(&v17), nullptr, 0, 0, 1))//_dword_BF04E8//int (__stdcall *PeekMessageA)(tagMSG *, HWND__ *, unsigned int, unsigned int, unsigned int);
				{
					break;
				}
				TranslateMessage((MSG*)(&v17));//_dword_BF0430//int (__stdcall *TranslateMessage)(tagMSG *);
				DispatchMessageA((MSG*)(&v17));//_dword_BF042C//int (__stdcall *DispatchMessageA)(tagMSG *);
				HRESULT v15 = 0;
				if (FAILED(v4->m_hrComErrorCode))//(v4[14])
				{
					v15 = v4->m_hrComErrorCode;//v15 = v4[14];
					v4->m_hrComErrorCode = 0;//v4[14] = 0;
					v4->m_hrZExceptionCode = 0;//v4[13] = 0;
				//	v6 = 1; //removing redundancies, portion is covered by int __thiscall CWvsApp::ExtractComErrorCode(CWvsApp *this, HRESULT *hr) in v95
				//}
				//else
				//{
				//	v6 = 0;
				//}
				//if (v6)
				//{
					_com_raise_error(v15, nullptr);//_sub_A605C3(v15, nullptr);//void __stdcall _com_raise_error(HRESULT hr, IErrorInfo *perrinfo)
				}
				if (FAILED(v4->m_hrZExceptionCode))//if (v4[13])
				{
					v15 = v4->m_hrZExceptionCode;//v15 = v4[13];
					v4->m_hrComErrorCode = 0;//v4[14] = 0;
					v4->m_hrZExceptionCode = 0;//v4[13] = 0;
				//	v5 = 1; //removing redundancies, portion is covered by int __thiscall CWvsApp::ExtractComErrorCode(CWvsApp *this, HRESULT *hr) in v95
				//}
				//else
				//{
				//	v5 = 0;
				//}
				//if (v5)
				//{
					if (v15 == 0x20000000)
					{//create custom error here from struct ZException { const HRESULT m_hr; }; so it doesnt break
						CPatchException v12 = CPatchException();
						//void* v2 = change return to void* if trying other way
						_sub_51E834(&v12, v4->m_nTargetVersion);//v4[16]//void __thiscall CPatchException::CPatchException(CPatchException *this, int nTargetVersion)
						//void* v24 = _ReturnAddress();//v24 = 0; //address of current frame but idk what it's for
						//int v13;
						//memcpy(&v13, v2, 0x508u);
						std::cout << "sub_9F5C50 exception" << std::endl;
						_CxxThrowException1(&v12, _TI3_AVCPatchException__);//&v13
					}
					if (v15 >= 553648128 && v15 <= 553648134)
					{
						//v10 = v15;
						//v24 = 1;//address of one frame up but idk what it's for
						int v11 = v15;
						std::cout << "sub_9F5C50 exception" << std::endl;
						_CxxThrowException1(&v11, _TI3_AVCDisconnectException__);
					}
					if (v15 >= 570425344 && v15 <= 570425357)
					{
						//v8 = v15;
						//v24 = 2;//address of 2 frames up but idk what it's for
						int v9 = v15;
						std::cout << "sub_9F5C50 exception " << v9 << _TI3_AVCTerminateException__ << std::endl;
						_CxxThrowException1(&v9, _TI3_AVCTerminateException__);
					}
					int v7 = v15;
					std::cout << "sub_9F5C50 exception " << v7 << _TI1_AVZException__ << std::endl;
					_CxxThrowException1(&v7, _TI1_AVZException__);
				}
			} while (!*pbTerminate && v17.message != 18);
		}
		else
		{//int __thiscall CInputSystem::GenerateAutoKeyDown(CInputSystem *this, ISMSG *pISMsg)
			if (_sub_59B2D2(*_dword_BEC33C, &v21))//dword_BEC33C=TSingleton_CInputSystem___ms_pInstance dd ? v95 C68C20
			{
				_sub_9F97B7(v4, v21.message, v21.wParam, v21.lParam);//void __thiscall CWvsApp::ISMsgProc(CWvsApp *this, unsigned int message, unsigned int wParam, int lParam)
			}
			//std::cout << "_sub_9F5C50 @ _dword_BF14EC error check" << std::endl;
			if ((*_dword_BF14EC).m_pInterface)//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
			{
				//if (!_dword_BF14EC)//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
				//{ //redundant code
				//	_sub_A5FDE4(-2147467261);//void __stdcall _com_issue_error(HRESULT hr)
				//}				//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
				int v14 = _sub_9F6990((*_dword_BF14EC).m_pInterface);//int __thiscall IWzGr2D::GetnextRenderTime(IWzGr2D *this)
				_sub_9F84D0_rewrite(v4, nullptr, v14);//void __thiscall CWvsApp::CallUpdate(CWvsApp *this, int tCurTime)//_rewrite
				_sub_9E4547();//void __cdecl CWndMan::RedrawInvalidatedWindows(void)
				if (!(*_dword_BF14EC).m_pInterface)//_com_ptr_t<_com_IIID<IWzGr2D,&_GUID_e576ea33_d465_4f08_aab1_e78df73ee6d9> > g_gr
				{
					_com_issue_error(-2147467261);//_sub_A5FDE4(-2147467261);//void __stdcall _com_issue_error(HRESULT hr)
				}//not sure if still needed since the return isnt used
				HRESULT unused_result_vv = _sub_777326((*_dword_BF14EC).m_pInterface);//HRESULT __thiscall IWzGr2D::RenderFrame(IWzGr2D *this)
			}
			Sleep(1);//_dword_BF02F4(1);//void(__stdcall* Sleep)(unsigned int);
		}
	} while (!*pbTerminate && v17.message != 18);
	//_sub_A61DF2(lpMem);//void __cdecl free(void *) //hendi's instructions say to skip it, but for some reason it wasnt skipped in v83, skipping it
	if (v17.message == 18)
	{
		PostQuitMessage(0);//_dword_BF041C(0);//void (__stdcall *PostQuitMessage)(int);
	}
};
bool Hook_sub_9F5C50(bool bEnable)
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F5C50;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F5C50), _sub_9F5C50_rewrite);
}
bool sub_9F4FDA_initialized = true;
static _sub_9F4FDA_t _sub_9F4FDA_rewrite = [](CWvsApp* pThis, void* edx, const char* sCmdLine) {
	if (sub_9F4FDA_initialized)//void __thiscall CWvsApp::CWvsApp(CWvsApp *this, const char *sCmdLine)
	{
		std::cout << "sub_9F4FDA started" << std::endl;
		sub_9F4FDA_initialized = false;
	}
	edx = nullptr;
	CWvsApp* v3 = pThis;//function=void __thiscall CWvsApp::CWvsApp(CWvsApp *this, const char *sCmdLine)
	*_dword_BE7B38 = pThis->m_hWnd != nullptr ? pThis : nullptr;//protected: static class CWvsApp * TSingleton<class CWvsApp>::ms_pInstance
	pThis->m_hWnd = nullptr;//pThis[1] = 0; //unlikely to be wrong because [3] is assigned a unsigned int further down that matches
										//type and name of m_dwMainThreadId
	// 
	//note: the following 2 values are potentially wrongly named because nXXXon added 20 bytes worth of new members to the CWvsApp struct
	//in v95 compared to v83. their offsets should still be right and they will still be used in the right places; UpdateTime and above
	//are correct to the best of my ability, as they have been cross-reference between v83 and v95 in another function
	pThis->m_bPCOMInitialized = 0;//pThis[2] = 0; //could be wrong
	pThis->m_hHook = 0;//pThis[4] = 0; //could be wrong
	pThis->m_tUpdateTime = 0;//pThis[6] = 0;
	pThis->m_bFirstUpdate = 1;//pThis[7] = 1;
	//v19 = 0; //probly ref counter or stack frame counter//[esp+B4h] [ebp-4h]
	pThis->m_sCmdLine = ZXString<char>();//pThis[8] = 0;
	//LOBYTE(v19) = 1;
	pThis->m_nGameStartMode = 0;//pThis[9] = 0;
	pThis->m_bAutoConnect = 1;//pThis[10] = 1;
	pThis->m_bShowAdBalloon = 0;//pThis[11] = 0;
	pThis->m_bExitByTitleEscape = 0;//pThis[12] = 0;
	pThis->m_hrZExceptionCode = 0;//pThis[13] = 0;
	pThis->m_hrComErrorCode = 0;//pThis[14] = 0;
	pThis->vfptr = _off_B3F3E8;//const CWvsApp::`vftable'
	_sub_414617(&(pThis->m_sCmdLine), sCmdLine, -1);//void __thiscall ZXString<char>::Assign(ZXString<char> *this, const char *s, int n)
	ZXString<char>* v4 = _sub_474414(&(v3->m_sCmdLine), "\" ");//ZXString<char> *__thiscall ZXString<char>::TrimRight(ZXString<char> *this, const char *sWhiteSpaceSet)
	_sub_4744C9(v4, "\" ");//ZXString<char> *__thiscall ZXString<char>::TrimLeft(ZXString<char> *this, const char *sWhiteSpaceSet)
	//ZXString<char> v17; //part of part to skip
	//_sub_9F94A1(v3, &v17, 0);//ZXString<char> *__thiscall CWvsApp::GetCmdLine(CWvsApp *this, ZXString<char> *result, int nArg)
	//LOBYTE(v19) = 2;
	//if (v17.IsEmpty())//if (!v17 || !*v17)//!!start of part to skip, according to Hendi's instructions for CWvsapp Ctor
	//{									//for some reason it wasnt skipped in v83, skipping it
	//	goto LABEL_28;
	//}
	//ZXString<char>* v4 = _sub_9F94A1(v3, &((ZXString<char>)sCmdLine), 0);//ZXString<char> *__thiscall CWvsApp::GetCmdLine(CWvsApp *this, ZXString<char> *result, int nArg)
	//ZXString<char> v5(*_string_B3F3D8);// = "WebStart";
	//if (v5.IsEmpty())//!v5
	//{
	//	v5.Empty();//(*_dword_BF6B44);//ZXString<char> ZXString<char>::_s_sEmpty
	//}
	//ZXString<char> v6 = *v4;
	//if (v6.IsEmpty())//(!v6)
	//{
	//	v6.Empty();//v6 = *_dword_BF6B44;//ZXString<char> ZXString<char>::_s_sEmpty
	//}
	//int v7 = strcmp(v6, v5) == 0;
	//((ZXString<char>)sCmdLine).~ZXString();//_sub_4062DF(&sCmdLine);//void __cdecl ZXString<char>::_Release(ZXString<char>::_ZXStringData *pData)
	//if (v7)
	//{
	//	_sub_9F94A1(v3, &((ZXString<char>)sCmdLine), 1);//ZXString<char> *__thiscall CWvsApp::GetCmdLine(CWvsApp *this, ZXString<char> *result, int nArg)
	//	v3->m_nGameStartMode = sCmdLine && *sCmdLine;
	//	((ZXString<char>)sCmdLine).~ZXString();//_sub_4062DF(&sCmdLine);//void __cdecl ZXString<char>::_Release(ZXString<char>::_ZXStringData *pData)
	//}
	//else
	//{
	//LABEL_28://end of part to skip
	//	v3->m_nGameStartMode = 2; //remove the one below if not skipping
	//} //part of part to skip
	v3->m_nGameStartMode = 2;	//unlikely to be wrong, matches type and name of m_dwMainThreadId
	v3->m_dwMainThreadId = GetCurrentThreadId();//_dword_BF02B4();//unsigned int (__stdcall *GetCurrentThreadId)();
	OSVERSIONINFOA v13 = OSVERSIONINFOA();
	v13.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA((LPOSVERSIONINFOA)(&v13));//_dword_BF03E4(&v13);//int (__stdcall *GetVersionExA)(_OSVERSIONINFOA *);
	v3->m_bWin9x = v13.dwPlatformId == 1; //at memory byte 20 of v3 struct //could also be wrong
	if (v13.dwMajorVersion > 6 && !v3->m_nGameStartMode)
	{
		v3->m_nGameStartMode = 2;
	}
	if (v13.dwMajorVersion < 5)
	{
		*_dword_BE2EBC = 1996;//unsigned int g_dwTargetOS
	} v42 = L"EzorsiaV2_UI.wz";
	HMODULE v9 = GetModuleHandleA("kernel32.dll");//sub_44E88E=//int (__stdcall *__stdcall MyGetProcAddress(HINSTANCE__ *hModule, const char *lpProcName))()
	auto v10 = (void(__stdcall*)(HANDLE, int*))_sub_44E88E_rewrite(v9, "IsWow64Process"); //tough definition, one of a kind
	int v18 = 0;
	if (v10)
	{
		HANDLE v11 = GetCurrentProcess();
		v10(v11, &v18);
		if (v18 != 0)
		{
			*_dword_BE2EBC = 1996;
		}
	}
	if (v13.dwMajorVersion >= 6 && v18 == 0)
	{
		_sub_44ED47_rewrite();//void __cdecl ResetLSP(void)
	}
	//LOBYTE(v19) = 1;
	//v17.~ZXString();//_sub_4062DF(&v17);//void __cdecl ZXString<char>::_Release(ZXString<char>::_ZXStringData *pData)
};	//2 //^part of part to skip
bool Hook_sub_9F4FDA(bool bEnable)	//1
{
	BYTE firstval = 0x7D;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F4FDC;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F4FDA), _sub_9F4FDA_rewrite);	//2
}
static _sub_9F51F6_t _sub_9F51F6_Hook = [](CWvsApp* pThis, void* edx) {
	std::cout << "sub_9F51F6 started: CWvsapp dieing" << std::endl;
	_sub_9F51F6(pThis, nullptr);
};
bool Hook_sub_9F51F6(bool bEnable)	//1
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F51F6;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F51F6), _sub_9F51F6_Hook);	//2
}
//bool uiIntercepted_sub_9F79B8 = false;
//void* pThePackage = nullptr;
//Ztl_variant_t pBaseData = Ztl_variant_t();
//IUnknown* pSubUnknown = nullptr;
//void* pSubArchive = nullptr;
//
//
//_sub_5D995B(pDataFileSystem, nullptr, &pBaseData, BmyWzPath);//Ztl_variant_t *__thiscall IWzNameSpace::Getitem(IWzNameSpace *this, Ztl_variant_t *result, Ztl_bstr_t sPath)
//pSubUnknown = _sub_4032B2(&pBaseData, nullptr, FALSE, FALSE);//IUnknown* __thiscall Ztl_variant_t::GetUnknown(Ztl_variant_t* this, bool fAddRef, bool fTryChangeType)
//_sub_9FCD88(pSubArchive, nullptr, pSubUnknown);//void __thiscall <IWzSeekableArchive(IWzSeekableArchive* this, IUnknown* p)
//
//
//static _sub_9F79B8_t _sub_9F79B8_Hook = [](void* pThis, void* edx, Ztl_bstr_t sKey, Ztl_bstr_t sBaseUOL, void* pArchive) {//sub_9F79B8
//	if (uiIntercepted_sub_9F79B8) {
//		uiIntercepted_sub_9F79B8 = false;
//		_sub_9FB084(L"NameSpace#Package", &pThePackage, NULL);//void __cdecl PcCreateObject_IWzPackage(const wchar_t *sUOL, ??? *pObj, IUnknown *pUnkOuter)
//		_sub_9F79B8(pThePackage, nullptr, sKey, sBaseUOL, pSubArchive); //HRESULT __thiscall IWzPackage::Init(IWzPackage *this, Ztl_bstr_t sKey, Ztl_bstr_t sBaseUOL, IWzSeekableArchive *pArchive)
//	}
//	return _sub_9F79B8(pThis, nullptr, sKey, sBaseUOL, pArchive); //HRESULT __thiscall IWzPackage::Init(IWzPackage *this, Ztl_bstr_t sKey, Ztl_bstr_t sBaseUOL, IWzSeekableArchive *pArchive)
//};
//bool Hook_sub_9F79B8(bool bEnable)	//1
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F79B8), _sub_9F79B8_Hook);	//2
//}
//bool Hook_sub_9F51F6(bool bEnable)	//1
//{
//	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F51F6), _sub_9F51F6_Hook);	//2
//}
static _IWzFileSystem__Init_t _sub_9F7964_Hook = [](void* pThis, void* edx, Ztl_bstr_t sPath) {
	//if (resmanSTARTED) {//HRESULT __thiscall IWzFileSystem::Init(IWzFileSystem *this, Ztl_bstr_t sPath)
	//	std::cout << "_IWzFileSystem__Init of resMAN started" << std::endl;
	//}
	edx = nullptr;	//function does nothing.., can replace return with S_OK and it works
	void* v2 = pThis; // esi
	wchar_t* v3 = 0; // eax
	HRESULT v5; // edi
	v13 = v42; //ebp

	if (sPath.m_Data)
	{
		v3 = sPath.m_Data->m_wstr;
	}
	auto v4 = (*(int(__stdcall**)(void*, wchar_t*))(*(DWORD*)pThis + 52));//overloaded unknown funct at offset 52 of IWzFileSystem
	v4(pThis, v3);//seems to do nothing and just check the input, works if not run
	v5 = (HRESULT)v4;
	if ((HRESULT)v4 < 0)
	{
		_com_issue_errorex((HRESULT)v4, (IUnknown*)v2, *_unk_BE2EC0);//GUID _GUID_352d8655_51e4_4668_8ce4_0866e2b6a5b5
	}
	if (sPath.m_Data)
	{
		_sub_402EA5(sPath.m_Data);
	}
	return v5;
	//return _sub_9F7964(pThis, nullptr, sPath);//HRESULT __thiscall IWzFileSystem::Init(IWzFileSystem *this, Ztl_bstr_t sPath)
};
bool Hook_sub_9F7964(bool bEnable)	//1
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F7964;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F7964), _sub_9F7964_Hook);	//2
}
static _sub_9FCD88_t _sub_9FCD88_Hook = [](void* pThis, void* edx, IUnknown* p) {
	//if (resmanSTARTED) {
	//	std::cout << "_sub_9FCD88 of resMAN started" << std::endl;
	//}
	_sub_9FCD88(pThis, nullptr, p);//void __thiscall <IWzSeekableArchive(IWzSeekableArchive* this, IUnknown* p)
};
bool Hook_sub_9FCD88(bool bEnable)	//1
{
	BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009FCD88;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9FCD88), _sub_9FCD88_Hook);	//2
}
bool ZSecureCrypt_Init = false;
static _sub_5D995B_t _sub_5D995B_Hook = [](void* pThis, void* edx, Ztl_variant_t* result, Ztl_bstr_t sPath) {
	//if (resmanSTARTED) {//Ztl_variant_t *__thiscall IWzNameSpace::Getitem(IWzNameSpace *this, Ztl_variant_t *result, Ztl_bstr_t sPath)
	//	std::cout << "_sub_5D995B of resMAN started" << std::endl;
	//}
	edx = nullptr;
	void* v3 = pThis; // esi
	const wchar_t* v4 = 0; // eax
	Ztl_variant_t pvarg = Ztl_variant_t(); // [esp+8h] [ebp-20h]

	VariantInit(&pvarg);
	if (sPath.m_Data)
	{
		v4 = sPath.m_Data->m_wstr;
	}
	//std::cout << "_sub_5D995B vals: " << *(DWORD*)v3 << " / " << *v4 << " / " << *(DWORD*)(&pvarg) << std::endl;
	auto v5 = (*(int(__stdcall**)(void*, const wchar_t*, Ztl_variant_t*))(*(DWORD*)v3 + 12));//unknown virtual function at offset 12 of IWzNameSpace
	if (!ZSecureCrypt_Init && MainMain::usingEzorsiaV2Wz)
	{
		ZSecureCrypt_Init = true; v4 = v13;
	}
	v5(v3, v4, &pvarg);
	//std::cout << "_sub_5D995B vals: " << *(DWORD*)v3 << " / " << *v4 << " / " << *(DWORD*)(&pvarg) << std::endl;//Sleep(22000);
	if ((HRESULT)v5 < 0)
	{
		_com_issue_errorex((HRESULT)v5, (IUnknown*)v3, *_unk_BD8F28); ///GUID _GUID_2aeeeb36_a4e1_4e2b_8f6f_2e7bdec5c53d
	}
	_sub_4039AC(result, &pvarg, 0);//non-existent func in v95//int __thiscall sub_4039AC(VARIANTARG *pvargDest, VARIANTARG *pvargSrc, char) //works with v95 overwrite//memcpy_s(result, 0x10u, &pvarg, 0x10u);//_sub_4039AC(result, &pvarg, 0); //works with v95 overwrite
	pvarg.vt = 0;
	if (sPath.m_Data)
	{
		_sub_402EA5(sPath.m_Data);//unsigned int __thiscall _bstr_t::Data_t::Release(_bstr_t::Data_t *this)
	}
	return result;
	//return _sub_5D995B(pThis, nullptr, result, sPath);
};
//bool sub_9F4E54_initialized = true; //unsigned int __cdecl Crc32_GetCrc32_VMTable(unsigned int* pmem, unsigned int size, unsigned int* pcheck, unsigned int *pCrc32) 
static _sub_9F4E54_t _sub_9F4E54_Hook = [](unsigned int* pmem, void* edx, unsigned int size, unsigned int* pcheck, unsigned int* pCrc32) {
	//if (sub_9F4E54_initialized)
	//{
		std::cout << "!!!WARNING!!! _sub_9F4E54 anonymously called !!!WARNING!!!" << std::endl;
		//sub_9F4E54_initialized = false;
	//}
	edx = nullptr;
	unsigned int result = *pCrc32;
	//v4 = pCrc32; /	/disabled this part just in case its anonymously called, rewrite if you want CrC
	//v6 = 0;
	//v7 = a2 >> 1;
	//v9 = a2 >> 1;
	//v8 = 0;
	//if (a2)
	//{
	//	while (1)
	//	{
	//		if (v6 == v7)
	//		{
	//			v9 = 0;
	//			*v4 = ((unsigned int)*v4 >> 8) ^ _dword_BF167C[*v4 & 0xFF ^ *(unsigned __int8*)(v6 + a1)]; //unsigned int g_crc32Table[256]
	//			*a3 = 811;
	//			result = *v4 + 1;
	//		}
	//		else
	//		{
	//			*v4 = ((unsigned int)*v4 >> 8) ^ _dword_BF167C[*v4 & 0xFF ^ *(unsigned __int8*)(v6 + a1)]; //unsigned int g_crc32Table[256]
	//			v6 = v8;
	//		}
	//		v8 = ++v6;
	//		if (v6 >= a2)
	//		{
	//			break;
	//		}
	//		v7 = v9;
	//	}
	//}
	return result;
};
bool Hook_sub_9F4E54(bool bEnable)	//1
{
	BYTE firstval = 0x55;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x009F4E54;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_9F4E54), _sub_9F4E54_Hook);	//2
}
bool Hook_sub_5D995B(bool bEnable)	//1
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x005D995B;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_5D995B), _sub_5D995B_Hook);	//2
}
static _sub_4032B2_t _sub_4032B2_Hook = [](Ztl_variant_t* pThis, void* edx, bool fAddRef, bool fTryChangeType) {
	//if (resmanSTARTED) {
	//	std::cout << "_sub_4032B2 of resMAN started" << std::endl;
	//}
	return _sub_4032B2(pThis, nullptr, fAddRef, fTryChangeType);//IUnknown* __thiscall Ztl_variant_t::GetUnknown(Ztl_variant_t* this, bool fAddRef, bool fTryChangeType)
};
bool Hook_sub_4032B2(bool bEnable)	//1
{
	BYTE firstval = 0xB8;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x004032B2;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_4032B2), _sub_4032B2_Hook);	//2
}
static _sub_425ADD_t _sub_425ADD_Hook = [](Ztl_bstr_t* pThis, void* edx, const char* str) {
	//if (resmanSTARTED) {
	//	std::cout << "resman-bstr_t wchar val: " << str << std::endl;
	//}
	return _sub_425ADD(pThis, nullptr, str); //HRESULT __thiscall IWzPackage::Init(IWzPackage *this, Ztl_bstr_t sKey, Ztl_bstr_t sBaseUOL, IWzSeekableArchive *pArchive)
};
bool Hook_sub_425ADD(bool bEnable)	//1
{
	BYTE firstval = 0x56;  //this part is necessary for hooking a client that is themida packed
	DWORD dwRetAddr = 0x00425ADD;	//will crash if you hook to early, so you gotta check the byte to see
	while (1) {						//if it matches that of an unpacked client
		if (ReadValue<BYTE>(dwRetAddr) != firstval) { Sleep(1); } //figured this out myself =)
		else { break; }
	}//void __thiscall Ztl_bstr_t::Ztl_bstr_t(Ztl_bstr_t *this, const char *s)
	return Memory::SetHook(bEnable, reinterpret_cast<void**>(&_sub_425ADD), _sub_425ADD_Hook);	//2
}
//#pragma optimize("", on)