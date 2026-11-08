# 02 · BYOVD Lifecycle

BYOVD 不是单点技术，而是一条生命周期。每个阶段都可以独立检测；把多个阶段的低置信信号关联起来，才能降低误报。

## 阶段 0 · 驱动获取

攻击者寻找：

- 有合法签名、但 IOCTL 允许任意物理/虚拟内存读写、任意 MSR/端口访问、任意进程/句柄操作的驱动；
- 能通过目标系统默认签名策略的驱动；
- 容易在目标机器上安装为服务的驱动（有 INF 或可直接 `sc create`）。

**遥测点**

- 文件下载/解压/落盘：文件路径、哈希、签名者、Zone.Identifier、父进程；
- 驱动文件首次出现：`C:\Windows\System32\drivers\`、临时目录、用户目录；
- 签名证书与已知易受攻击驱动名单的匹配。

**缓解**

- Microsoft Vulnerable Driver Blocklist / HVCI / WDAC；
- 驱动白名单；
- 阻止从用户可写目录加载驱动；
- 监控新驱动文件的签名者与哈希。

## 阶段 1 · 暂存

常见落盘位置：

- `%TEMP%`、`%APPDATA%`、用户下载目录；
- `C:\Windows\System32\drivers\`（服务安装后）；
- 通过 INF 的 `CopyFiles` 复制到 `%12%`（Drivers 目录）。

**遥测点**

- 文件创建/重命名/删除；
- 服务创建（Event ID 7045）与 `ImagePath`；
- INF 安装（`setupapi.dev.log`）；
- 驱动文件与已知样本哈希匹配。

## 阶段 2 · 加载

三条常见路径：

1. **服务加载**：`sc create` / `CreateService` + `sc start`，`ServiceType=1`（KERNEL_DRIVER）。
2. **INF 安装**：`DefaultInstall.Services` / `AddService`，PnP 或 legacy 服务。
3. **手动映射**：不创建服务，把 PE 映射到内核池并调用入口点；不进入 `PsLoadedModuleList`（见 [04-manual-mapping.md](04-manual-mapping.md)）。

**遥测点**

| 路径 | 遥测 |
|---|---|
| 服务 | Security 4697、System 7045、SCM 操作、`Services\` 注册表 |
| INF | `setupapi.dev.log`、`%windir%\inf\setupapi.app.log` |
| PnP | Kernel-PnP 事件、设备实例、`ROOT\...` |
| 手动映射 | 无服务事件；靠内存/对象/模块链异常发现 |

**缓解**

- 只允许签名驱动；开启 HVCI；
- 监控服务创建与驱动加载；
- 禁止普通用户创建内核服务；
- 对 `SeLoadDriverPrivilege` 做最小化授权与审计。

## 阶段 3 · IOCTL 利用

攻击者通过 `CreateFile("\\.\DeviceName")` + `DeviceIoControl` 调用驱动的危险 IOCTL。

常见危险原语：

- 任意物理内存映射/读写；
- 任意内核虚拟地址读写；
- 任意 MSR 读写（如 `MSR_LSTAR`、`MSR_GS_BASE`）；
- 任意端口 I/O；
- 进程/线程/句柄操作；
- 关闭 DSE / 修改回调 / 摘链。

**遥测点**

- 设备打开（`\\.\...`）与调用进程；
- IOCTL code、输入/输出长度、频率；
- 调用方令牌/完整性级别/镜像路径；
- 设备 ACL 是否允许普通用户。

**缓解**

- 驱动侧：最小权限、调用方校验、参数校验、METHOD_BUFFERED/NEITHER 正确使用；
- 系统侧：限制设备 ACL、监控异常 IOCTL、攻击面减少；
- 检测侧：把“设备打开 + IOCTL + 后续内核内存变化”关联。

## 阶段 4 · Payload 执行

BYOVD 只提供原语；真正的 payload 可能是：

- 未签名驱动（手动映射后调用入口）；
- 用户态 DLL/EXE（借驱动读写内存）；
- 对内核代码/回调的 patch。

**遥测点**

- 内核池中的 PE 镜像（MZ/PE 头、节表）；
- 不在 `PsLoadedModuleList` 的可执行内存；
- 驱动对象/设备对象没有对应服务/文件；
- 代码页哈希与磁盘文件不一致（内核代码完整性）。

**缓解**

- HVCI / 内核代码完整性；
- 池扫描与可执行内存审计；
- 驱动对象/模块链表/服务三方交叉校验。

## 阶段 5 · 隐藏与清理

常见隐藏面：

- 删除服务/注册表；
- 删除驱动文件；
- 删除设备符号链接；
- 从 `PsLoadedModuleList` 摘链；
- 从 `\Driver` / `\Device` 对象目录摘名；
- 清除池中 PE 头。

**检测原则**

单个隐藏动作通常会留下**另一个视角的矛盾**：

- 服务不存在，但驱动对象/设备对象仍存在；
- 模块链表没有，但池里有 PE/可执行页；
- 符号链接没有，但已有句柄仍可 IOCTL；
- 文件删除，但内核里仍有代码页。

详见 [06-hiding-and-detection.md](06-hiding-and-detection.md)。

## 阶段 6 · 清理

攻击者可能删除服务、文件、注册表、证书；防守方应保存：

- 驱动文件哈希与签名者；
- 服务创建/删除时间线；
- 设备打开/IOCTL 记录；
- 内存快照/崩溃转储；
- 相关进程与父进程链。

## 小结

| 阶段 | 最强单点检测 | 关联检测 |
|---|---|---|
| 获取 | 已知易受攻击驱动名单 | 文件 + 签名 + 下载来源 |
| 暂存 | 驱动文件落盘 | 服务/INF 安装 |
| 加载 | 7045 / Sysmon 6 | 服务 + 驱动对象 + 设备 |
| 利用 | 设备打开 + IOCTL | 调用方 + 后续内存变化 |
| 执行 | 池中 PE / 可执行内存 | 模块链 vs 内存扫描 |
| 隐藏 | 对象/模块/服务交叉校验 | 多视角矛盾 |
