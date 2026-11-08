# Known Vulnerable Driver Hashes

这个目录只放**检测元数据**（文件名、SHA256、版本、签名者、处置建议），不放驱动二进制。

## 使用

- 在 EDR/AV 中按 SHA256 拦截；
- 在 WDAC/App Control 中按签名者 + 文件名 + 版本拦截；
- 在 Microsoft Vulnerable Driver Blocklist 中确认已有条目；
- 定期更新：厂商可能发布新版本/修复，旧版本仍应拦截。

## 字段

| 字段 | 说明 |
|---|---|
| `filename` | 驱动文件名 |
| `sha256` | 样本哈希 |
| `version` | 文件版本 |
| `signer` | 签名者 |
| `action` | `block` / `audit` |
| `note` | 来源/说明 |

## 注意

- 哈希只对应一个具体版本；攻击者可能换版本/重新签名。
- 不要只依赖哈希；结合签名者、文件名、服务、设备、行为。
- 不要从本仓库下载/分发驱动二进制。
