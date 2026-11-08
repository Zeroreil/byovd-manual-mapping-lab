# 07 · Detection Matrix

按攻击阶段列出可观测面、检测逻辑和数据源。规则模板见 [`../detection/`](../detection/)。

## 1. 阶段矩阵

| 阶段 | 遥测 | 检测逻辑 | 数据源 | 误报注意 |
|---|---|---|---|---|
| 驱动落盘 | 文件创建、哈希、签名 | 驱动文件出现在用户可写目录；签名者/哈希命中名单 | Sysmon 11、EDR 文件遥测 | 软件更新、驱动安装包 |
| 服务创建 | 7045、4697、SCM | `ServiceType=1` + `ImagePath` 异常 + 非管理员创建 | Security/System 日志 | 运维安装驱动 |
| 驱动加载 | Sysmon 6、ETW | 未签名/测试签名/黑名单驱动 | Sysmon、EDR | 开发机测试签名 |
| 设备打开 | 句柄/对象 | `\\.\` 设备被非预期进程打开 | 句柄审计、EDR | 合法硬件工具 |
| IOCTL | 驱动日志/ETW | 高频/异常长度/危险 code | 驱动遥测 | 正常配置工具 |
| 手动映射 | 池/内存 | 可执行池中的 PE；模块外代码 | 内存扫描、HVCI 日志 | JIT/安全产品 |
| 隐藏 | 多视角 | 服务/驱动/模块/设备/文件矛盾 | 组合查询 | 测试驱动/内存驱动 |
| 清理 | 文件/服务删除 | 驱动文件删除但驱动仍加载 | 文件/SCM 日志 | 正常卸载 |

## 2. 高价值关联

### A. 服务创建 → 驱动加载 → 设备打开

```text
7045 (ServiceType=1)
  → Sysmon 6 (ImageLoaded = 服务 ImagePath)
  → 60s 内 CreateFile(\\.\<device>)
```

单看 7045 误报高；加上“驱动加载 + 设备打开”显著提升置信度。

### B. 驱动对象无服务

```text
枚举 \Driver\* 对象
  → 对每个 DriverObject 取 DriverStart/DriverSize/驱动名
  → 查 Services\* 是否有对应 ImagePath
  → 无服务 + 有设备对象 = 可疑
```

### C. 模块外代码

```text
采样内核栈/回调/线程起始地址
  → 查地址是否落在任何已知模块 [Base, Base+Size)
  → 不在 + 页可执行 = 可疑
```

### D. 隐藏 vs 句柄

```text
符号链接不存在
  → 但句柄表中有 \Device\<name> 的句柄
  → 且该句柄可 IOCTL
```

## 3. 规则映射

| 文件 | 用途 |
|---|---|
| [`detection/sigma/byovd-driver-service.yml`](../detection/sigma/byovd-driver-service.yml) | 内核服务创建 |
| [`detection/sigma/driver-load-unsigned.yml`](../detection/sigma/driver-load-unsigned.yml) | 未签名/测试签名驱动加载 |
| [`detection/kql/driver-load.kql`](../detection/kql/driver-load.kql) | Defender XDR / Sentinel 模板 |
| [`detection/yara/README.md`](../detection/yara/README.md) | 手动映射样本的**内存扫描**思路（非通用文件规则） |

> 规则是模板，不是“开箱即用”的最终检测。请按自己的日志字段、系统版本和基线调整。

## 4. 误报控制

- 维护驱动签名者/哈希白名单；
- 区分测试签名与生产签名；
- 区分 `\Driver` 对象与合法 legacy 驱动；
- 对 JIT/安全产品的可执行池做例外；
- 对“服务删除但驱动仍加载”做时间窗口；
- 对一次性事件只记录，对重复/组合事件告警。

## 5. 响应

1. 隔离主机，保留内存/磁盘；
2. 记录驱动文件哈希、签名者、服务注册表、设备对象、句柄；
3. 检查 7045/4697/Sysmon 6/文件时间线；
4. 回滚到已知干净快照/镜像；
5. 撤销/封禁相关证书或驱动哈希；
6. 复盘检测缺口，更新规则。
