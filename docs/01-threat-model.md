# 01 · Threat Model

## 1. 定义

**BYOVD**（Bring Your Own Vulnerable Driver）是指攻击者把一个**合法签名但存在任意读写/调用漏洞**的内核驱动带到目标机器上，通过正常或半正常的加载路径获得内核代码执行或任意物理/虚拟内存访问，再配合一个未签名 payload 完成提权、EDR/反作弊绕过或数据窃取。

关键点不是“驱动本身恶意”，而是：

1. 驱动有合法签名，能通过默认的驱动签名策略；
2. 驱动暴露了一个过宽的 IOCTL（任意物理内存读写、任意 MSR、任意端口、任意内核 VA 读写、进程/句柄操作等）；
3. 攻击者把“加载”和“利用”拆成两步：先加载合法驱动，再通过 IOCTL 完成恶意动作。

## 2. 资产与信任边界

| 资产 | 位置 | 被破坏后的影响 |
|---|---|---|
| 内核代码执行 | Ring 0 | 关闭/绕过 EDR、反作弊、DSE、ETW |
| 进程内存 | 目标进程 | 读取凭据、注入、篡改游戏/业务数据 |
| 驱动加载策略 | CI / Secure Boot / HVCI | 未签名 payload 进入内核 |
| 设备对象 / 符号链接 | Object Manager | 任意用户态进程可打开设备 |
| 内核模块/对象链 | PsLoadedModuleList、`\Driver`、`\Device` | 隐藏驻留、规避枚举 |
| 遥测 | ETW / Sysmon / SCM / PnP | 攻击链不可见 |

## 3. 对手能力

| 级别 | 能力 | 典型目标 |
|---|---|---|
| T1 用户态 | 进程/模块/窗口/句柄枚举、API hook | 普通游戏外挂、用户态反作弊 |
| T2 静态扫描 | 文件、签名、YARA、字符串、PDB | AV/EDR 扫描 |
| T3 内核级 | 驱动枚举、对象/池扫描、回调、句柄表、交叉校验 | ACE/EDR/反作弊 |
| T4 人工 | 动态调试、内存 dump、差分 | 深度取证 |

本仓库的检测设计以 **T3** 为主要对手：单点隐藏不够，必须有交叉校验。

## 4. 攻击链（抽象）

```text
初始访问
  → 驱动获取（合法签名、存在过宽 IOCTL）
  → 暂存/落盘（或内存映射）
  → 加载（服务/INF/手动映射/白利用）
  → IOCTL 利用（任意读写、调用、进程操作）
  → payload 执行（未签名驱动/用户态模块）
  → 隐藏/清理（服务、对象、模块链、文件）
  → 目标动作（提权、读内存、绕过检测）
```

每一阶段都有可观测面；见 [02-byovd-lifecycle.md](02-byovd-lifecycle.md) 和 [07-detection-matrix.md](07-detection-matrix.md)。

## 5. 防御目标

1. **减少可用驱动面**：驱动白名单、攻击面减少（ASR）、Hypervisor-Protected Code Integrity（HVCI）、Microsoft Vulnerable Driver Blocklist。
2. **让加载可见**：SCM、PnP、Sysmon EID 6、ETW `Microsoft-Windows-Kernel-Boot`/`DriverFrameworks`、WDAC。
3. **让 IOCTL 滥用可见**：设备打开、IOCTL 频率/参数、调用方身份、句柄来源。
4. **让隐藏失效**：模块链表 vs 内存扫描、对象目录 vs 句柄表、服务 vs 实际驱动、池扫描。
5. **快速响应**：隔离、回滚、证书/驱动撤销、取证。

## 6. 非目标

- 不提供真实易受攻击驱动的名称、哈希、下载地址或二进制；
- 不提供完整手动映射 loader；
- 不提供 DKOM/对象摘名等可直接 weaponize 的隐藏实现；
- 不针对具体游戏、反作弊或生产环境。

这些边界是刻意的，见 [09-publication-boundary.md](09-publication-boundary.md)。
