# Codebase Memory MCP v0.8.1 - Windows 兼容性修复和改进

本文档记录了针对 Codebase Memory MCP v0.8.1 的 Windows 兼容性修复和功能改进。

## 修复列表

### 修复1：Windows 数据目录自动创建失败

**问题描述：**
在 Windows 系统上，当缓存目录不存在时，程序可能会因为无法创建目录而失败。

**解决方案：**
- 在 `src/foundation/platform.c` 的 `cbm_resolve_cache_dir()` 函数中，在所有返回路径的地方都添加了 `cbm_mkdir_p(buf, 0755)` 调用，确保目录自动创建。
- 新增了 `CBM_DATA_DIR` 环境变量支持（优先级高于 `CBM_CACHE_DIR`），允许用户自定义数据存储位置。

**影响文件：**
- `src/foundation/platform.c`

**使用方式：**
```bash
# 设置数据目录（优先级最高）
set CBM_DATA_DIR=D:\codebase-memory-data

# 或者设置缓存目录
set CBM_CACHE_DIR=D:\codebase-memory-cache
```

---

### 修复2：中文路径下文件读取失败（cbm_fopen）

**问题描述：**
在 Windows 系统上，当文件路径包含中文或其他非 ASCII 字符时，标准的 `fopen()` 函数可能无法正确打开文件，导致索引失败。

**解决方案：**
- 新增了跨平台的 `cbm_fopen()` 函数：
  - **Windows 版本：** 使用 `_wfopen()` 配合 UTF-8 到宽字符的转换
  - **POSIX 版本：** 直接调用标准 `fopen()`
- 将所有 19 个源文件中的 `fopen()` 调用替换为 `cbm_fopen()`

**影响文件：**
- `src/foundation/compat_fs.h` - 新增声明
- `src/foundation/compat_fs.c` - 新增实现
- `src/pipeline/pass_definitions.c`
- `src/pipeline/pass_calls.c`
- `src/pipeline/pass_parallel.c`
- `src/pipeline/pass_k8s.c`
- `src/pipeline/pass_lsp_cross.c`
- `src/pipeline/pass_pkgmap.c`
- `src/pipeline/pass_semantic.c`
- `src/pipeline/pass_usages.c`
- `src/pipeline/artifact.c`
- `src/pipeline/pass_envscan.c`
- `src/pipeline/path_alias.c`
- `src/discover/gitignore.c`
- `src/discover/language.c`
- `src/discover/userconfig.c`
- `src/mcp/mcp.c`
- `src/cli/cli.c`
- `src/ui/config.c`
- `src/ui/http_server.c`
- `src/foundation/diagnostics.c`

---

### 修复3：数据库位置硬编码在 C 盘（CBM_DATA_DIR）

**问题描述：**
在 Windows 系统上，默认缓存目录位于 C 盘用户目录下，当 C 盘空间不足或用户希望将数据存储在其他位置时不够灵活。

**解决方案：**
- 新增 `CBM_DATA_DIR` 环境变量，优先级高于 `CBM_CACHE_DIR`
- 用户可以通过设置此环境变量将数据库和缓存文件存储到任意位置
- 目录会自动创建，无需手动干预

**使用方式：**
```bash
# 将数据存储到 D 盘
set CBM_DATA_DIR=D:\codebase-memory-data
```

---

## 改进列表

### 改进1：Watcher 自动读取 .gitignore，过滤无关目录

**问题描述：**
对于非 Git 项目，Watcher 无法过滤无关目录（如 node_modules、.git、build 等），导致文件计数不准确和不必要的重新索引。

**解决方案：**
- 在 `project_state_t` 结构体中新增 `gitignore` 字段
- 项目初始化时自动加载根目录下的 `.gitignore` 文件
- 新增 `is_default_skip_dir()` 函数，内置常用排除目录列表：
  - node_modules, .git, .svn, .hg
  - build, dist, target
  - .idea, .vscode, .cache
  - vendor, third_party
  - __pycache__, .next, .nuxt
- 新增 `count_files_recursive_impl()` 函数，递归统计文件时自动应用 gitignore 过滤
- 新增 `CBM_WATCHER_IGNORE` 环境变量支持，用户可自定义排除模式（用分号或逗号分隔）

**使用方式：**
```bash
# 自定义排除模式
set CBM_WATCHER_IGNORE=*.log;*.tmp;temp_dir
```

**影响文件：**
- `src/watcher/watcher.c`

---

### 改进2：Windows 下用 ReadDirectoryChangesW 实现事件驱动监听

**问题描述：**
在 Windows 系统上，非 Git 项目只能通过轮询方式检测文件变化，响应速度慢且资源消耗较高。

**解决方案：**
- 在 `project_state_t` 结构体中新增 Windows 专用字段：
  - `watch_dir_handle` - 目录监听句柄
  - `watch_overlapped` - 重叠 I/O 结构
  - `watch_buffer[65536]` - 事件缓冲区
  - `watch_active` - 监听状态标志
  - `watch_thread_handle` - 后台线程句柄
  - `watch_cs` - 临界区（线程同步）
  - `pending_change` - 待处理变更标志
- 新增 `watch_directory_thread()` 函数：使用 `ReadDirectoryChangesW` 实现后台事件监听
- 新增 `start_watch_thread()` / `stop_watch_thread()` 函数：启动/停止监听线程
- 新增 `poll_watch_change()` 函数：非阻塞检查是否有变化
- 新增 `should_trigger_reindex_win()` 函数：过滤临时文件（.tmp, .bak, .swp, ~ 等）
- 非 Git 项目在 Windows 上自动启用事件驱动监听，配合轮询作为后备

**技术特点：**
- 使用 overlapped I/O 实现高效的目录变更监听
- 监听文件/目录的创建、删除、重命名、修改等事件
- 自动过滤编辑器临时文件，避免不必要的重新索引
- 线程安全的变更标志设计，确保主线程可以安全轮询

**影响文件：**
- `src/watcher/watcher.c`

---

## 修改的文件汇总

| 文件路径 | 修改类型 | 说明 |
|---------|---------|------|
| `src/foundation/platform.c` | 修复 | 添加 CBM_DATA_DIR 支持 + 自动创建目录 |
| `src/foundation/compat_fs.h` | 修复 | 新增 cbm_fopen 声明 |
| `src/foundation/compat_fs.c` | 修复 | 新增 cbm_fopen 实现 |
| `src/pipeline/pass_definitions.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_calls.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_parallel.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_k8s.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_lsp_cross.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_pkgmap.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_semantic.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_usages.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/artifact.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/pass_envscan.c` | 修复 | fopen → cbm_fopen |
| `src/pipeline/path_alias.c` | 修复 | fopen → cbm_fopen |
| `src/discover/gitignore.c` | 修复 | fopen → cbm_fopen |
| `src/discover/language.c` | 修复 | fopen → cbm_fopen |
| `src/discover/userconfig.c` | 修复 | fopen → cbm_fopen |
| `src/mcp/mcp.c` | 修复 | fopen → cbm_fopen |
| `src/cli/cli.c` | 修复 | fopen → cbm_fopen |
| `src/ui/config.c` | 修复 | fopen → cbm_fopen |
| `src/ui/http_server.c` | 修复 | fopen → cbm_fopen |
| `src/foundation/diagnostics.c` | 修复 | fopen → cbm_fopen |
| `src/watcher/watcher.c` | 改进 | 多项 Windows 增强功能 |

总计：24 个文件被修改。
