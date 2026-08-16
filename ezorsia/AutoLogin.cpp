#include "stdafx.h"
#include "AutoLogin.h"
#include "Memory.h"

namespace {
constexpr DWORD kCLoginUpdateAddr = 0x005F4C16;
constexpr DWORD kSendCheckPasswordPacketAddr = 0x005F6952;
constexpr DWORD kSendSelectWorldAddr = 0x005F6D6A;

constexpr DWORD kOffsetStep = 0x168;
constexpr DWORD kOffsetStepTime = 0x16C;
constexpr DWORD kOffsetRequestSent = 0x170;
constexpr DWORD kOffsetWorldItem = 0x18C;

enum class AutoLoginPhase {
    INIT,
    WAIT_LOGIN_READY,
    LOGIN_PACKET_SENT,
    WAIT_WORLD_READY,
    WORLD_PACKET_SENT,
    CHAR_SELECT_REACHED,
    FAILED
};

static bool g_bEnabled = false;
static std::string g_Username;
static std::string g_Password;
static int g_nWorld = 0;
static int g_nChannel = 0;

static AutoLoginPhase g_Phase = AutoLoginPhase::INIT;
static DWORD g_dwPhaseStartTick = 0;

using CLogin__Update_t = void(__fastcall*)(void* pThis, void* edx);
static CLogin__Update_t s_CLogin__Update = reinterpret_cast<CLogin__Update_t>(kCLoginUpdateAddr);

using SendCheckPasswordPacket_t = int(__fastcall*)(void* pThis, void* edx, const char* sID, const char* sPasswd);
static SendCheckPasswordPacket_t s_SendCheckPasswordPacket = reinterpret_cast<SendCheckPasswordPacket_t>(kSendCheckPasswordPacketAddr);

using SendSelectWorld_t = int(__fastcall*)(void* pThis, void* edx, int nWorldId, int nChannelId);
static SendSelectWorld_t s_SendSelectWorld = reinterpret_cast<SendSelectWorld_t>(kSendSelectWorldAddr);

static void __fastcall CLogin__Update_Hook(void* pThis, void* edx) {
    if (s_CLogin__Update) {
        s_CLogin__Update(pThis, edx);
    }

    if (!g_bEnabled || pThis == nullptr) {
        return;
    }

    if (g_Username.empty()) {
        return;
    }

    __try {
        const DWORD now = GetTickCount();
        const int nStep = *reinterpret_cast<const int*>((uintptr_t)pThis + kOffsetStep);
        const DWORD tStepTime = *reinterpret_cast<const DWORD*>((uintptr_t)pThis + kOffsetStepTime);
        const int bRequestSent = *reinterpret_cast<const int*>((uintptr_t)pThis + kOffsetRequestSent);

        switch (g_Phase) {
        case AutoLoginPhase::INIT:
            if (nStep == 0) {
                g_Phase = AutoLoginPhase::WAIT_LOGIN_READY;
                g_dwPhaseStartTick = now;
            }
            break;

        case AutoLoginPhase::WAIT_LOGIN_READY:
            if (nStep == 0 && tStepTime == 0 && bRequestSent == 0) {
                if (now - g_dwPhaseStartTick >= 500) {
                    s_SendCheckPasswordPacket(pThis, nullptr, g_Username.c_str(), g_Password.c_str());
                    g_Phase = AutoLoginPhase::LOGIN_PACKET_SENT;
                    g_dwPhaseStartTick = now;
                }
            }
            break;

        case AutoLoginPhase::LOGIN_PACKET_SENT:
            if (nStep == 1) {
                g_Phase = AutoLoginPhase::WAIT_WORLD_READY;
                g_dwPhaseStartTick = now;
            } else if (now - g_dwPhaseStartTick > 15000) {
                g_Phase = AutoLoginPhase::FAILED;
            }
            break;

        case AutoLoginPhase::WAIT_WORLD_READY:
            if (nStep == 1 && tStepTime == 0 && bRequestSent == 0) {
                const DWORD worldItems = *reinterpret_cast<const DWORD*>((uintptr_t)pThis + kOffsetWorldItem);
                if (worldItems != 0 && (now - g_dwPhaseStartTick >= 300)) {
                    s_SendSelectWorld(pThis, nullptr, g_nWorld, g_nChannel);
                    g_Phase = AutoLoginPhase::WORLD_PACKET_SENT;
                    g_dwPhaseStartTick = now;
                }
            }
            break;

        case AutoLoginPhase::WORLD_PACKET_SENT:
            if (nStep == 2) {
                // Arrived at character selection screen; stop and stay here.
                g_Phase = AutoLoginPhase::CHAR_SELECT_REACHED;
            } else if (now - g_dwPhaseStartTick > 15000) {
                g_Phase = AutoLoginPhase::FAILED;
            }
            break;

        case AutoLoginPhase::CHAR_SELECT_REACHED:
        case AutoLoginPhase::FAILED:
        default:
            break;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // Suppress any memory read/write exception to safeguard game client stability
    }
}
} // namespace

namespace AutoLogin {

void Init(bool bEnable, const std::string& username, const std::string& password, int nWorld, int nChannel) {
    g_bEnabled = bEnable;
    g_Username = username;
    g_Password = password;
    g_nWorld = nWorld;
    g_nChannel = nChannel;
    g_Phase = AutoLoginPhase::INIT;
    g_dwPhaseStartTick = GetTickCount();
}

void Hook(bool bEnable) {
    Memory::SetHook(bEnable, reinterpret_cast<void**>(&s_CLogin__Update), CLogin__Update_Hook);
}

} // namespace AutoLogin
