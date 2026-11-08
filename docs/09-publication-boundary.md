# 09 · Publication Boundary

公开仓库的目标是**可复现的防御研究**，不是分发可直接 weaponize 的内核工具。以下内容被刻意排除。

## 1. 不包含的二进制

- 真实易受攻击的 OEM/第三方签名驱动（BYOVD 的“B”），包括 `AmdTools64.sys` 等已知样本的**二进制**。
- `kdmapper`、`kdmapper_kfp` 或任何现成 manual mapper。
- 带有任意物理/虚拟内存读写、MSR/端口访问、进程操作的危险驱动。
- 完整 payload、shellcode、DSE/CI bypass。
- ACE/游戏/目标进程的模块、偏移、符号、版本数据。

> 文件名、版本、签名者、SHA256 可以保留在检测目录中（如 `detection/hashes/`），但二进制、INF、服务安装包和 loader 不进入公开仓库。

## 2. 不包含的实现

- 完整 manual mapping loader（分配、复制、重定位、导入解析、入口调用）。
- DKOM `ActiveProcessLinks` 摘链。
- `\Driver` / `\Device` 对象摘名。
- `PsLoadedModuleList` 隐藏。
- 池中 PE 头清零、代码页恢复。
- Ob/Ps 回调隐藏、inline hook。
- 服务/注册表/文件清理的“无痕”脚本。

## 3. 为什么

1. **可被直接滥用**：完整 loader + 真实漏洞驱动 = 任意内核代码执行/反检测绕过。
2. **供应链风险**：公开仓库中的驱动二进制会被镜像、篡改、二次分发。
3. **法律与授权**：ACE/游戏测试授权不等于公开分发绕过工具。
4. **防御价值更高**：检测矩阵、交叉校验、误报控制比再放一个 loader 更有长期价值。

## 4. 保留的内容

- 概念级攻击链与阶段划分；
- benign 驱动 + INF + IOCTL 客户端；
- 加载/服务/设备/IOCTL 的遥测点；
- 手动映射的失败模式与检测信号（不含代码）；
- 隐藏面的交叉校验；
- Sigma/KQL/YARA 模板；
- 隔离实验与清理流程；
- **已知易受攻击驱动的元数据/哈希**（用于检测和拦截，不含二进制、INF、loader）。

例如 [11-amdtools64-case-study.md](11-amdtools64-case-study.md) 只保留文件名、版本、签名者和 SHA256；驱动文件本身不在仓库里。

## 5. 如果确实需要完整实验

在**授权的隔离实验区**内，按项目授权范围保存：

- 原始驱动/loader 只在离线实验目录；
- 不提交到 Git；
- 使用独立测试证书；
- 实验后回滚快照；
- 不把目标/ACE/游戏数据带入公开仓库。

## 6. 贡献者检查

提交 PR 前确认：

- [ ] 没有 `.sys` / `.exe` / `.pfx` / `.cat` 等二进制或私钥；
- [ ] 没有真实易受攻击驱动的名称+哈希+下载地址组合；
- [ ] 没有完整 manual mapper / DKOM / 隐藏实现；
- [ ] 没有 ACE/游戏/生产目标偏移；
- [ ] 文档中的代码只做 benign 的 PING/VERSION/IOCTL 演示。
