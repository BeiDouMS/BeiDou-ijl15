// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "NMCO.h"
#include "ijl15.h"
#include "INIReader.h"
#include "ReplacementFuncs.h"
#include <comutil.h>
#include "BossHP.h"
#include "HpMpAlert.h"
#include "SelectCharMacFix.h"
#include "AutoLogin.h"
#pragma comment(lib, "ws2_32.lib")

// config.ini can use IP or hostname (ServerIP_Address=...).
// The patch expects an IPv4 dotted string; resolve hostnames to IPv4.
// On failure, fall back to the original value.
static std::string ResolveToIpv4String(const std::string& hostOrIp)
{
	if (hostOrIp.empty()) return hostOrIp;

	IN_ADDR parsedAddr{};
	if (InetPtonA(AF_INET, hostOrIp.c_str(), &parsedAddr) == 1) {
		return hostOrIp;
	}

	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return hostOrIp;
	}

	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	addrinfo* result = nullptr;
	const int gaiRc = getaddrinfo(hostOrIp.c_str(), nullptr, &hints, &result);
	if (gaiRc != 0 || result == nullptr) {
		WSACleanup();
		return hostOrIp;
	}

	char ipBuf[INET_ADDRSTRLEN]{};
	const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(result->ai_addr);
	const PCSTR ipStr = InetNtopA(AF_INET, const_cast<IN_ADDR*>(&ipv4->sin_addr), ipBuf, sizeof(ipBuf));

	freeaddrinfo(result);
	WSACleanup();

	if (ipStr == nullptr) {
		return hostOrIp;
	}

	return std::string(ipStr);
}

void CreateConsole() {
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout); //CONOUT$
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	{
		//CreateConsole();	//console for devs, use this to log stuff if you want
		INIReader reader("config.ini");
		if (reader.ParseError() == 0) {
			Client::m_nGameWidth = reader.GetInteger("general", "width", 1280);
			Client::m_nGameHeight = reader.GetInteger("general", "height", 720);
			Client::MsgAmount = reader.GetInteger("general", "MsgAmount", 26);
			Client::CustomLoginFrame = reader.GetBoolean("general", "CustomLoginFrame", true);
			Client::WindowedMode = reader.GetBoolean("general", "WindowedMode", true);
			Client::RemoveLogos = reader.GetBoolean("general", "RemoveLogos", true);
			Memory::UseVirtuProtect = reader.GetBoolean("general", "UseVirtuProtect", true);
			Client::setDamageCap = reader.GetReal("optional", "setDamageCap", 199999);
			Client::setMAtkCap = reader.GetReal("optional", "setMAtkCap", 1999);
			Client::setAccCap = reader.GetReal("optional", "setAccCap", 999);
			Client::setAvdCap = reader.GetReal("optional", "setAvdCap", 999);
			Client::setAtkOutCap = reader.GetReal("optional", "setAtkOutCap", 199999);
			Client::useTubi = reader.GetBoolean("optional", "useTubi", false);
			Client::bigLoginFrame = reader.GetBoolean("general", "bigLoginFrame", false);
			Client::SwitchChinese = reader.GetBoolean("general", "SwitchChinese", false);
			Client::speedMovementCap = reader.GetInteger("optional", "speedMovementCap", 140);
			Client::jumpCap = reader.GetInteger("optional", "jumpCap", 123);
			Client::debug = reader.GetBoolean("debug", "debug", false);
			Client::noPassword = reader.GetBoolean("debug", "noPassword", false);
			Client::imeType = reader.GetInteger("general", "imeType", 1);
			ownLoginFrame = reader.GetBoolean("optional", "ownLoginFrame", false);
			ownCashShopFrame = reader.GetBoolean("optional", "ownCashShopFrame", false);
			EzorsiaV2WzIncluded = reader.GetBoolean("general", "EzorsiaV2WzIncluded", true);
			Client::ServerIP_AddressFromINI = ResolveToIpv4String(reader.Get("general", "ServerIP_Address", "127.0.0.1"));
			Client::serverIP_Port = reader.GetInteger("general", "serverIP_Port", 8484);
			Client::climbSpeedAuto = reader.GetBoolean("optional", "climbSpeedAuto", false);
			Client::climbSpeed = reader.GetFloat("optional", "climbSpeed", 1.0);
			Client::talkRepeat = reader.GetBoolean("optional", "talkRepeat", false);
			Client::talkTime = reader.GetInteger("optional", "talkTime", 2000);

			bool autoLogin = reader.GetBoolean("general", "AutoLogin", false);
			if (!autoLogin) {
				autoLogin = reader.GetBoolean("auto_login", "enable", false);
			}
			std::string autoLoginUsername = reader.Get("general", "AutoLogin_Username", "");
			if (autoLoginUsername.empty()) {
				autoLoginUsername = reader.Get("auto_login", "username", "");
			}
			std::string autoLoginPassword = reader.Get("general", "AutoLogin_Password", "");
			if (autoLoginPassword.empty()) {
				autoLoginPassword = reader.Get("auto_login", "password", "");
			}
			int autoLoginWorld = reader.GetInteger("general", "AutoLogin_World", 0);
			if (autoLoginWorld == 0) {
				autoLoginWorld = reader.GetInteger("auto_login", "world", 0);
			}
			int autoLoginChannel = reader.GetInteger("general", "AutoLogin_Channel", 0);
			if (autoLoginChannel == 0) {
				autoLoginChannel = reader.GetInteger("auto_login", "channel", 0);
			}
			AutoLogin::Init(autoLogin, autoLoginUsername, autoLoginPassword, autoLoginWorld, autoLoginChannel);
		}

		Hook_CreateMutexA(true); //multiclient //ty darter, angel, and alias!
		HookCreateWindowExA(true); //default ezorsia
		HookGetModuleFileName(true); //default ezorsia
		HookPcCreateObject_IWzResMan(true);
		HookPcCreateObject_IWzNameSpace(true);
		HookPcCreateObject_IWzFileSystem(true);
		HookCWvsApp__Dir_BackSlashToSlash(true);
		HookCWvsApp__Dir_upDir(true);
		Hookbstr_ctor(true);
		HookIWzFileSystem__Init(true);
		HookIWzNameSpace__Mount(true);
		HookCWvsApp__InitializeResMan(false); //experimental //ty to all the contributors of the ragezone release: Client load .img instead of .wz v62~v92
		Hook_StringPool__GetString(true); //hook stringpool modification //ty !! popcorn //ty darter
		Hook_lpfn_NextLevel(true);
		HookSaveGlobal(true);
		HookHpMpAlertRecv(true);
		HookSelectCharMacFix(true);
		AutoLogin::Hook(true);
		//Hook_get_unknown(true);
		//Hook_get_resource_object(true); //helper function hooks  //ty teto for helping me get started
		//Hook_com_ptr_t_IWzProperty__ctor(true);
		//Hook_com_ptr_t_IWzProperty__dtor(true);

		Client::UpdateGameStartup();

		std::cout << "Applying resolution " << Client::m_nGameWidth << "x" << Client::m_nGameHeight << std::endl;
		Client::UpdateResolution();
		Client::FixMouseWheel();
		Client::Chinese();
		Client::LongQuickSlot();
		Client::FixDateFormat();
		Client::FixItemType();
		Client::JumpCap();
		Client::FixChatPosHook();
		Client::NoPassword();
		Client::MoreHook();
		BossHP::Hook();
		Client::WorldMap();
		Client::RefreshRate(); 
		Client::DeleteChar();
		std::cout << "GetModuleFileName hook created" << std::endl;
		ijl15::CreateHook(); //NMCO::CreateHook();

		std::cout << "NMCO hook initialized" << std::endl;
		break;
	}
	default: break;
	case DLL_PROCESS_DETACH:
		ExitProcess(0);
	}
	return TRUE;
}





