# Codebase Memory MCP v0.8.2 - Windows 鍏煎鎬т慨澶嶅拰鏀硅繘

鏈枃妗ｈ褰曚簡閽堝 Codebase Memory MCP v0.8.1 鐨?Windows 鍏煎鎬т慨澶嶅拰鍔熻兘鏀硅繘銆?
## 淇鍒楄〃

### 淇1锛歐indows 鏁版嵁鐩綍鑷姩鍒涘缓澶辫触

**闂鎻忚堪锛?*
鍦?Windows 绯荤粺涓婏紝褰撶紦瀛樼洰褰曚笉瀛樺湪鏃讹紝绋嬪簭鍙兘浼氬洜涓烘棤娉曞垱寤虹洰褰曡€屽け璐ャ€?
**瑙ｅ喅鏂规锛?*
- 鍦?`src/foundation/platform.c` 鐨?`cbm_resolve_cache_dir()` 鍑芥暟涓紝鍦ㄦ墍鏈夎繑鍥炶矾寰勭殑鍦版柟閮芥坊鍔犱簡 `cbm_mkdir_p(buf, 0755)` 璋冪敤锛岀‘淇濈洰褰曡嚜鍔ㄥ垱寤恒€?- 鏂板浜?`CBM_DATA_DIR` 鐜鍙橀噺鏀寔锛堜紭鍏堢骇楂樹簬 `CBM_CACHE_DIR`锛夛紝鍏佽鐢ㄦ埛鑷畾涔夋暟鎹瓨鍌ㄤ綅缃€?
**褰卞搷鏂囦欢锛?*
- `src/foundation/platform.c`

**浣跨敤鏂瑰紡锛?*
```bash
# 璁剧疆鏁版嵁鐩綍锛堜紭鍏堢骇鏈€楂橈級
set CBM_DATA_DIR=D:\codebase-memory-data

# 鎴栬€呰缃紦瀛樼洰褰?set CBM_CACHE_DIR=D:\codebase-memory-cache
```

---

### 淇2锛氫腑鏂囪矾寰勪笅鏂囦欢璇诲彇澶辫触锛坈bm_fopen锛?
**闂鎻忚堪锛?*
鍦?Windows 绯荤粺涓婏紝褰撴枃浠惰矾寰勫寘鍚腑鏂囨垨鍏朵粬闈?ASCII 瀛楃鏃讹紝鏍囧噯鐨?`fopen()` 鍑芥暟鍙兘鏃犳硶姝ｇ‘鎵撳紑鏂囦欢锛屽鑷寸储寮曞け璐ャ€?
**瑙ｅ喅鏂规锛?*
- 鏂板浜嗚法骞冲彴鐨?`cbm_fopen()` 鍑芥暟锛?  - **Windows 鐗堟湰锛?* 浣跨敤 `_wfopen()` 閰嶅悎 UTF-8 鍒板瀛楃鐨勮浆鎹?  - **POSIX 鐗堟湰锛?* 鐩存帴璋冪敤鏍囧噯 `fopen()`
- 灏嗘墍鏈?19 涓簮鏂囦欢涓殑 `fopen()` 璋冪敤鏇挎崲涓?`cbm_fopen()`

**褰卞搷鏂囦欢锛?*
- `src/foundation/compat_fs.h` - 鏂板澹版槑
- `src/foundation/compat_fs.c` - 鏂板瀹炵幇
- 19 涓?pipeline 鍜屽伐鍏锋簮鏂囦欢涓殑 `fopen()` 鈫?`cbm_fopen()`

---

### 淇3锛氭暟鎹簱浣嶇疆纭紪鐮佸湪 C 鐩橈紙CBM_DATA_DIR锛?
鍚屼慨澶?锛屾柊澧?`CBM_DATA_DIR` 鐜鍙橀噺銆?
---

### 淇4锛氫腑鏂囦复鏃剁洰褰曚笅 `search_code` 鎼滅储澶辫触锛坴0.8.2 鏂板锛?
**闂鎻忚堪锛?*
褰?Windows 鐢ㄦ埛鍚嶅寘鍚腑鏂囧瓧绗︼紙濡?`C:\Users\璋㈠博.DELL-PC`锛夋椂锛屾墽琛?`search_code` 鎶ラ敊锛?```
search failed: cannot create temp file (No such file or directory)
```

**鏍规湰鍘熷洜锛?*
涓ゅ眰璺緞缂栫爜闂锛?1. `getenv("TEMP")` 杩斿洖 ANSI 缂栫爜璺緞锛屽湪 `cbm_fopen()` 涓褰撲綔 UTF-8 浼犵粰 `_wfopen()`锛屽鑷磋矾寰勪贡鐮?2. SQLite FTS5 鎼滅储绱㈠紩鍒涘缓鏃朵娇鐢ㄥ悓涓€涔辩爜涓存椂璺緞锛屼篃鍒涘缓澶辫触

**瑙ｅ喅鏂规锛? 涓枃浠讹級锛?*

1. **`src/foundation/compat.h`** 鈥?`cbm_tmpdir()` 鏀圭敤 `GetTempPathW` 瀹藉瓧绗?API
   - 鐢?`GetTempPathW` + `WideCharToMultiByte` 鑾峰彇姝ｇ‘ UTF-8 涓存椂鐩綍璺緞
   - 纭繚 `cbm_fopen` 鈫?`_wfopen` 鐨勮矾寰勭紪鐮佷竴鑷?
2. **`internal/cbm/cbm.c`** 鈥?`cbm_init()` 璁剧疆 `sqlite3_temp_directory`
   - 鐢?`GetTempPathW` 鑾峰彇瀹藉瓧绗︿复鏃剁洰褰曪紝杞崲涓?UTF-8
   - 璧嬪€肩粰 SQLite 鍏ㄥ眬鍙橀噺锛岃 FTS5 浣跨敤瀹藉瓧绗﹀畨鍏ㄨ矾寰勫垱寤轰复鏃舵枃浠?
**褰卞搷鏂囦欢锛?*
- `src/foundation/compat.h`
- `internal/cbm/cbm.c`

**楠岃瘉锛?*
```bash
# 绱㈠紩鎴愬姛涓旀寔涔呭寲
index_repository 鈫?nodes: 9, edges: 9 鉁?list_projects    鈫?椤圭洰淇℃伅瀹屾暣 鉁?search_code      鈫?姝ｅ父杩斿洖缁撴灉锛堟棤 temp file 閿欒锛夆渽
```

---

## 鏀硅繘鍒楄〃

### 鏀硅繘1锛歐atcher 鑷姩璇诲彇 .gitignore锛岃繃婊ゆ棤鍏崇洰褰?
**闂鎻忚堪锛?*
瀵逛簬闈?Git 椤圭洰锛學atcher 鏃犳硶杩囨护鏃犲叧鐩綍锛堝 node_modules銆?git銆乥uild 绛夛級銆?
**瑙ｅ喅鏂规锛?*
- 鍦?`project_state_t` 缁撴瀯浣撲腑鏂板 `gitignore` 瀛楁
- 椤圭洰鍒濆鍖栨椂鑷姩鍔犺浇鏍圭洰褰曚笅鐨?`.gitignore` 鏂囦欢
- 鍐呯疆甯哥敤鎺掗櫎鐩綍鍒楄〃锛歯ode_modules, .git, build, dist, .idea, .vscode 绛?- 鏂板 `CBM_WATCHER_IGNORE` 鐜鍙橀噺鑷畾涔夋帓闄ゆā寮?
**褰卞搷鏂囦欢锛?*
- `src/watcher/watcher.c`

---

### 鏀硅繘2锛歐indows 涓嬬敤 ReadDirectoryChangesW 瀹炵幇浜嬩欢椹卞姩鐩戝惉

**闂鎻忚堪锛?*
闈?Git 椤圭洰鍙兘閫氳繃杞妫€娴嬫枃浠跺彉鍖栵紝鍝嶅簲鎱笖鑰楄祫婧愩€?
**瑙ｅ喅鏂规锛?*
- 浣跨敤 `ReadDirectoryChangesW` API 瀹炵幇姣绾х洰褰曞彉鏇寸洃鍚?- 鑷姩杩囨护缂栬緫鍣ㄤ复鏃舵枃浠讹紙.tmp, .bak, .swp锛?- 绾跨▼瀹夊叏璁捐锛岄厤鍚堣疆璇綔涓哄悗澶?
**褰卞搷鏂囦欢锛?*
- `src/watcher/watcher.c`

---

## 淇敼鐨勬枃浠舵眹鎬?
| 鏂囦欢璺緞 | 鐗堟湰 | 璇存槑 |
|---------|------|------|
| `src/foundation/compat.h` | v0.8.2 | `cbm_tmpdir()` GetTempPathW 淇 |
| `internal/cbm/cbm.c` | v0.8.2 | `sqlite3_temp_directory` 鍒濆鍖?|
| `src/foundation/platform.c` | v0.8.1 | CBM_DATA_DIR + 鑷姩鍒涘缓鐩綍 |
| `src/foundation/compat_fs.h` | v0.8.1 | cbm_fopen 澹版槑 |
| `src/foundation/compat_fs.c` | v0.8.1 | cbm_fopen 瀹炵幇 (_wfopen) |
| `src/watcher/watcher.c` | v0.8.1 | 澶氶」 Windows 澧炲己鍔熻兘 |
| 19 涓?pipeline 婧愭枃浠?| v0.8.1 | fopen 鈫?cbm_fopen |

---

**缂栬瘧鐜**锛歀LVM Clang 22.1.8 (MinGW-w64, x86_64)
**缂栬瘧鏃ユ湡**锛?026-06-27