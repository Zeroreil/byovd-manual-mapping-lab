# labctl

`LabDrv.sys` 的最小用户态客户端。

## 构建

```bat
cl /nologo /O2 /utf-8 /I..\benign-driver /Fe:labctl.exe labctl.c
```

## 运行

```bat
labctl.exe ping
labctl.exe version
labctl.exe
```

期望：

```text
open \\.\LabDrv OK
PING: 0x4C414231 (LAB1)
VERSION: 1.0 magic=0x4C414231
```

## 说明

- 只调用 `LAB_IOCTL_PING` / `LAB_IOCTL_VERSION`。
- 不包含任意内存读写、进程操作、MSR/端口访问。
- 需要 Guest 里已加载 `LabDrv.sys`，且当前用户能打开设备（测试时用管理员）。
