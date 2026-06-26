# Codebase Memory MCP - Windows Enhanced Edition

高性能代码智能 MCP 服务器的 **Windows 增强版**，在原版基础上针对 Windows 平台做了多项兼容性修复和功能增强。

> 原版项目：[codebase-memory-mcp](https://github.com/awalker/codebase-memory-mcp)（假设）
> 本仓库基于 v0.8.1 版本修改

## 本版本做了什么？

### 3 项 Windows 兼容性修复

| 修复 | 说明 |
|------|------|
| **数据目录自动创建** | Windows 下数据/缓存目录不存在时自动创建，不会报错退出 |
| **中文/Unicode 路径支持** | 所有文件操作使用 _wfopen 宽字符接口，中文路径不再乱码/失败 |
| **数据库位置可配置** | 支持 CBM_DATA_DIR 环境变量自定义数据存放位置，不再硬编码 C 盘 |

### 2 项功能增强

| 增强 | 说明 |
|------|------|
| **自动读取 .gitignore** | 文件监听自动解析项目 .gitignore，过滤无关目录，减少噪音 |
| **Windows 事件驱动监听** | 使用 ReadDirectoryChangesW API 实现毫秒级文件变更响应，替代轮询 |

## 快速开始

### Windows 用户（推荐）

直接下载预编译二进制：

1. 从 [Releases](https://github.com/mulouout/codebase-memory-mcp-windows/releases) 下载最新版 zip
2. 解压到任意目录
3. 配置 MCP 客户端使用 in/codebase-memory-mcp.exe

### 环境变量

| 变量 | 用途 |
|------|------|
| CBM_DATA_DIR | 自定义数据目录（数据库存放位置） |
| CBM_CACHE_DIR | 自定义缓存目录 |
| CBM_WATCHER_IGNORE | 自定义文件监听排除模式（逗号分隔） |

### 详细说明

- [Windows 修复与改进详细说明](WINDOWS_FIXES.md)
- [原版 README](README.md)

## 编译

`ash
# 需要 MinGW-w64 + zlib
make -f Makefile.cbm
`

## 与原版的关系

本项目是原作者 codebase-memory-mcp 的 Windows 增强分支，核心代码全部来自原作者，仅针对 Windows 平台做了兼容性修复和少量功能增强。如果原作者后续合并了这些改进，以原作者版本为准。

## License

与原版保持一致。
