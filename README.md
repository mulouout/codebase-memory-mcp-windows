# Codebase Memory MCP - Windows Enhanced Edition / Windows 增强版

EN: A Windows-optimized version of the official codebase-memory-mcp (https://github.com/continualai/codebase-memory-mcp) with enhanced functionality for Chinese/Unicode paths, event-driven file watching, and bug fixes.

CN: 基于原版 codebase-memory-mcp 的 Windows 优化版本，增加了中文/Unicode 路径支持、事件驱动文件监听和多项 Bug 修复。

---

## Download / 下载

EN: Download the pre-built binary from the Releases page (https://github.com/mulouout/codebase-memory-mcp-windows/releases).
CN: 从 Releases 页面下载预编译的二进制文件。

| File / 文件 | Size / 大小 | Description / 说明 |
|------|------|-------------|
| codebase-memory-mcp-v0.8.2-windows-enhanced.zip | ~35 MB | Recommended / 推荐 - Pre-built binary + docs + patches |
| codebase-memory-mcp-v0.8.2-source.zip | ~83 MB | Full source code (original v0.8.1) / 完整源代码 |

---

## Quick Start / 快速开始

EN: Extract the zip archive and run bin/codebase-memory-mcp.exe.
CN: 解压压缩包，运行 bin/codebase-memory-mcp.exe.

MCP client config:
```
{
  "mcpServers": {
    "codebase-memory": {
      "command": "path\\to\\codebase-memory-mcp.exe"
    }
  }
}
```

Environment Variables / 环境变量:

| Variable / 变量 | Description / 说明 | Example / 示例 |
|----------|-------------|---------|
| CBM_DATA_DIR | Data directory / 数据存储目录 | D:\cbm_data |
| CBM_CACHE_DIR | Cache directory / 缓存目录 | D:\cbm_cache |
| CBM_WATCHER_IGNORE | Watcher exclude / 监听排除模式 | *.log;*.tmp;temp_dir |

---

## Features / 功能特性

### v0.8.2 (Latest / 最新)
- Fix: search_code fails on Chinese Windows usernames / 修复中文路径搜索失败
  - cbm_tmpdir() now uses GetTempPathW for correct UTF-8 temp path
  - cbm_init() sets sqlite3_temp_directory for SQLite wide-char safety

### v0.8.1
- Auto-create data directory / 数据目录自动创建
- Chinese/Unicode path support (_wfopen) / 中文路径支持
- Configurable database location (CBM_DATA_DIR) / 数据库位置可配置
- Auto-read .gitignore / 自动读取 .gitignore
- Windows event-driven watching (ReadDirectoryChangesW) / 事件驱动文件监听

---

## Patches / 修改的文件

| File / 文件 | Ver / 版本 | Description / 说明 |
|------|------|-------------|
| src/foundation/compat.h | v0.8.2 | cbm_tmpdir() GetTempPathW fix |
| internal/cbm/cbm.c | v0.8.2 | sqlite3_temp_directory init |
| src/foundation/platform.c | v0.8.1 | Auto-create dir + CBM_DATA_DIR |
| src/foundation/compat_fs.h/c | v0.8.1 | UTF-8 safe cbm_fopen() via _wfopen |
| src/watcher/watcher.c | v0.8.1 | Gitignore + ReadDirectoryChangesW |

### Build from Source / 从源码编译
```
make clean && make -j4
```

---

## Changelog / 更新日志

### v0.8.2-windows (2026-06-27)
- Fixed: search_code temp file on Chinese usernames / 修复中文路径搜索失败

### v0.8.1-windows (2026-06-26)
- Fixed: Auto-create data dir, Chinese path, CBM_DATA_DIR
- Enhanced: .gitignore watcher, ReadDirectoryChangesW

---

License / 许可: Based on official codebase-memory-mcp, same license.

Contact / 联系: 89021396@qq.com | GitHub Issues
