# Contributing

## Before you open a PR

- [ ] I have read [AUTHORIZED_USE.md](AUTHORIZED_USE.md) and [docs/09-publication-boundary.md](docs/09-publication-boundary.md).
- [ ] My change does not add a real vulnerable driver binary, kdmapper, or a complete manual-mapping/hiding loader.
- [ ] My change does not add specific target/game-specific offsets, symbols, or credentials.
- [ ] My change does not add `.pfx`, `.pvk`, `.cat`, `.sys`, `.exe` or `.dmp` files.
- [ ] If I add a detection rule, I documented false positives and a baseline test.

## Good contributions

- 更准确的检测规则（附误报说明）；
- 更好的 benign 示例（仍只做 PING/VERSION 级别的操作）；
- 文档勘误、图表、术语统一；
- 隔离实验/清理流程的改进；
- 对 defenders 更有用的交叉校验方法。

## Not accepted

- 真实易受攻击驱动的下载/哈希/二进制；
- 完整 manual mapper / DKOM / 对象摘名 / 模块隐藏；
- 针对具体反作弊/游戏的绕过步骤；
- 测试证书私钥或生产签名材料。

## Style

- 中文为主，技术术语保留英文；
- 代码可编译、无 TODO；
- 命令用 fenced code block；
- 新增文件放进对应目录，并更新 README 索引。
