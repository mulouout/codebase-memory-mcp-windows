# Codebase Memory MCP - Windows Enhanced Edition

A Windows-optimized version of the [official codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp) with enhanced functionality for Chinese/Unicode paths and event-driven file watching.

## ✨ Features

### Bug Fixes
- **Auto-create data directory**: Automatically creates data/cache directories on Windows startup
- **Chinese/Unicode path support**: Uses `_wfopen` wide-character API for proper Unicode file operations
- **Configurable database location**: Supports `CBM_DATA_DIR` environment variable to customize data path

### Enhancements
- **Auto-read .gitignore**: File watcher automatically parses project `.gitignore` files to filter irrelevant directories
- **Windows event-driven watching**: Uses `ReadDirectoryChangesW` API for millisecond-level file change detection (replaces polling)

## 📥 Download

| File | Size | Description |
|------|------|-------------|
| `codebase-memory-mcp-v0.8.1-windows-enhanced.zip` | ~35 MB | Pre-built Windows binary + documentation |
| `codebase-memory-mcp-v0.8.1-source.zip` | ~83 MB | Full source code (original v0.8.1) |

Download from the [Releases](https://github.com/mulouout/codebase-memory-mcp-windows/releases) page.

## 🚀 Quick Start

### Installation
1. Extract the zip archive
2. Run `codebase-memory-mcp.exe`

### Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `CBM_DATA_DIR` | Custom data directory for databases | `J:\cbm_data` |
| `CBM_CACHE_DIR` | Custom cache directory | `J:\cbm_cache` |
| `CBM_WATCHER_IGNORE` | Custom watcher exclude patterns (comma-separated) | `node_modules,.git` |

## 🔧 Technical Details

### Files Modified
- `src/foundation/platform.c` - Auto-create directory + `CBM_DATA_DIR` support
- `src/foundation/compat_fs.h` - Declare `cbm_fopen()` function
- `src/foundation/compat_fs.c` - Implement UTF-8 safe `cbm_fopen()` using `_wfopen`
- `src/watcher/watcher.c` - Gitignore integration + `ReadDirectoryChangesW` event-driven watching

### Building from Source
```bash
# Requires MinGW-w64 compiler
make clean
make -j4
```

## 📝 Changelog

### v0.8.1-windows
- Fixed: Auto-create data directory on Windows
- Fixed: Chinese/Unicode path support
- Fixed: Configurable database location via `CBM_DATA_DIR`
- Enhanced: Auto-read `.gitignore` for watcher filtering
- Enhanced: Windows `ReadDirectoryChangesW` event-driven file watching

## 📄 License

This project is based on the official [codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp) and follows the same license.

## 🤝 Contributing

Feel free to submit issues and pull requests!

## 📧 Contact

Email: 89021396@qq.com