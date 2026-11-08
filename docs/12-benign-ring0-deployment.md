# 12 · Benign Ring-0 Deployment

> 这是一个**测试签名的 benign 驱动**通过正常服务路径进入 ring0 的完整部署流程。它不是 BYOVD：没有易受攻击驱动、没有手动映射、没有未签名 payload。

目标：把 `LabDrv.sys` 加载为内核服务，打开设备，完成 `PING`/`VERSION`，然后干净卸载。

## 1. 产物

```text
LabDrv.sys        # 测试签名后的 benign 驱动
LabDrv.inf        # 可选：INF 服务安装模板
labctl.exe        # 用户态 IOCTL 客户端
deploy.ps1        # 本目录的部署/清理脚本
```

## 2. 前置

- Windows 10/11 x64 **隔离 VM**；
- Secure Boot 关闭；
- `testsigning on`；
- 测试证书已导入 `LocalMachine\Root` 和 `LocalMachine\TrustedPublisher`；
- 管理员 PowerShell；
- 已有 `LabDrv.sys`（测试签名）和 `labctl.exe`。

## 3. 构建/签名

见 [samples/benign-driver/README.md](../samples/benign-driver/README.md)。核心：

```bat
sc create LabDrv type= kernel start= demand binPath= C:\Lab\LabDrv.sys
```

签名（测试证书）：

```bat
signtool sign /fd SHA256 /f LabDrvTest.pfx /p <password> /a LabDrv.sys
```

## 4. 部署

```powershell
# 以管理员打开 PowerShell
cd C:\Lab
powershell -ExecutionPolicy Bypass -File .\deploy.ps1
```

脚本会：

1. 校验 `LabDrv.sys` / `labctl.exe` 存在；
2. `sc stop` / `sc delete` 旧服务（忽略不存在）；
3. `sc create LabDrv type= kernel start= demand binPath= ...`；
4. `sc start LabDrv`；
5. `sc query LabDrv`；
6. 运行 `labctl.exe all`；
7. 打印结果。

期望：

```text
SERVICE_NAME: LabDrv
        TYPE               : 1  KERNEL_DRIVER
        STATE              : 4  RUNNING

open \\.\LabDrv OK
PING: 0x4C414231 (LAB1)
VERSION: 1.0 magic=0x4C414231
```

## 5. 清理

```powershell
powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -Cleanup
```

或手动：

```bat
sc stop LabDrv
sc delete LabDrv
del C:\Lab\LabDrv.sys
```

## 6. 观察点

| 阶段 | 遥测 |
|---|---|
| 复制驱动 | 文件创建（Sysmon 11） |
| 创建服务 | 7045 / 4697 |
| 启动驱动 | Sysmon 6 / SCM |
| 打开设备 | 句柄/对象审计 |
| IOCTL | 驱动/ETW/EDR |
| 卸载 | 服务删除/文件删除 |

## 7. 边界

- 只加载 **benign** `LabDrv.sys`；
- 不使用 `AmdTools64.sys` 或其他易受攻击驱动；
- 不做手动映射、DKOM、对象摘名、模块隐藏；
- 只在隔离 VM；实验后回滚快照。

如果你要的是“BYOVD 把未签名驱动送进 ring0”的完整 weaponized 包，本仓库不提供；见 [09-publication-boundary.md](09-publication-boundary.md)。
