# Codebase Memory MCP v0.8.2 - Windows 澧炲己鐗堬紙淇鐗堬級

鍩轰簬鍘熺増 v0.8.1 杩涜浜?Windows 鍏煎鎬т慨澶嶅拰鍔熻兘澧炲己銆?
## 蹇€熷紑濮?
### 1. 瀹夎

灏?`bin\codebase-memory-mcp.exe` 鏀惧埌浣犲枩娆㈢殑浣嶇疆锛堟垨鐩存帴浣跨敤锛夈€?
### 2. 閰嶇疆鍒颁綘鐨?MCP 瀹㈡埛绔?
鍦ㄤ綘鐨?MCP 閰嶇疆涓坊鍔狅細

```json
{
  "mcpServers": {
    "codebase-memory": {
      "command": "path\\to\\codebase-memory-mcp.exe"
    }
  }
}
```

### 3. 鐜鍙橀噺锛堝彲閫夛級

| 鐜鍙橀噺 | 璇存槑 | 绀轰緥 |
|---------|------|------|
| `CBM_DATA_DIR` | 鏁版嵁瀛樺偍鐩綍锛堜紭鍏堢骇鏈€楂橈級 | `D:\codebase-memory-data` |
| `CBM_CACHE_DIR` | 缂撳瓨鐩綍 | `D:\codebase-memory-cache` |
| `CBM_WATCHER_IGNORE` | 鏂囦欢鐩戝惉鑷畾涔夋帓闄ゆā寮?| `*.log;*.tmp;temp_dir` |

## 涓嬭浇鎸囧崡

姣忎釜 Release 鍖呭惈涓や釜鏂囦欢锛?
| 鏂囦欢 | 澶у皬 | 璇存槑 |
|------|------|------|
| `codebase-memory-mcp-v0.8.2-windows-enhanced.zip` | ~35 MB | **鎺ㄨ崘涓嬭浇** 鈥?棰勭紪璇戜簩杩涘埗 + 鏂囨。 + 婧愮爜 patches |
| `codebase-memory-mcp-v0.8.2-source.zip` | ~83 MB | 瀹屾暣婧愪唬鐮侊紙鍘熺増 v0.8.1锛?|

## Release 鐗堟湰璇存槑

### v0.8.2 (2026-06-27) 鈥?淇 search_code 鎼滅储澶辫触 bug

**鏂板淇**锛?1. **`cbm_tmpdir()` UTF-8 璺緞淇** 鈥?Windows 涓存椂鐩綍鍚腑鏂囨椂鐢?`GetTempPathW` 鑾峰彇姝ｇ‘ UTF-8 璺緞
2. **`sqlite3_temp_directory` 鍒濆鍖?* 鈥?SQLite 鍐呴儴涓存椂鏂囦欢鍒涘缓浣跨敤瀹藉瓧绗﹀畨鍏ㄨ矾寰?
**淇闂**锛氫腑鏂?Windows 鐢ㄦ埛鍚嶄笅 `search_code` 鎶?"cannot create temp file"

### v0.8.1 鈥?棣栨 Windows 澧炲己鐗?
**淇鍐呭锛? 椤癸級**锛?1. Windows 鏁版嵁鐩綍鑷姩鍒涘缓
2. 涓枃/Unicode 璺緞鏀寔锛坄_wfopen`锛?3. 鏁版嵁搴撲綅缃彲閰嶇疆锛坄CBM_DATA_DIR`锛?
**澧炲己鍐呭锛? 椤癸級**锛?1. Watcher 鑷姩璇诲彇 .gitignore
2. Windows 浜嬩欢椹卞姩鏂囦欢鐩戝惉锛坄ReadDirectoryChangesW`锛?
## 鐩綍缁撴瀯

```
codebase-memory-mcp-windows-enhanced/
鈹溾攢鈹€ bin/
鈹?  鈹斺攢鈹€ codebase-memory-mcp.exe    # 缂栬瘧濂界殑 Windows 鍙墽琛屾枃浠?(~258MB)
鈹溾攢鈹€ docs/
鈹?  鈹斺攢鈹€ WINDOWS_FIXES.md            # 璇︾粏鐨勪慨澶?鏀硅繘璇存槑
鈹溾攢鈹€ patches/                         # 鎵€鏈変慨鏀硅繃鐨勬簮浠ｇ爜鏂囦欢
鈹?  鈹溾攢鈹€ src/foundation/             # foundation 灞備慨澶?鈹?  鈹溾攢鈹€ src/watcher/                # 鏂囦欢鐩戝惉鏀硅繘
鈹?  鈹斺攢鈹€ internal/cbm/               # 鏍稿績鍒濆鍖栦慨澶?鈹斺攢鈹€ README_WINDOWS.md               # 鏈枃浠?```

## 璇︾粏鏂囨。

璇﹁ [docs/WINDOWS_FIXES.md](WINDOWS_FIXES.md)

## 鍘熺増淇℃伅

- 鍘熺増鐗堟湰锛歷0.8.1
- 椤圭洰鍦板潃锛歨ttps://github.com/nomic-phi/codebase-memory-mcp
- 鏈寮虹増鍩轰簬鍘熺増杩涜 Windows 鍏煎鎬ф敼杩涳紝涓嶆敼鍙樺師鏈夊姛鑳?
---

**缂栬瘧鐜**锛歀LVM Clang 22.1.8 (MinGW-w64, x86_64) / MSVC 2022
**缂栬瘧鏃ユ湡**锛?026-06-27