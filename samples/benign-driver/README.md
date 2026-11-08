# Benign Lab Driver

一个只做 `PING` / `VERSION` 的 WDM 驱动，用来演示：

- 驱动入口、设备对象、符号链接；
- `METHOD_BUFFERED` IOCTL；
- INF 服务安装；
- 驱动加载/设备打开/IOCTL 的遥测。

它**没有**内存读写、进程操作、MSR/端口访问或隐藏逻辑。

## 文件

| 文件 | 说明 |
|---|---|
| `LabDrv.c` | 驱动源码 |
| `LabDrv.h` | IOCTL/设备名/版本结构 |
| `LabDrv.inf` | demand-start 内核服务安装模板 |
| `../client/labctl.c` | 用户态测试客户端 |

## 构建（WDK + MSVC x64）

在 **x64 Native Tools Command Prompt** 中：

```bat
cl /nologo /c /W3 /O2 /GS- /Zl /D_KERNEL_MODE=1 /D_AMD64_ /D_AMD64 ^
   /D_WIN32_WINNT=0x0A00 /DNTDDI_VERSION=0x0A000000 ^
   /I"%WDKIncludePath%\km" /I"%WDKIncludePath%\km\crt" ^
   /I"%WDKIncludePath%\shared" ^
   /Fo:LabDrv.obj LabDrv.c

link /nologo /DRIVER /SUBSYSTEM:NATIVE /MACHINE:X64 /ENTRY:DriverEntry ^
     /NODEFAULTLIB /OUT:LabDrv.sys ^
     LabDrv.obj ntoskrnl.lib hal.lib BufferOverflowK.lib
```

> 具体 include/lib 路径按你的 WDK 版本调整；也可用 Visual Studio 的 "Empty WDM Driver" 模板。

## 测试签名（仅 VM）

```bat
makecert -r -pe -ss PrivateCertStore -n "CN=LabDrv Test" LabDrvTest.cer
signtool sign /fd SHA256 /f LabDrvTest.pfx /p <password> /a LabDrv.sys
```

把证书导入 Guest 的 `LocalMachine\Root` 和 `TrustedPublisher`，开启 `testsigning`，关闭 Secure Boot，重启。

> 测试证书只用于隔离 VM。不要提交 `.pfx`/私钥。

## 安装/加载

### 方式 A：INF

```bat
pnputil /add-driver LabDrv.inf /install
sc start LabDrv
```

> 生产 INF 安装需要签名 catalog；测试环境如果没有 `.cat`，用方式 B 更方便。

### 方式 B：sc create

```bat
copy LabDrv.sys C:\Lab\LabDrv.sys
sc create LabDrv type= kernel start= demand binPath= C:\Lab\LabDrv.sys
sc start LabDrv
sc query LabDrv
```

## 运行客户端

```bat
cl /nologo /O2 /utf-8 /Fe:labctl.exe ..\client\labctl.c
labctl.exe ping
labctl.exe version
```

期望：

```text
open \\.\LabDrv OK
PING: 0x4C414231 (LAB1)
VERSION: 1.0 magic=0x4C414231
```

## 卸载

```bat
sc stop LabDrv
sc delete LabDrv
del C:\Lab\LabDrv.sys
```

## 观察点

在 Guest 里开启 Sysmon/审核后，应能看到：

- 服务创建（7045 / 4697）；
- 驱动加载（Sysmon 6）；
- 驱动文件落盘（Sysmon 11）；
- `\\.\LabDrv` 打开与 IOCTL 调用（驱动/ETW/EDR 遥测）。

这些遥测是 [docs/07-detection-matrix.md](../../docs/07-detection-matrix.md) 的输入。
