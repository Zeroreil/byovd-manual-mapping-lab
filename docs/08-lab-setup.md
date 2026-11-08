# 08 · Lab Setup

## 1. 隔离

- 使用独立 VM（VMware/Hyper-V/VirtualBox），不要在日常主机上测试。
- 网络设为 NAT/Host-only；实验期间不要桥接到生产网络。
- 关闭共享文件夹/剪贴板，或只传测试文件。
- 每次实验前打快照，实验后回滚。

## 2. Guest 配置

```powershell
# 管理员 PowerShell
bcdedit /set "{current}" testsigning on
bcdedit /set "{current}" nointegritychecks on   # 仅测试环境
shutdown /r /t 0
```

- 关闭 Secure Boot（VM 固件设置）。
- 关闭 VBS/HVCI（测试签名驱动与 HVCI 冲突）。
- 确认 `testsigning` 水印。
- 不要在实体机/生产机开启 `nointegritychecks`。

## 3. 构建环境

- Visual Studio 2022/2026 + WDK 10.0.26100。
- x64 Native Tools Command Prompt。
- 测试证书：`makecert`/`New-SelfSignedCertificate` + `signtool`。
- 调试：WinDbg + 串口/网络内核调试；VM 快照。

### 构建 benign 驱动

见 [`samples/benign-driver/README.md`](../samples/benign-driver/README.md)。核心命令：

```bat
cl /c /W3 /O2 /GS- /Zl /D_KERNEL_MODE=1 /I"%WDKINC%\km" ... LabDrv.c
link /DRIVER /SUBSYSTEM:NATIVE /ENTRY:DriverEntry /NODEFAULTLIB ...
signtool sign /fd SHA256 /f test.pfx /p <password> /a LabDrv.sys
```

## 4. 遥测

建议开启：

- Sysmon（EID 1/6/11/13）；
- Windows Security 审核：4697（服务安装）、4656/4663（对象访问）；
- System 7045（服务创建）；
- PowerShell 日志（如果使用脚本部署）；
- ETW：`Microsoft-Windows-Kernel-Boot`、`DriverFrameworks`；
- 驱动 Verifier（测试 benign 驱动时）。

## 5. 测试顺序

1. 快照 `clean`。
2. 构建/签名 benign 驱动。
3. 安装 INF 或 `sc create`。
4. `sc start`，确认 `RUNNING`。
5. 运行 `labctl ping/version`。
6. 采集遥测（服务创建、驱动加载、设备打开、IOCTL）。
7. 卸载/删除服务。
8. 回滚快照。

## 6. 清理

```bat
sc stop LabDrv
sc delete LabDrv
del C:\Windows\System32\drivers\LabDrv.sys
```

- 删除测试证书（`certmgr.msc` / `certutil -delstore`）。
- 回滚 VM 快照。
- 不要把测试证书私钥、驱动二进制、日志提交到公开仓库。

## 7. 安全提醒

- 内核驱动 bug 可直接蓝屏/破坏系统；只在 VM。
- 不要在生产机开启测试签名。
- 不要加载来源不明的驱动。
- 不要在公开仓库放置真实易受攻击驱动或完整 loader。
