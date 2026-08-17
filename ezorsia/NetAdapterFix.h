#pragma once

// GetAdaptersInfo 失败兜底
//
// 客户端在选完角色后要枚举网卡取 MAC 地址（CLogin 那条路径，和 SelectCharMacFix 同源）。
// 它的写法是：栈上开一个 0x2800 字节的缓冲区（正好 16 个 IP_ADAPTER_INFO），
// 调 GetAdaptersInfo，然后 **完全不看返回值** 就直接把缓冲区当链表遍历：
//
//     for (p = buf; p != NULL; p = p->Next)
//         if (p->Type == MIB_IF_TYPE_ETHERNET) ...
//
// 缓冲区事先没清零。所以只要 GetAdaptersInfo 失败（Wine/macOS 上就会失败，或者
// 报缓冲区不足），p->Next 读到的就是未初始化的栈垃圾，一解引用就是访问违例。
//
// 更糟的是崩溃之后：客户端自己的未处理异常过滤器在写转储文件时会二次崩溃，
// 重入之后去抢一把自己已经持有的自旋锁，于是无限 Sleep(100) —— 崩溃表现为永久假死。
// （实测地址：崩溃点 0x005FCE38，自锁的加锁循环 0x00797070。）
//
// 这里的做法：调用失败时给客户端造一条最小可用的单节点链表（Next=NULL、
// Type=以太网、6 字节 MAC），让它拿到合法数据正常往下走。
// 真实调用成功时一个字节都不改，所以对 Windows 用户没有任何影响。

// 目标代码页无关；enable=false 则完全不挂钩。对应 config.ini 的 [compat] FixAdapterInfo
void NetAdapterFix_Install(bool enable);
