# 05 · IOCTL 通信

驱动与用户态之间最小、最可控的接口是 `DeviceIoControl`。本仓库的 benign 驱动只实现两个 IOCTL：

| IOCTL | 功能 | 输入 | 输出 |
|---|---|---|---|
| `LAB_IOCTL_PING` | 返回魔数 | 无 | `ULONG` |
| `LAB_IOCTL_VERSION` | 返回版本 | 无 | `LAB_VERSION` |

完整定义见 [`samples/benign-driver/LabDrv.h`](../samples/benign-driver/LabDrv.h)。

## 1. 设备与符号链接

```c
#define LAB_DEVICE_NAME  L"\\Device\\LabDrv"
#define LAB_SYMLINK_NAME L"\\DosDevices\\LabDrv"
#define LAB_USER_PATH    L"\\\\.\\LabDrv"
```

- `\Device\LabDrv` 是内核对象路径；
- `\DosDevices\LabDrv` 是用户态可见的符号链接；
- 用户态用 `CreateFileW(L"\\\\.\\LabDrv", ...)` 打开。

## 2. IOCTL 定义

```c
#define LAB_IOCTL_PING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define LAB_IOCTL_VERSION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
```

- `DeviceType = FILE_DEVICE_UNKNOWN`（0x22）；
- `Function = 0x800/0x801`；
- `Method = METHOD_BUFFERED`：I/O manager 负责把输入/输出复制到 `SystemBuffer`；
- `Access = FILE_ANY_ACCESS`：任何能打开设备的调用方都可发；**生产驱动应使用 `FILE_READ_DATA` / `FILE_WRITE_DATA` 并做调用方校验**。

## 3. 用户态客户端

```c
HANDLE h = CreateFileW(L"\\\\.\\LabDrv",
                       GENERIC_READ | GENERIC_WRITE,
                       0, NULL, OPEN_EXISTING, 0, NULL);
if (h == INVALID_HANDLE_VALUE) {
    printf("open failed: %lu\n", GetLastError());
    return 1;
}

ULONG magic = 0;
DWORD bytes = 0;
BOOL ok = DeviceIoControl(h, LAB_IOCTL_PING,
                          NULL, 0,
                          &magic, sizeof(magic),
                          &bytes, NULL);
```

完整客户端见 [`samples/client/labctl.c`](../samples/client/labctl.c)。

## 4. 驱动侧处理

```c
switch (code) {
case LAB_IOCTL_PING:
    if (outlen < sizeof(ULONG)) return STATUS_BUFFER_TOO_SMALL;
    *(PULONG)Irp->AssociatedIrp.SystemBuffer = LAB_MAGIC;
    Irp->IoStatus.Information = sizeof(ULONG);
    status = STATUS_SUCCESS;
    break;

case LAB_IOCTL_VERSION:
    if (outlen < sizeof(LAB_VERSION)) return STATUS_BUFFER_TOO_SMALL;
    ...
    break;

default:
    status = STATUS_INVALID_DEVICE_REQUEST;
}
```

要点：

1. **先校验 `OutputBufferLength` / `InputBufferLength`**；
2. `METHOD_BUFFERED` 下输入和输出共用 `SystemBuffer`，不要假设它一定非空；
3. 不要从用户指针直接解引用；用 `ProbeForRead/Write`（`METHOD_NEITHER` 时）或让 I/O manager 复制；
4. 对 IOCTL code 做白名单，不要用 `default` 放行；
5. 返回 `STATUS_INVALID_DEVICE_REQUEST` 而不是静默成功。

## 5. 调用方校验

危险驱动常见错误是“任何能打开设备的进程都能调用任意读写 IOCTL”。防御性设计：

- 设备 ACL：只允许管理员/受保护进程打开；
- `IRP_MJ_CREATE` 中检查 `SeQueryInformationToken(TokenElevation)`；
- 检查调用方镜像路径/签名（注意可伪造，不能作为唯一门槛）；
- 对高权限 IOCTL 要求额外会话令牌/挑战响应；
- 记录调用方 PID、令牌、IOCTL code、长度。

## 6. 检测与模糊测试

### 检测

- 设备打开事件：`\\.\...` 路径、调用方 PID/父进程；
- IOCTL code/长度/频率；
- 设备 ACL 是否允许普通用户；
- 同一进程短时间内大量不同 IOCTL（扫描/探测）；
- 设备打开后紧跟内核内存/进程操作（关联）。

### 模糊测试（授权环境）

- 对每个 IOCTL 做边界长度（0、1、`sizeof-1`、`sizeof`、超大）；
- 检查 `SystemBuffer` 为空、输出长度不足；
- 检查并发调用/句柄复用；
- 用 Driver Verifier（`verifier /standard /driver LabDrv.sys`）在 VM 中跑；
- 任何蓝屏都记录 BugCheck、参数、IRP/IOCTL。

## 7. 最小权限原则

| 设计 | 风险 |
|---|---|
| `FILE_ANY_ACCESS` + 任意用户可打开 | 任意进程可发 IOCTL |
| 直接信任 `SystemBuffer` 长度 | 越界读写 |
| `METHOD_NEITHER` + 直接解引用用户指针 | 任意地址读写 |
| 不校验调用方 | 低权限进程提权 |
| 危险 IOCTL 无审计 | 滥用不可见 |

## 8. 公开边界

本仓库的客户端只做 `PING`/`VERSION`，不包含：

- 任意进程/内核内存读写；
- 物理内存映射；
- MSR/端口访问；
- 进程隐藏/句柄操作。

这些原语的危险实现不进入公开仓库；检测与安全设计是本仓库的重点。
