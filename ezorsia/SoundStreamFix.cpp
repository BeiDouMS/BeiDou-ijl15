#include "stdafx.h"
#include "SoundStreamFix.h"
#include "detours.h"

namespace {

// 不引 msacm.h：WIN32_LEAN_AND_MEAN 把 mmsystem 排掉了，为这几个函数再拉一套头进来不划算。
typedef void* HACMSTREAM;

struct WaveFormatEx           // WAVEFORMATEX
{
	WORD  wFormatTag;
	WORD  nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD  nBlockAlign;
	WORD  wBitsPerSample;
	WORD  cbSize;
};

// acmStreamOpen 的 fdwOpen 位
const DWORD kAcmOpenQuery = 0x00000001;     // 只是问问这个格式能不能转，不给句柄
const DWORD kAcmOpenAsync = 0x00000002;     // 异步，带回调，不参与复用

const UINT  kMmsysNoError = 0;              // MMSYSERR_NOERROR
const WORD  kWaveFormatPcm = 1;             // WAVE_FORMAT_PCM

typedef UINT(WINAPI* acmStreamOpen_t)(HACMSTREAM*, void*, WaveFormatEx*, WaveFormatEx*, void*, DWORD_PTR, DWORD_PTR, DWORD);
typedef UINT(WINAPI* acmStreamClose_t)(HACMSTREAM, DWORD);
typedef UINT(WINAPI* acmStreamReset_t)(HACMSTREAM, DWORD);

acmStreamOpen_t  oAcmStreamOpen = nullptr;
acmStreamClose_t oAcmStreamClose = nullptr;
acmStreamReset_t pAcmStreamReset = nullptr;     // 只调用，不挂钩

CRITICAL_SECTION g_cs;

// ---- 池键 -------------------------------------------------------------------
// 键 = 驱动句柄 + fdwOpen + 源格式 + 目标格式。只有这四样全都一致，
// 上一条流才能原样交给下一次打开用。

inline unsigned __int64 FnvMix(unsigned __int64 h, const void* p, size_t n)
{
	const BYTE* b = (const BYTE*)p;
	for (size_t i = 0; i < n; ++i) {
		h ^= b[i];
		h *= 1099511628211ULL;
	}
	return h;
}

// 把一个 WAVEFORMATEX 揉进哈希。
// 非 PCM 的格式后面还挂着 cbSize 个字节的扩展字段，MPEGLAYER3 的码率就藏在那儿，
// 必须一起算，否则两种码率的音效会被当成同一条流复用。
// 格式指针来自客户端，用 __try 兜一下（这个函数里因此不能有需要析构的对象，C2712）。
bool HashFormat(unsigned __int64* h, const WaveFormatEx* wf)
{
	if (wf == nullptr) {
		*h = FnvMix(*h, "<null>", 6);
		return true;
	}
	__try {
		DWORD n = 16;                       // PCM 只有固定的 16 字节，cbSize 可能都不存在
		if (wf->wFormatTag != kWaveFormatPcm) {
			n = sizeof(WaveFormatEx) + wf->cbSize;
		}
		if (n > 256) {
			return false;                   // cbSize 离谱，当作不可复用
		}
		*h = FnvMix(*h, wf, n);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
	return false;
}

bool MakePoolKey(void* driver, const WaveFormatEx* src, const WaveFormatEx* dst,
	DWORD fdwOpen, unsigned __int64* key)
{
	unsigned __int64 h = 14695981039346656037ULL;
	h = FnvMix(h, &driver, sizeof(driver));
	h = FnvMix(h, &fdwOpen, sizeof(fdwOpen));
	if (!HashFormat(&h, src) || !HashFormat(&h, dst)) {
		return false;
	}
	*key = (h == 0) ? 1 : h;        // 0 另有含义（见下）
	return true;
}

// ---- 流池 -------------------------------------------------------------------
// 一张表兼两用：inUse=true 是客户端正在用的，inUse=false 是关过、留着复用的。
// key==0 表示这条流不参与复用，客户端关它的时候照常关掉。
// 表满了就不再登记，行为完全退回打补丁之前。

struct StreamSlot
{
	HACMSTREAM       stream;
	unsigned __int64 key;
	bool             inUse;
};

const int  kMaxStreams = 64;
StreamSlot g_streams[kMaxStreams] = { { nullptr, 0, false } };

HACMSTREAM PoolTake(unsigned __int64 key)
{
	HACMSTREAM got = nullptr;
	EnterCriticalSection(&g_cs);
	for (int i = 0; i < kMaxStreams; ++i) {
		if (g_streams[i].stream != nullptr && !g_streams[i].inUse && g_streams[i].key == key) {
			g_streams[i].inUse = true;
			got = g_streams[i].stream;
			break;
		}
	}
	LeaveCriticalSection(&g_cs);
	return got;
}

void PoolRegister(HACMSTREAM stream, unsigned __int64 key)
{
	EnterCriticalSection(&g_cs);
	for (int i = 0; i < kMaxStreams; ++i) {
		if (g_streams[i].stream == nullptr) {
			g_streams[i].stream = stream;
			g_streams[i].key = key;
			g_streams[i].inUse = true;
			break;
		}
	}
	LeaveCriticalSection(&g_cs);
}

// 客户端要关这条流。返回 true = 我们留下了，别真关。
bool PoolRelease(HACMSTREAM stream)
{
	bool keep = false;
	EnterCriticalSection(&g_cs);
	for (int i = 0; i < kMaxStreams; ++i) {
		if (g_streams[i].stream != stream) {
			continue;
		}
		if (g_streams[i].key != 0) {
			g_streams[i].inUse = false;     // 留池，解码器状态等下次领用时再复位
			keep = true;
		}
		else {
			g_streams[i].stream = nullptr;  // 不参与复用的，让它照常关掉
			g_streams[i].key = 0;
			g_streams[i].inUse = false;
		}
		break;
	}
	LeaveCriticalSection(&g_cs);
	return keep;
}

// ---- 钩子 -------------------------------------------------------------------

UINT WINAPI HkAcmStreamOpen(HACMSTREAM* phas, void* driver, WaveFormatEx* srcFmt, WaveFormatEx* dstFmt,
	void* filter, DWORD_PTR callback, DWORD_PTR instance, DWORD fdwOpen)
{
	// 只复用"同步、无回调、无过滤器、非 QUERY"的普通打开，其余一律原样放行
	unsigned __int64 key = 0;
	const bool poolable =
		phas != nullptr && filter == nullptr && callback == 0 && instance == 0 &&
		(fdwOpen & (kAcmOpenQuery | kAcmOpenAsync)) == 0 &&
		MakePoolKey(driver, srcFmt, dstFmt, fdwOpen, &key);

	if (poolable) {
		HACMSTREAM reuse = PoolTake(key);
		if (reuse != nullptr) {
			// 上一次用完可能还留着解码器状态，领用前先复位
			if (pAcmStreamReset != nullptr) {
				pAcmStreamReset(reuse, 0);
			}
			*phas = reuse;
			return kMmsysNoError;
		}
	}

	const UINT result = oAcmStreamOpen(phas, driver, srcFmt, dstFmt, filter, callback, instance, fdwOpen);

	if (result == kMmsysNoError && (fdwOpen & kAcmOpenQuery) == 0 &&
		phas != nullptr && *phas != nullptr) {
		PoolRegister(*phas, poolable ? key : 0);
	}
	return result;
}

UINT WINAPI HkAcmStreamClose(HACMSTREAM stream, DWORD flags)
{
	if (PoolRelease(stream)) {
		return kMmsysNoError;       // 句柄留在池子里，下次 open 直接递回去
	}
	return oAcmStreamClose(stream, flags);
}

}   // namespace

void SoundStreamFix_Install(bool enable)
{
	if (!enable) {
		return;
	}

	// 音频子系统是懒加载的，DllMain 这会儿 msacm32 多半还没映射进来。
	// 它不依赖别的东西，在 loader lock 里 LoadLibrary 是安全的。
	HMODULE msacm = GetModuleHandleA("msacm32.dll");
	if (msacm == nullptr) {
		msacm = LoadLibraryA("msacm32.dll");
	}
	if (msacm == nullptr) {
		return;
	}

	oAcmStreamOpen = (acmStreamOpen_t)GetProcAddress(msacm, "acmStreamOpen");
	oAcmStreamClose = (acmStreamClose_t)GetProcAddress(msacm, "acmStreamClose");
	pAcmStreamReset = (acmStreamReset_t)GetProcAddress(msacm, "acmStreamReset");
	if (oAcmStreamOpen == nullptr || oAcmStreamClose == nullptr) {
		return;
	}

	// 锁必须在挂钩之前就绪：钩子一装上就可能被调用
	InitializeCriticalSection(&g_cs);

	// open 和 close 必须成对挂上，否则会出现"留在池子里却没人认领"的流
	if (!Memory::SetHook(true, (void**)&oAcmStreamOpen, (void*)HkAcmStreamOpen)) {
		oAcmStreamOpen = nullptr;
		DeleteCriticalSection(&g_cs);
		return;
	}
	if (!Memory::SetHook(true, (void**)&oAcmStreamClose, (void*)HkAcmStreamClose)) {
		Memory::SetHook(false, (void**)&oAcmStreamOpen, (void*)HkAcmStreamOpen);
		oAcmStreamOpen = nullptr;
		oAcmStreamClose = nullptr;
		DeleteCriticalSection(&g_cs);
		return;
	}
}
