#pragma once

// 非中文系统上的中文显示修复（英文/其他语言的 Windows，以及 macOS 上的 Wine）
//
// 客户端是 ANSI 程序，中文以 GBK(936) 字节存放（Client::Chinese 里那些
// Memory::WriteString 写进去的就是 GBK）。它怎么变成屏幕上的字，有两条路，
// 而且两条都跟着"系统区域设置(非Unicode程序)"走：
//
//   1) 客户端自己调 MultiByteToWideChar(CP_ACP, ...) 转宽字符，
//      折行/光标移动则依赖 IsDBCSLeadByte 判断双字节前导字节；
//   2) 客户端用 TextOutA 画字，而 GDI 的 ...A 文字函数是按
//      "当前字体的 charset" 决定拿哪个代码页解码字节的。
//      客户端建字体时给的是 DEFAULT_CHARSET，含义正是"跟随系统区域设置"：
//      中文系统上解析成 GB2312(134) -> CP936，非中文系统上解析成 ANSI(0) -> CP1252。
//
// 所以在非中文系统上，GBK 字节被当拉丁文解码，满屏乱码。
// 实测 GB2312_CHARSET 恒定映射到 CP936，与系统区域无关，所以把 DEFAULT_CHARSET
// 钉成 GB2312 就等于在任何区域下复现中文系统的行为。
//
// 注意：光钩 MultiByteToWideChar 是不够的（第 2 条路的转换发生在 gdi32 内部，
// 用的是字体 charset，不经过这个导出函数）；反过来光改字体 charset 也不够
// （客户端自己的字符串逻辑还是错的）。两条都要。
//
// 系统 ACP 本来就是 936 时一个钩子都不装，中文 Windows 用户完全不受影响。

// forceFontCharset：是否把建字体时的 DEFAULT_CHARSET 改写成 GB2312。
// 对应 config.ini 的 [compat] ForceFontCharset。代码页模拟部分（上面第 1 条）
// 是必需的、无副作用的，所以不设开关，只在系统 ACP 不是 936 时自动生效。
void LocaleFix_Install(bool forceFontCharset);
