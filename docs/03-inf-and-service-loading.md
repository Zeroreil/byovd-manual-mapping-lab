# 03 · INF 与服务加载

本仓库的 benign 驱动 `LabDrv.sys` 只创建 `\Device\LabDrv`，提供两个只读 IOCTL：`PING` 和 `VERSION`。它用于演示：

- INF 的 `DefaultInstall.Services` / `AddService` 结构；
- `sc create` / `sc start` 的加载路径；
- 用户态 `CreateFile` + `DeviceIoControl`；
- 驱动加载、服务创建、设备打开产生的遥测。

它**不包含**任意内存读写、进程操作、MSR/端口访问或隐藏逻辑。

## 1. INF 结构

见 [`samples/benign-driver/LabDrv.inf`](../samples/benign-driver/LabDrv.inf)。

关键段：

```ini
[DefaultInstall]
CopyFiles=LabDrv.CopyFiles

[DefaultInstall.Services]
AddService=LabDrv,,LabDrv.Service

[LabDrv.Service]
DisplayName=%ServiceDesc%
ServiceType=1          ; SERVICE_KERNEL_DRIVER
StartType=3            ; SERVICE_DEMAND_START
ErrorControl=1
ServiceBinary=%12%\LabDrv.sys

[DestinationDirs]
LabDrv.CopyFiles=12    ; %windir%\System32\drivers
```

- `ServiceType=1` 表示内核驱动。
- `StartType=3` 表示按需启动（不是 boot-start）。
- `ServiceBinary=%12%\LabDrv.sys` 指向 Drivers 目录。
- `CatalogFile` 需要签名；测试环境可用测试证书生成 `.cat`，或直接 `sc create` 加载已签名的 `.sys`。

## 2. 两种加载方式

### 2.1 INF 安装（接近真实软件分发）

```bat
pnputil /add-driver LabDrv.inf /install
```

或右键 INF → 安装。安装后：

```bat
sc query LabDrv
sc start LabDrv
```

### 2.2 直接服务加载（研究/调试常用）

```bat
sc create LabDrv type= kernel start= demand binPath= C:\Lab\LabDrv.sys
sc start LabDrv
sc query LabDrv
```

卸载：

```bat
sc stop LabDrv
sc delete LabDrv
```

> `sc create` 需要管理员权限；驱动必须已签名（测试签名需开启 `testsigning` 且证书受信任）。

## 3. 驱动侧最小实现

`LabDrv.c` 的 `DriverEntry`：

1. `IoCreateDevice` 创建 `\Device\LabDrv`；
2. `IoCreateSymbolicLink` 创建 `\DosDevices\LabDrv`；
3. 注册 `IRP_MJ_CREATE/CLOSE/DEVICE_CONTROL`；
4. `DriverUnload` 删除符号链接与设备。

设备控制：

```c
switch (code) {
case LAB_IOCTL_PING:
    if (outlen < sizeof(ULONG)) return STATUS_BUFFER_TOO_SMALL;
    *(PULONG)buf = LAB_MAGIC;
    info = sizeof(ULONG);
    break;
case LAB_IOCTL_VERSION:
    if (outlen < sizeof(LAB_VERSION)) return STATUS_BUFFER_TOO_SMALL;
    ...
}
```

完整的 `METHOD_BUFFERED` 处理见源码。

## 4. 加载阶段的可观测面

| 事件 | 来源 | 说明 |
|---|---|---|
| 服务创建 | Security 4697 / System 7045 | `ServiceType=1`、`ImagePath`、启动类型 |
| 驱动加载 | Sysmon EID 6 | `ImageLoaded`、`Signature`、`SignatureStatus` |
| 驱动文件落盘 | Sysmon EID 11 / 文件审计 | 路径、哈希、签名者 |
| PnP 设备 | Kernel-PnP / `Get-PnpDevice` | `ROOT\...`、FriendlyName |
| 设备打开 | 驱动/ETW/句柄审计 | `\\.\LabDrv`、调用方 PID/令牌 |
| IOCTL | 驱动日志/ETW/内核回调 | code、长度、频率 |

### 检测建议

- 对 `ServiceType=1` 且 `ImagePath` 位于用户可写目录的服务创建告警；
- 对未签名/测试签名驱动的加载告警；
- 对“服务创建后 60 秒内设备被打开”的序列关联；
- 对 `\\.\` 设备名的异常访问做基线；
- 监控 `SeLoadDriverPrivilege` 的使用。

## 5. 常见失败模式

| 现象 | 原因 | 处理 |
|---|---|---|
| `sc start` 577 | 签名不受信任 / testsigning 未开 | 导入测试证书、开启 testsigning、关闭 Secure Boot |
| `sc start` 1275 | 驱动被签名策略拦截 | 检查 HVCI/WDAC/Blocklist |
| `CreateFile` 失败 5 | 设备 ACL 拒绝 | 以管理员运行，或检查驱动 DACL |
| `CreateFile` 失败 2 | 符号链接不存在 | 检查 `IoCreateSymbolicLink` / 卸载路径 |
| INF 安装失败 | `.cat` 缺失/签名无效 | 生成并签名 catalog，或改用 `sc create` 测试 |

## 6. 安全边界

- 这个 INF 只安装 benign 驱动，不包含危险 IOCTL。
- 不要把真实易受攻击驱动的 INF、二进制或证书提交到公开仓库。
- 详细发布边界见 [09-publication-boundary.md](09-publication-boundary.md)。
