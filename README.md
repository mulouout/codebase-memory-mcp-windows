# Codebase Memory MCP - Windows Enhanced Edition / Windows 澧炲己鐗?
**EN:** A Windows-optimized version of the [official codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp) with enhanced functionality for Chinese/Unicode paths, event-driven file watching, and bug fixes.

**CN:** 鍩轰簬鍘熺増 [codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp) 鐨?Windows 浼樺寲鐗堟湰锛屽鍔犱簡涓枃/Unicode 璺緞鏀寔銆佷簨浠堕┍鍔ㄦ枃浠剁洃鍚拰澶氶」 Bug 淇銆?
---

## 馃摝 Download / 涓嬭浇

**EN:** Download the pre-built binary from the [Releases](https://github.com/mulouout/codebase-memory-mcp-windows/releases) page.
**CN:** 浠?[Releases](https://github.com/mulouout/codebase-memory-mcp-windows/releases) 椤甸潰涓嬭浇棰勭紪璇戠殑浜岃繘鍒舵枃浠躲€?
| File / 鏂囦欢 | Size / 澶у皬 | Description / 璇存槑 |
|------|------|-------------|
| `codebase-memory-mcp-v0.8.2-windows-enhanced.zip` | ~35 MB | Recommended / 鎺ㄨ崘 - Pre-built binary + docs + patches |
| `codebase-memory-mcp-v0.8.2-source.zip` | ~83 MB | Full source code (original v0.8.1) / 瀹屾暣婧愪唬鐮?|

---

## 馃殌 Quick Start / 蹇€熷紑濮?
### Installation / 瀹夎

**EN:** Extract the zip archive and run `bin/codebase-memory-mcp.exe`.
**CN:** 瑙ｅ帇鍘嬬缉鍖咃紝杩愯 `bin/codebase-memory-mcp.exe`銆?
```json
// MCP client config / MCP 瀹㈡埛绔厤缃?{
  "mcpServers": {
    "codebase-memory": {
      "command": "path\\to\\codebase-memory-mcp.exe"
    }
  }
}
```

### Environment Variables / 鐜鍙橀噺

| Variable / 鍙橀噺 | Description / 璇存槑 | Example / 绀轰緥 |
|----------|-------------|---------|
| `CBM_DATA_DIR` | Data directory for databases / 鏁版嵁瀛樺偍鐩綍 | `D:\cbm_data` |
| `CBM_CACHE_DIR` | Cache directory / 缂撳瓨鐩綍 | `D:\cbm_cache` |
| `CBM_WATCHER_IGNORE` | Watcher exclude patterns / 鏂囦欢鐩戝惉鎺掗櫎妯″紡 | `*.log;*.tmp;temp_dir` |

---

## 鉁?Features / 鍔熻兘鐗规€?
### Bug Fixes / 闂淇

#### v0.8.2 (Latest / 鏈€鏂?
- **Fix: search_code fails on Chinese Windows usernames / 淇涓枃璺緞鎼滅储澶辫触**
  - `cbm_tmpdir()` now uses `GetTempPathW` for correct UTF-8 temp path
  - `cbm_init()` sets `sqlite3_temp_directory` for SQLite wide-char safety

#### v0.8.1
- **Auto-create data directory / 鏁版嵁鐩綍鑷姩鍒涘缓** - Automatically creates directories on Windows startup
- **Chinese/Unicode path support / 涓枃璺緞鏀寔** - Uses `_wfopen` for proper Unicode file operations
- **Configurable database location / 鏁版嵁搴撲綅缃彲閰嶇疆** - Supports `CBM_DATA_DIR` environment variable

### Enhancements / 鍔熻兘澧炲己
- **Auto-read .gitignore / 鑷姩璇诲彇 .gitignore** - File watcher parses `.gitignore` to filter irrelevant directories
- **Windows event-driven watching / 浜嬩欢椹卞姩鏂囦欢鐩戝惉** - Uses `ReadDirectoryChangesW` API for millisecond-level file change detection

---

## 馃敡 Patches / 淇敼鐨勬枃浠?
**EN:** All modified source files are in the `patches/` directory of the enhanced zip package.
**CN:** 鎵€鏈変慨鏀硅繃鐨勬簮鏂囦欢閮藉湪澧炲己鍖呯殑 `patches/` 鐩綍涓€?
| File / 鏂囦欢 | Version / 鐗堟湰 | Description / 璇存槑 |
|------|------|-------------|
| `src/foundation/compat.h` | v0.8.2 | `cbm_tmpdir()` GetTempPathW fix |
| `internal/cbm/cbm.c` | v0.8.2 | `sqlite3_temp_directory` init |
| `src/foundation/platform.c` | v0.8.1 | Auto-create dir + CBM_DATA_DIR |
| `src/foundation/compat_fs.h` | v0.8.1 | Declare `cbm_fopen()` |
| `src/foundation/compat_fs.c` | v0.8.1 | UTF-8 safe `cbm_fopen()` via `_wfopen` |
| `src/watcher/watcher.c` | v0.8.1 | Gitignore + ReadDirectoryChangesW |

### Building from Source / 浠庢簮鐮佺紪璇?
```bash
# Requires MinGW-w64 / 闇€瑕?MinGW-w64 缂栬瘧鍣?make clean
make -j4
```

---

## 馃摑 Changelog / 鏇存柊鏃ュ織

### v0.8.2-windows (2026-06-27)
- **Fixed / 淇**: search_code temp file creation on Chinese Windows usernames / 涓枃璺緞鎼滅储澶辫触
- Modified / 淇敼: compat.h, cbm.c

### v0.8.1-windows (2026-06-26)
- **Fixed / 淇**: Auto-create data directory / 鏁版嵁鐩綍鑷姩鍒涘缓
- **Fixed / 淇**: Chinese/Unicode path support / 涓枃璺緞鏀寔
- **Fixed / 淇**: Configurable database location / 鏁版嵁搴撲綅缃彲閰嶇疆
- **Enhanced / 澧炲己**: Auto-read .gitignore for watcher
- **Enhanced / 澧炲己**: Windows ReadDirectoryChangesW event-driven watching

---

## 馃搫 License / 璁稿彲

**EN:** Based on the official [codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp) with the same license.
**CN:** 鍩轰簬瀹樻柟 [codebase-memory-mcp](https://github.com/continualai/codebase-memory-mcp)锛岄伒寰浉鍚岃鍙崗璁€?
## 馃摟 Contact / 鑱旂郴鏂瑰紡

**EN:** Email: 89021396@qq.com | Issues: [GitHub Issues](https://github.com/mulouout/codebase-memory-mcp-windows/issues)
**CN:** 閭: 89021396@qq.com | 鎻愪氦闂: [GitHub Issues](https://github.com/mulouout/codebase-memory-mcp-windows/issues)