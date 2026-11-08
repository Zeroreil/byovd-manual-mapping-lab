# 11 · AmdTools64 Case Study (Metadata Only)

> 这一页是**防御案例**，不是分发页。公开仓库只保留文件名、版本、签名者和 SHA256，用于检测/拦截；不包含驱动二进制、INF、服务安装包、IOCTL 合同或加载器。

## 1. 样本元数据

| 项 | 值 |
|---|---|
| 文件名 | `AmdTools64.sys` |
| 大小 | 77,240 bytes |
| 版本 | `1.7.16.219` |
| 签名者 | `CN=Advanced Micro Devices Inc., O=Advanced Micro Devices Inc., L=Santa Clara, S=California, C=US` |
| 签名状态 | Valid（Sectigo RSA Code Signing CA） |
| SHA256 | `891007BC9F3B55AE168FA60CCBB831CCDB5DCE4DA4A591D000806D5A2C6115C3` |

> 这些值来自本地授权实验样本，仅用于检测规则/哈希拦截。不要从本仓库下载二进制；本仓库没有放。

## 2. 为什么它是 BYOVD 相关样本

`AmdTools64.sys` 是 AMD 签名的工具驱动，历史上被公开披露为可被滥用的一类“签名但接口过宽”的驱动：它暴露了物理内存访问类接口，攻击者可把它当作内核原语，再配合未签名 payload。

**本页不提供**：

- 具体 IOCTL code、输入结构、物理地址转换步骤；
- 设备名/服务名的利用脚本；
- 加载器、映射器、payload；
- 任何“从用户态到 ring0”的完整利用链。

这些内容即使在本项目的授权实验区存在，也不进入公开仓库。

## 3. 检测

### 3.1 文件/哈希

```text
SHA256 = 891007BC9F3B55AE168FA60CCBB831CCDB5DCE4DA4A591D000806D5A2C6115C3
```

- 在 EDR/AV 中按哈希拦截；
- 在 Microsoft Vulnerable Driver Blocklist / WDAC 中按签名者+文件名+版本拦截；
- 监控 `C:\Windows\System32\drivers\AmdTools64.sys` 的出现。

### 3.2 服务/设备

- 服务名 `AmdTools64`（或变体）的创建/启动；
- `ROOT\AMDTOOLS64\...` 设备实例；
- 驱动加载后短时间内的设备打开/IOCTL。

### 3.3 行为

- 非 AMD 软件/非管理员进程打开该设备；
- 设备打开后出现物理内存/内核地址操作；
- 驱动加载后出现未签名可执行内存/模块外代码。

### 3.4 事件

| 来源 | 事件 |
|---|---|
| System | 7045（服务创建） |
| Security | 4697（服务安装） |
| Sysmon | 6（驱动加载）、11（文件创建） |
| Kernel-PnP | 设备实例创建 |
| EDR | 设备打开/IOCTL 遥测 |

## 4. 缓解

1. **Blocklist**：Microsoft Vulnerable Driver Blocklist / WDAC / HVCI。
2. **移除/更新**：如果机器不需要 AMD 工具驱动，卸载对应软件并删除驱动。
3. **最小权限**：限制 `SeLoadDriverPrivilege`；普通用户不能创建内核服务。
4. **监控**：对 `AmdTools64.sys` 哈希、签名者、服务、设备打开做告警。
5. **隔离**：高风险机器使用 HVCI + 驱动白名单；实验在 VM/快照中进行。

## 5. 公开边界

本仓库不包含：

- `AmdTools64.sys` 二进制；
- 其 INF / 服务安装脚本；
- 其 IOCTL 合同或利用代码；
- 任何 BYOVD 加载器/手动映射器。

只保留**元数据、哈希、检测与缓解**。理由见 [09-publication-boundary.md](09-publication-boundary.md)。

## 6. 参考

- LOLDrivers（公开的易受攻击驱动目录）：<https://www.loldrivers.io/>
- Microsoft recommended driver block rules：<https://learn.microsoft.com/windows/security/application-security/application-control/app-control-for-business/design/microsoft-recommended-driver-block-rules>
- HVCI：<https://learn.microsoft.com/windows/security/hardware-security/enable-virtualization-based-protection-of-code-integrity>
