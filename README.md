# BYOVD & Manual Mapping Lab — Defensive Research Edition

> 一个面向 **授权红蓝对抗 / 内核安全研究** 的开源资料库：把 BYOVD、INF 服务安装、手动映射、IOCTL 通信和隐藏面分析拆成可复现的实验步骤，并给出对应的检测与缓解方法。

## 这个仓库是什么

- **可复现的 benign 实验驱动**：一个只做 `PING` / `VERSION` 的 WDM 驱动 + INF + 用户态 IOCTL 客户端。
- **BYOVD 生命周期分析**：驱动获取、暂存、服务加载、IOCTL 利用、清理，以及每个阶段的遥测点。
- **手动映射的概念与检测**：PE 映射阶段、入口点、重定位、导入、内存特征，以及 defenders 应该看什么。
- **隐藏面清单**：服务/注册表、驱动对象、模块链表、设备与符号链接、句柄、池内存；每项对应检测方法。
- **检测工程素材**：Sigma / KQL / YARA 模板，可直接改成自己的规则。

## 这个仓库不是什么

公开版**刻意不包含**：

- 真实易受攻击的 OEM / 第三方驱动二进制（例如用于 BYOVD 的签名驱动）；
- `kdmapper` 或任何完整的手动映射 loader；
- DKOM 摘链、对象目录摘名、模块链表隐藏等可直接运行的隐蔽实现；
- ACE / 游戏 / 具体目标进程的偏移、符号、绕过步骤；
- 任何测试证书私钥、生产签名材料或未公开漏洞细节。

这些内容即使在本项目的授权实验区存在，也不应进入公开仓库。参见 [docs/09-publication-boundary.md](docs/09-publication-boundary.md)。

## 目录

```text
.
├─ README.md
├─ AUTHORIZED_USE.md
├─ SECURITY.md
├─ LICENSE
├─ docs/
│  ├─ 01-threat-model.md
│  ├─ 02-byovd-lifecycle.md
│  ├─ 03-inf-and-service-loading.md
│  ├─ 04-manual-mapping.md
│  ├─ 05-ioctl-communication.md
│  ├─ 06-hiding-and-detection.md
│  ├─ 07-detection-matrix.md
│  ├─ 08-lab-setup.md
│  └─ 09-publication-boundary.md
├─ samples/
│  ├─ benign-driver/        # LabDrv.c / LabDrv.inf / LabDrv.h
│  └─ client/               # labctl.c
├─ detection/
│  ├─ sigma/
│  ├─ kql/
│  └─ yara/
└─ diagrams/
```

## 快速开始（隔离 VM）

1. 准备 Windows 10/11 x64 测试 VM，关闭 Secure Boot，开启 `testsigning`，打快照。
2. 用 WDK 构建 `samples/benign-driver`，测试签名 `LabDrv.sys`。
3. 用 `samples/benign-driver/LabDrv.inf` 安装服务，或按 README 用 `sc create`。
4. 构建并运行 `samples/client/labctl.exe`，确认 `PING` / `VERSION` 返回。
5. 按 [docs/08-lab-setup.md](docs/08-lab-setup.md) 开启 Sysmon / 审核策略，观察驱动加载、服务创建、设备打开、IOCTL 调用产生的遥测。
6. 回滚快照，删除测试证书。

> 只在隔离 VM 中运行。不要在日常主机、生产机或他人的机器上加载任何内核驱动。

## 给 defenders 的入口

- 先看 [docs/07-detection-matrix.md](docs/07-detection-matrix.md)：按攻击阶段列出遥测与规则。
- 再看 [docs/02-byovd-lifecycle.md](docs/02-byovd-lifecycle.md)：BYOVD 的每个阶段分别留下什么。
- 最后看 [docs/06-hiding-and-detection.md](docs/06-hiding-and-detection.md)：隐藏面与交叉校验。

## 给研究者的入口

- [docs/03-inf-and-service-loading.md](docs/03-inf-and-service-loading.md)：INF / 服务 / 加载路径。
- [docs/04-manual-mapping.md](docs/04-manual-mapping.md)：手动映射的阶段与失败模式（概念级，不含 loader）。
- [docs/05-ioctl-communication.md](docs/05-ioctl-communication.md)：IOCTL 契约、调用方校验、模糊测试。

## 授权与伦理

本仓库只用于**经过书面授权的红蓝对抗、CTF、隔离实验和防御研究**。使用者必须确保对目标系统有明确授权，并遵守当地法律。详见 [AUTHORIZED_USE.md](AUTHORIZED_USE.md)。

## License

代码与文档以 MIT License 发布；见 [LICENSE](LICENSE)。
