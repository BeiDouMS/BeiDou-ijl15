#pragma once

// 音效解码流复用 —— 修 Wine/macOS 上"每次跳跃卡 100ms 左右，背景动画一起冻住"
//
// 客户端的音频封装 SOUND_DX8.DLL 每播一次音效都要新开一条 MP3 解码流（acmStreamOpen）、
// 播完就关。Windows 原生开流几毫秒就完了；Wine 用的是内建的 l3codeca.acm，
// 开流时要初始化 mpg123 的解码表，再叠一层 Rosetta 翻译，实测**一次 107ms**，
// 而且是在主线程上同步开，于是整个主循环冻住。
//
// 定位过程见 SoundStreamFix-notes.md。决定性的一行帧日志：
//
//     FRAME #1534 133.5ms | ... acm=107.5/1 | worst=acmStreamOpen 107.5ms @SOUND_DX8.DLL+0x00469B
//
// 那一帧只有 1 次 ACM 调用，就是开流，它自己吃掉了 107.5ms —— 慢的不是解码，是开流。
// 跳跃最容易察觉，因为它高频、且音效只给本地角色播（所以"别人跳不卡"；掉落、被击退、
// 从绳子跳下来这些动作根本没音效，也都不卡）。
//
// 这里的做法：客户端关流时不真关，把句柄留在池子里；下次同格式打开直接把它递回去，
// 领用前调 acmStreamReset 复位解码器状态。只对同步、无回调、无过滤器、非 QUERY 的
// 普通打开生效，其余一律原样放行。Windows 上开流本来就快，开着也没有副作用。

// enable=false 则完全不挂钩。对应 config.ini 的 [compat] ReuseSoundStreams
void SoundStreamFix_Install(bool enable);
