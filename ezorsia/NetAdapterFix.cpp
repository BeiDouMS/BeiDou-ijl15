#include "stdafx.h"
#include "NetAdapterFix.h"
#include "detours.h"

namespace {

// IP_ADAPTER_INFO 的 ABI（不引 iphlpapi.h，避免多一个头/库依赖）。
// 关键偏移已经和客户端反汇编逐个核对过：Next=+0x000、Address=+0x194、Type=+0x1A0，
// 单条 640 字节 —— 客户端那个 0x2800 的缓冲区正好放 16 条。
struct IpAdapterInfo
{
	IpAdapterInfo* Next;        // +0x000
	DWORD ComboIndex;           // +0x004
	char  AdapterName[260];     // +0x008
	char  Description[132];     // +0x10C
	UINT  AddressLength;        // +0x190
	BYTE  Address[8];           // +0x194
	DWORD Index;                // +0x19C
	UINT  Type;                 // +0x1A0
	UINT  DhcpEnabled;          // +0x1A4
	void* CurrentIpAddress;     // +0x1A8
	BYTE  Rest[0x280 - 0x1AC];  // IpAddressList / GatewayList / ... 一律清零
};

const UINT kIfTypeEthernet = 6;   // MIB_IF_TYPE_ETHERNET，客户端就是拿它跟 6 比

typedef DWORD(WINAPI* GetAdaptersInfo_t)(IpAdapterInfo*, PULONG);
GetAdaptersInfo_t oGetAdaptersInfo = nullptr;

// 造一个稳定的本地管理 MAC：同一台机器每次都一样（服务器可能拿它认机器）。
// 首字节 0x02 = 本地管理 + 单播，不会和真实网卡冲突，也不会撞上客户端里
// 那个 "DEST" 特判（'D' = 0x44 开头）。
void BuildStableMac(BYTE* address)
{
	DWORD hash = 0x811C9DC5;

	DWORD volumeSerial = 0;
	if (GetVolumeInformationA("C:\\", nullptr, 0, &volumeSerial, nullptr, nullptr, nullptr, 0)) {
		hash = (hash ^ volumeSerial) * 16777619u;
	}

	char computer[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	DWORD computerLength = sizeof(computer);
	if (GetComputerNameA(computer, &computerLength)) {
		for (DWORD i = 0; i < computerLength; ++i) {
			hash = (hash ^ (BYTE)computer[i]) * 16777619u;
		}
	}

	address[0] = 0x02;
	address[1] = (BYTE)(hash >> 24);
	address[2] = (BYTE)(hash >> 16);
	address[3] = (BYTE)(hash >> 8);
	address[4] = (BYTE)hash;
	address[5] = 0x01;
}

DWORD WINAPI HkGetAdaptersInfo(IpAdapterInfo* adapterInfo, PULONG outBufLen)
{
	// 真实调用可能会改写 *outBufLen（比如报所需大小），所以先记下调用方给的容量
	const ULONG capacity = (outBufLen != nullptr) ? *outBufLen : 0;

	const DWORD result = oGetAdaptersInfo(adapterInfo, outBufLen);
	if (result == ERROR_SUCCESS) {
		return result;   // 正常情况：一个字节都不动
	}

	// 失败了。客户端不看返回值，会直接把这块未初始化的栈内存当链表遍历 -> 野指针崩溃。
	// 缓冲区放得下一条就给它造一条合法的。
	if (adapterInfo == nullptr || capacity < sizeof(IpAdapterInfo)) {
		return result;
	}

	ZeroMemory(adapterInfo, sizeof(IpAdapterInfo));
	adapterInfo->Next = nullptr;
	adapterInfo->ComboIndex = 1;
	adapterInfo->Index = 1;
	adapterInfo->Type = kIfTypeEthernet;
	adapterInfo->AddressLength = 6;
	BuildStableMac(adapterInfo->Address);
	strcpy_s(adapterInfo->AdapterName, "{00000000-0000-0000-0000-000000000000}");
	strcpy_s(adapterInfo->Description, "Ethernet Adapter");

	if (outBufLen != nullptr) {
		*outBufLen = sizeof(IpAdapterInfo);
	}
	return ERROR_SUCCESS;
}

}   // namespace

void NetAdapterFix_Install(bool enable)
{
	if (!enable) {
		return;
	}

	// 客户端静态导入了 iphlpapi，所以这时候它已经映射进来了；
	// 优先用 GetModuleHandle，免得在 DllMain 里 LoadLibrary 踩 loader lock
	HMODULE iphlpapi = GetModuleHandleA("iphlpapi.dll");
	if (iphlpapi == nullptr) {
		iphlpapi = LoadLibraryA("iphlpapi.dll");
	}
	if (iphlpapi == nullptr) {
		return;
	}

	oGetAdaptersInfo = (GetAdaptersInfo_t)GetProcAddress(iphlpapi, "GetAdaptersInfo");
	if (oGetAdaptersInfo == nullptr) {
		return;
	}

	if (!Memory::SetHook(true, (void**)&oGetAdaptersInfo, (void*)HkGetAdaptersInfo)) {
		oGetAdaptersInfo = nullptr;
	}
}
