# Codebase Memory MCP v0.8.1 - Windows 增强版

基于原版 v0.8.1 进行了 Windows 兼容性修复和功能增强。

## 快速开始

### 1. 安装

将 `bin\codebase-memory-mcp.exe` 放到你喜欢的位置（或直接使用）。

### 2. 配置到你的 MCP 客户端

在你的 MCP 配置中添加：

```json
{
  "mcpServers": {
    "codebase-memory": {
      "command": "path\\to\\codebase-memory-mcp.exe"
    }
  }
}
```

### 3. 环境变量（可选）

| 环境变量 | 说明 | 示例 |
|---------|------|------|
| `CBM_DATA_DIR` | 数据存储目录（优先级最高） | `D:\codebase-memory-data` |
| `CBM_CACHE_DIR` | 缓存目录 | `D:\codebase-memory-cache` |
| `CBM_WATCHER_IGNORE` | 文件监听自定义排除模式 | `*.log;*.tmp;temp_dir` |

## 版本说明

### 修复内容（3 项）

1. **Windows 数据目录自动创建** — 缓存目录不存在时自动创建，不再失败
2. **中文路径支持** — 使用 `_wfopen` 解决 Windows 下中文/Unicode 路径文件读取失败
3. **数据库位置可配置** — 新增 `CBM_DATA_DIR` 环境变量，不再硬编码在 C 盘

### 增强内容（2 项）

1. **Watcher 自动读取 .gitignore** — 自动过滤 `node_modules`、`.git`、`build` 等无关目录
2. **Windows 事件驱动文件监听** — 使用 `ReadDirectoryChangesW` API 实现毫秒级响应，替代轮询

## 目录结构

```
codebase-memory-mcp-v0.8.1-windows-enhanced/
├── bin/
│   └── codebase-memory-mcp.exe    # 编译好的 Windows 可执行文件 (257MB)
├── docs/
│   ├── README.md                    # 原版 README
│   └── WINDOWS_FIXES.md             # 详细的修复/改进说明
├── source-patches/                  # 所有修改过的源代码文件
│   ├── src/
│   │   ├── foundation/
│   │   │   ├── platform.c
│   │   │   ├── compat_fs.h
│   │   │   └── compat_fs.c
│   │   └── watcher/
│   │       └── watcher.c
└── README_WINDOWS.md               # 本文件
```

## 详细文档

详见 [docs/WINDOWS_FIXES.md](docs/WINDOWS_FIXES.md)

## 原版信息

- 原版版本：v0.8.1
- 项目地址：https://github.com/nomic-phi/codebase-memory-mcp
- 本增强版基于原版进行 Windows 兼容性改进，不改变原有功能

---

**编译环境**：LLVM Clang 22.1.8 (MinGW-w64, x86_64)
**编译日期**：2026-06-27
