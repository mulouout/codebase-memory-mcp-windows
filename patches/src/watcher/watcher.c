/*
 * watcher.c — Git-based file change watcher with Windows ReadDirectoryChangesW support.
 *
 * Strategy: git status + HEAD tracking (the most reliable approach).
 * For non-git projects, uses directory mtime polling or ReadDirectoryChangesW on Windows.
 *
 *
 * Per-project state tracks:
 *   - Last git HEAD hash (detects commits, checkout, pull)
 *   - Last poll time + adaptive interval
 *   - Whether the project is a git repo
 *   - Gitignore patterns for filtering
 *   - Windows: ReadDirectoryChangesW background thread
 *
 * Adaptive interval: 5s base + 1s per 500 files, capped at 60s.
 * Matches the Go watcher's `pollInterval()` logic.
 */
#include <stdint.h>
#include "watcher/watcher.h"
#include "store/store.h"
#include "foundation/constants.h"
#include "foundation/log.h"
#include "foundation/hash_table.h"
#include "foundation/compat.h"
#include "foundation/compat_thread.h"
#include "foundation/compat_fs.h"
#include "foundation/str_util.h"
#include "foundation/platform.h"
#include "discover/discover.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "foundation/win_utf8.h"
#endif

/* ── Per-project state ──────────────────────────────────────────── */

typedef struct project_state_t {
    char *project_name;
    char *root_path;
    char last_head[CBM_SZ_64]; /* git HEAD hash */
    bool is_git;               /* false → skip polling */
    bool baseline_done;        /* true after first poll */
    int file_count;            /* approximate, for interval calc */
    int interval_ms;           /* adaptive poll interval */
    int64_t next_poll_ns;      /* next poll time (monotonic ns) */
    int64_t last_dir_mtime;    /* last directory mtime for non-git polling */
    int last_file_count;       /* last file count for non-git polling */
    cbm_gitignore_t *gitignore; /* gitignore patterns for filtering */

#ifdef _WIN32
    HANDLE watch_dir_handle;
    OVERLAPPED watch_overlapped;
    char watch_buffer[65536];
    bool watch_active;
    HANDLE watch_thread_handle;
    CRITICAL_SECTION watch_cs;
    volatile bool pending_change;
#endif
} project_state_t;

/* ── Watcher struct ─────────────────────────────────────────────── */

struct cbm_watcher {
    cbm_store_t *store;
    cbm_index_fn index_fn;
    void *user_data;
    CBMHashTable *projects; /* name → project_state_t* */
    cbm_mutex_t projects_lock;
    atomic_int stopped;
};

/* ── Constants ─────────────────────────────────────────────────── */

/* Time unit conversions */
#define NS_PER_SEC 1000000000LL
#define US_PER_MS 1000000LL

/* Adaptive poll interval parameters (ms) */
#define POLL_BASE_MS 5000
#define POLL_FILE_STEP 500 /* add 1s per this many files */
#define POLL_MAX_MS 60000

/* Sleep chunk for responsive shutdown (ms) */
#define SLEEP_CHUNK_MS 500

/* ── Time helper ────────────────────────────────────────────────── */

static int64_t now_ns(void) {
    struct timespec ts;
    cbm_clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((int64_t)ts.tv_sec * NS_PER_SEC) + ts.tv_nsec;
}

/* ── Adaptive interval ──────────────────────────────────────────── */

int cbm_watcher_poll_interval_ms(int file_count) {
    int ms = POLL_BASE_MS + ((file_count / POLL_FILE_STEP) * CBM_MSEC_PER_SEC);
    if (ms > POLL_MAX_MS) {
        ms = POLL_MAX_MS;
    }
    return ms;
}

/* ── Git helpers ────────────────────────────────────────────────── */

/* Portable command pieces: cbm_popen runs through cmd.exe on Windows, which does
 * NOT strip single quotes (git would receive a literal-quoted path → "cannot find
 * the path") and has no /dev/null. Use double quotes (stripped by both cmd.exe and
 * POSIX sh) and the platform null device. */
#if defined(_WIN32)
#define WATCHER_NULDEV "NUL"
#else
#define WATCHER_NULDEV "/dev/null"
#endif

static bool is_git_repo(const char *root_path) {
    char cmd[CBM_SZ_1K];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" rev-parse --git-dir 2>%s", root_path, WATCHER_NULDEV);
    FILE *fp = cbm_popen(cmd, "r");
    if (!fp) {
        return false;
    }
    /* Drain output so pclose gets a clean exit status. */
    char drain[CBM_SZ_128];
    while (fgets(drain, (int)sizeof(drain), fp)) { /* discard */
    }
    int rc = cbm_pclose(fp);
    return rc == 0;
}

static int git_head(const char *root_path, char *out, size_t out_size) {
    char cmd[CBM_SZ_1K];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" rev-parse HEAD 2>%s", root_path, WATCHER_NULDEV);
    FILE *fp = cbm_popen(cmd, "r");
    if (!fp) {
        return CBM_NOT_FOUND;
    }

    if (fgets(out, (int)out_size, fp)) {
        size_t len = strlen(out);
        while (len > 0 && (out[len - SKIP_ONE] == '\n' || out[len - SKIP_ONE] == '\r')) {
            out[--len] = '\0';
        }
        cbm_pclose(fp);
        return 0;
    }
    cbm_pclose(fp);
    return CBM_NOT_FOUND;
}

/* Returns true if working tree has changes (modified, untracked, etc.).
 * Also checks submodules via `git submodule foreach` to detect uncommitted
 * changes inside submodules that `git status` alone would not report. */
static bool git_is_dirty(const char *root_path) {
    char cmd[CBM_SZ_1K];
    snprintf(cmd, sizeof(cmd),
             "git --no-optional-locks -C \"%s\" status --porcelain "
             "--untracked-files=normal 2>%s",
             root_path, WATCHER_NULDEV);
    FILE *fp = cbm_popen(cmd, "r");
    if (!fp) {
        return false;
    }

    char line[CBM_SZ_256];
    bool dirty = false;
    if (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - SKIP_ONE] == '\n' || line[len - SKIP_ONE] == '\r')) {
            line[--len] = '\0';
        }
        if (len > 0) {
            dirty = true;
        }
    }
    cbm_pclose(fp);

    if (dirty) {
        return true;
    }

#if !defined(_WIN32)
    /* Check submodules: uncommitted changes inside a submodule are invisible
     * to the parent's git status. Use `git submodule foreach` as a portable
     * fallback (Apple Git lacks --recurse-submodules). POSIX-only: foreach takes
     * an inner shell command that cmd.exe cannot pass intact; the parent-repo
     * status check above already covers the common (non-submodule) case. */
    snprintf(cmd, sizeof(cmd),
             "git --no-optional-locks -C '%s' submodule foreach --quiet --recursive "
             "'git status --porcelain --untracked-files=normal 2>/dev/null' "
             "2>/dev/null",
             root_path);
    fp = cbm_popen(cmd, "r");
    if (!fp) {
        return false;
    }
    if (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - SKIP_ONE] == '\n' || line[len - SKIP_ONE] == '\r')) {
            line[--len] = '\0';
        }
        if (len > 0) {
            dirty = true;
        }
    }
    cbm_pclose(fp);
#endif
    return dirty;
}

/* Count tracked files via git ls-files */
static int git_file_count(const char *root_path) {
    char cmd[CBM_SZ_1K];
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" ls-files 2>%s", root_path, WATCHER_NULDEV);
    FILE *fp = cbm_popen(cmd, "r");
    if (!fp) {
        return 0;
    }

    /* Count newlines (one tracked file per line). `wc -l` is unavailable on
     * Windows, so count in C, robust to paths longer than the read buffer. */
    int count = 0;
    char buf[CBM_SZ_1K];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                count++;
            }
        }
    }
    cbm_pclose(fp);
    return count;
}

/* ── Default skip directories ─────────────────────────────────────── */

static bool is_default_skip_dir(const char *dirname) {
    static const char *skip_dirs[] = {
        "node_modules", ".git", ".svn", ".hg", "build", "dist",
        "target", ".idea", ".vscode", ".cache", "vendor", "third_party",
        "__pycache__", ".next", ".nuxt", NULL
    };
    for (int i = 0; skip_dirs[i]; i++) {
        if (strcmp(dirname, skip_dirs[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* ── Extra ignore patterns from CBM_WATCHER_IGNORE ──────────────── */

typedef struct {
    char **patterns;
    int count;
    int capacity;
} extra_ignore_t;

static extra_ignore_t g_extra_ignore = {NULL, 0, 0};

static void init_extra_ignore(void) {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    char buf[CBM_SZ_1K];
    const char *env = cbm_safe_getenv("CBM_WATCHER_IGNORE", buf, sizeof(buf), NULL);
    if (!env || !env[0]) {
        return;
    }

    char *tmp = strdup(env);
    if (!tmp) {
        return;
    }

    int cap = 8;
    g_extra_ignore.patterns = malloc(cap * sizeof(char *));
    if (!g_extra_ignore.patterns) {
        free(tmp);
        return;
    }
    g_extra_ignore.capacity = cap;
    g_extra_ignore.count = 0;

    char *saveptr = NULL;
    char *tok = strtok_s(tmp, ";,", &saveptr);
    while (tok) {
        while (*tok == ' ') tok++;
        size_t len = strlen(tok);
        while (len > 0 && tok[len - 1] == ' ') tok[--len] = '\0';
        if (len > 0) {
            if (g_extra_ignore.count >= g_extra_ignore.capacity) {
                int new_cap = g_extra_ignore.capacity * 2;
                char **new_p = realloc(g_extra_ignore.patterns, new_cap * sizeof(char *));
                if (!new_p) {
                    break;
                }
                g_extra_ignore.patterns = new_p;
                g_extra_ignore.capacity = new_cap;
            }
            g_extra_ignore.patterns[g_extra_ignore.count++] = strdup(tok);
        }
        tok = strtok_s(NULL, ";,", &saveptr);
    }
    free(tmp);
}

static bool extra_ignore_match(const char *path) {
    init_extra_ignore();
    for (int i = 0; i < g_extra_ignore.count; i++) {
        const char *pat = g_extra_ignore.patterns[i];
        size_t pat_len = strlen(pat);
        size_t path_len = strlen(path);

        if (pat_len == 0) {
            continue;
        }

        if (pat[0] == '*' && pat[pat_len - 1] == '*') {
            const char *sub = pat + 1;
            size_t sub_len = pat_len - 2;
            if (sub_len == 0) {
                return true;
            }
            for (size_t j = 0; j + sub_len <= path_len; j++) {
                if (strncmp(path + j, sub, sub_len) == 0) {
                    return true;
                }
            }
        } else if (pat[0] == '*') {
            const char *suffix = pat + 1;
            size_t suffix_len = pat_len - 1;
            if (path_len >= suffix_len &&
                strcmp(path + path_len - suffix_len, suffix) == 0) {
                return true;
            }
        } else if (pat[pat_len - 1] == '*') {
            const char *prefix = pat;
            size_t prefix_len = pat_len - 1;
            if (path_len >= prefix_len &&
                strncmp(path, prefix, prefix_len) == 0) {
                return true;
            }
        } else {
            if (strcmp(path, pat) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool is_extra_ignored(const char *rel_path, bool is_dir) {
    (void)is_dir;
    if (extra_ignore_match(rel_path)) {
        return true;
    }
    const char *basename = strrchr(rel_path, '/');
    if (basename) {
        basename++;
    } else {
        basename = rel_path;
    }
    if (extra_ignore_match(basename)) {
        return true;
    }
    return false;
}

/* ── Recursive file counting with gitignore filtering ────────────── */

static int count_files_recursive_impl(const char *base_path, const char *rel_path,
                                      cbm_gitignore_t *gi) {
    char full_path[CBM_PATH_MAX];
    if (rel_path[0]) {
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, rel_path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s", base_path);
    }

    cbm_dir_t *d = cbm_opendir(full_path);
    if (!d) {
        return 0;
    }

    int count = 0;
    cbm_dirent_t *ent;
    while ((ent = cbm_readdir(d)) != NULL) {
        if (ent->is_dir && is_default_skip_dir(ent->name)) {
            continue;
        }

        char sub_rel[CBM_PATH_MAX];
        if (rel_path[0]) {
            snprintf(sub_rel, sizeof(sub_rel), "%s/%s", rel_path, ent->name);
        } else {
            snprintf(sub_rel, sizeof(sub_rel), "%s", ent->name);
        }

        if (gi && cbm_gitignore_matches(gi, sub_rel, ent->is_dir)) {
            continue;
        }

        if (is_extra_ignored(sub_rel, ent->is_dir)) {
            continue;
        }

        if (ent->is_dir) {
            count += count_files_recursive_impl(base_path, sub_rel, gi);
        } else {
            count++;
        }
    }

    cbm_closedir(d);
    return count;
}

/* ── Windows event filtering ─────────────────────────────────────── */

#ifdef _WIN32
static bool should_trigger_reindex_win(const char *filename) {
    size_t len = strlen(filename);
    if (len == 0) {
        return false;
    }

    const char *basename = strrchr(filename, '\\');
    if (!basename) {
        basename = strrchr(filename, '/');
    }
    if (basename) {
        basename++;
    } else {
        basename = filename;
    }
    size_t blen = strlen(basename);

    if (blen == 0) {
        return false;
    }

    if (basename[0] == '~' || basename[blen - 1] == '~') {
        return false;
    }

    if (blen >= 5 && strcmp(basename + blen - 5, ".tmp") == 0) {
        return false;
    }
    if (blen >= 4 && strcmp(basename + blen - 4, ".bak") == 0) {
        return false;
    }
    if (blen >= 5 && strcmp(basename + blen - 5, ".swp") == 0) {
        return false;
    }
    if (blen >= 5 && strcmp(basename + blen - 5, ".swx") == 0) {
        return false;
    }
    if (blen >= 2 && strcmp(basename + blen - 2, ".~") == 0) {
        return false;
    }

    if (blen >= 12 && strncmp(basename, ".gitignore.", 12) == 0) {
        return false;
    }

    return true;
}
#endif

/* ── Windows ReadDirectoryChangesW thread ────────────────────────── */

#ifdef _WIN32
static DWORD WINAPI watch_directory_thread(LPVOID lpParam) {
    project_state_t *s = (project_state_t *)lpParam;
    if (!s) {
        return 1;
    }

    char notify_buf[65536];
    DWORD bytes_returned = 0;

    while (s->watch_active) {
        BOOL ok = ReadDirectoryChangesW(
            s->watch_dir_handle,
            notify_buf,
            sizeof(notify_buf),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_CREATION,
            &bytes_returned,
            &s->watch_overlapped,
            NULL
        );

        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                GetOverlappedResult(s->watch_dir_handle, &s->watch_overlapped,
                                    &bytes_returned, TRUE);
            } else {
                break;
            }
        }

        if (!s->watch_active) {
            break;
        }

        if (bytes_returned == 0) {
            continue;
        }

        bool triggered = false;
        FILE_NOTIFY_INFORMATION *info = (FILE_NOTIFY_INFORMATION *)notify_buf;
        do {
            if (info->Action == FILE_ACTION_RENAMED_NEW_NAME ||
                info->Action == FILE_ACTION_RENAMED_OLD_NAME ||
                info->Action == FILE_ACTION_MODIFIED ||
                info->Action == FILE_ACTION_ADDED ||
                info->Action == FILE_ACTION_REMOVED) {

                char name_buf[CBM_PATH_MAX];
                int name_len = WideCharToMultiByte(CP_UTF8, 0,
                    info->FileName, info->FileNameLength / sizeof(WCHAR),
                    name_buf, sizeof(name_buf) - 1, NULL, NULL);
                if (name_len > 0) {
                    name_buf[name_len] = '\0';
                    if (should_trigger_reindex_win(name_buf)) {
                        triggered = true;
                        break;
                    }
                }
            }

            if (info->NextEntryOffset == 0) {
                break;
            }
            info = (FILE_NOTIFY_INFORMATION *)((char *)info + info->NextEntryOffset);
        } while (true);

        if (triggered) {
            EnterCriticalSection(&s->watch_cs);
            s->pending_change = true;
            LeaveCriticalSection(&s->watch_cs);
        }
    }

    return 0;
}

static bool start_watch_thread(project_state_t *s) {
    if (!s || s->watch_active) {
        return false;
    }

    wchar_t *wpath = cbm_utf8_to_wide(s->root_path);
    if (!wpath) {
        return false;
    }

    s->watch_dir_handle = CreateFileW(
        wpath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    free(wpath);

    if (s->watch_dir_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    InitializeCriticalSection(&s->watch_cs);
    memset(&s->watch_overlapped, 0, sizeof(s->watch_overlapped));
    s->pending_change = false;
    s->watch_active = true;

    s->watch_thread_handle = CreateThread(
        NULL, 0, watch_directory_thread, s, 0, NULL
    );

    if (!s->watch_thread_handle) {
        CloseHandle(s->watch_dir_handle);
        s->watch_dir_handle = INVALID_HANDLE_VALUE;
        DeleteCriticalSection(&s->watch_cs);
        s->watch_active = false;
        return false;
    }

    return true;
}

static void stop_watch_thread(project_state_t *s) {
    if (!s || !s->watch_active) {
        return;
    }

    s->watch_active = false;

    if (s->watch_dir_handle != INVALID_HANDLE_VALUE) {
        CancelIo(s->watch_dir_handle);
        CloseHandle(s->watch_dir_handle);
        s->watch_dir_handle = INVALID_HANDLE_VALUE;
    }

    if (s->watch_thread_handle) {
        WaitForSingleObject(s->watch_thread_handle, 5000);
        CloseHandle(s->watch_thread_handle);
        s->watch_thread_handle = NULL;
    }

    DeleteCriticalSection(&s->watch_cs);
}

static bool poll_watch_change(project_state_t *s) {
    if (!s || !s->watch_active) {
        return false;
    }

    bool changed = false;
    EnterCriticalSection(&s->watch_cs);
    if (s->pending_change) {
        changed = true;
        s->pending_change = false;
    }
    LeaveCriticalSection(&s->watch_cs);
    return changed;
}
#endif

/* ── Project state lifecycle ────────────────────────────────────── */

static project_state_t *state_new(const char *name, const char *root_path) {
    project_state_t *s = calloc(CBM_ALLOC_ONE, sizeof(*s));
    if (!s) {
        return NULL;
    }
    s->project_name = strdup(name);
    s->root_path = strdup(root_path);
    s->interval_ms = POLL_BASE_MS;
    s->last_dir_mtime = 0;
    s->last_file_count = 0;
    s->gitignore = NULL;
#ifdef _WIN32
    s->watch_dir_handle = INVALID_HANDLE_VALUE;
    s->watch_active = false;
    s->watch_thread_handle = NULL;
    s->pending_change = false;
    memset(&s->watch_overlapped, 0, sizeof(s->watch_overlapped));
#endif
    return s;
}

static void state_free(project_state_t *s) {
    if (!s) {
        return;
    }
#ifdef _WIN32
    stop_watch_thread(s);
#endif
    if (s->gitignore) {
        cbm_gitignore_free(s->gitignore);
    }
    free(s->project_name);
    free(s->root_path);
    free(s);
}

/* Hash table foreach callback to free state entries */
static void free_state_entry(const char *key, void *val, void *ud) {
    (void)key;
    (void)ud;
    state_free(val);
}

/* ── Watcher lifecycle ──────────────────────────────────────────── */

cbm_watcher_t *cbm_watcher_new(cbm_store_t *store, cbm_index_fn index_fn, void *user_data) {
    cbm_watcher_t *w = calloc(CBM_ALLOC_ONE, sizeof(*w));
    if (!w) {
        return NULL;
    }
    w->store = store;
    w->index_fn = index_fn;
    w->user_data = user_data;
    w->projects = cbm_ht_create(CBM_SZ_32);
    if (!w->projects) {
        free(w);
        return NULL;
    }
    cbm_mutex_init(&w->projects_lock);
    atomic_init(&w->stopped, 0);
    return w;
}

void cbm_watcher_free(cbm_watcher_t *w) {
    if (!w) {
        return;
    }
    cbm_mutex_lock(&w->projects_lock);
    cbm_ht_foreach(w->projects, free_state_entry, NULL);
    cbm_ht_free(w->projects);
    cbm_mutex_unlock(&w->projects_lock);
    cbm_mutex_destroy(&w->projects_lock);
    free(w);
}

/* ── Watch list management ──────────────────────────────────────── */

void cbm_watcher_watch(cbm_watcher_t *w, const char *project_name, const char *root_path) {
    if (!w || !project_name || !root_path) {
        return;
    }

    /* Reject paths with shell metacharacters — all git helpers use popen/system */
    if (!cbm_validate_shell_arg(root_path)) {
        cbm_log_warn("watcher.watch.reject", "project", project_name, "reason",
                     "path contains shell metacharacters");
        return;
    }

    /* Remove old entry first (key points to state's project_name) */
    cbm_mutex_lock(&w->projects_lock);
    project_state_t *old = cbm_ht_get(w->projects, project_name);
    if (old) {
        cbm_ht_delete(w->projects, project_name);
        state_free(old);
    }

    project_state_t *s = state_new(project_name, root_path);
    if (!s) {
        cbm_mutex_unlock(&w->projects_lock);
        cbm_log_warn("watcher.watch.oom", "project", project_name, "path", root_path);
        return;
    }
    cbm_ht_set(w->projects, s->project_name, s);
    cbm_mutex_unlock(&w->projects_lock);
    cbm_log_info("watcher.watch", "project", project_name, "path", root_path);
}

void cbm_watcher_unwatch(cbm_watcher_t *w, const char *project_name) {
    if (!w || !project_name) {
        return;
    }
    bool removed = false;
    cbm_mutex_lock(&w->projects_lock);
    project_state_t *s = cbm_ht_get(w->projects, project_name);
    if (s) {
        cbm_ht_delete(w->projects, project_name);
        state_free(s);
        removed = true;
    }
    cbm_mutex_unlock(&w->projects_lock);
    if (removed) {
        cbm_log_info("watcher.unwatch", "project", project_name);
    }
}

void cbm_watcher_touch(cbm_watcher_t *w, const char *project_name) {
    if (!w || !project_name) {
        return;
    }
    cbm_mutex_lock(&w->projects_lock);
    project_state_t *s = cbm_ht_get(w->projects, project_name);
    if (s) {
        /* Reset backoff — poll immediately on next cycle */
        s->next_poll_ns = 0;
    }
    cbm_mutex_unlock(&w->projects_lock);
}

int cbm_watcher_watch_count(cbm_watcher_t *w) {
    if (!w) {
        return 0;
    }
    cbm_mutex_lock(&w->projects_lock);
    int count = (int)cbm_ht_count(w->projects);
    cbm_mutex_unlock(&w->projects_lock);
    return count;
}

/* ── Single poll cycle ──────────────────────────────────────────── */

/* Init baseline for a project: check if git, get HEAD, count files */
static void init_baseline(project_state_t *s) {
    struct stat st;
    if (stat(s->root_path, &st) != 0) {
        cbm_log_warn("watcher.root_gone", "project", s->project_name, "path", s->root_path);
        s->baseline_done = true;
        s->is_git = false;
        return;
    }

    s->is_git = is_git_repo(s->root_path);
    s->baseline_done = true;

    char gi_path[CBM_PATH_MAX];
    snprintf(gi_path, sizeof(gi_path), "%s/.gitignore", s->root_path);
    s->gitignore = cbm_gitignore_load(gi_path);

    if (s->is_git) {
        git_head(s->root_path, s->last_head, sizeof(s->last_head));
        s->file_count = git_file_count(s->root_path);
        s->interval_ms = cbm_watcher_poll_interval_ms(s->file_count);
        s->last_file_count = s->file_count;
        cbm_log_info("watcher.baseline", "project", s->project_name, "strategy", "git", "files",
                     s->file_count > 0 ? "yes" : "0");
    } else {
        s->file_count = count_files_recursive_impl(s->root_path, "", s->gitignore);
        s->last_file_count = s->file_count;
        s->interval_ms = cbm_watcher_poll_interval_ms(s->file_count);
        s->last_dir_mtime = cbm_file_size(s->root_path);
        cbm_log_info("watcher.baseline", "project", s->project_name, "strategy", "dirscan",
                     "files", s->file_count > 0 ? "yes" : "0");

#ifdef _WIN32
        if (!start_watch_thread(s)) {
            cbm_log_warn("watcher.win32.fs_watch_fail", "project", s->project_name);
        } else {
            cbm_log_info("watcher.win32.fs_watch_start", "project", s->project_name);
        }
#endif
    }

    s->next_poll_ns = now_ns() + ((int64_t)s->interval_ms * US_PER_MS);
}

/* Check if a project has changes. Returns true if reindex needed. */
static bool check_changes(project_state_t *s) {
    if (s->is_git) {
        /* Check HEAD movement */
        char head[CBM_SZ_64] = {0};
        if (git_head(s->root_path, head, sizeof(head)) == 0) {
            if (s->last_head[0] != '\0' && strcmp(head, s->last_head) != 0) {
                /* HEAD moved — commit, checkout, pull */
                strncpy(s->last_head, head, sizeof(s->last_head) - 1);
                return true;
            }
            strncpy(s->last_head, head, sizeof(s->last_head) - 1);
        }

        /* Check working tree */
        return git_is_dirty(s->root_path);
    }

    /* Non-git project: check directory mtime + file count */
#ifdef _WIN32
    if (s->watch_active) {
        if (poll_watch_change(s)) {
            return true;
        }
    }
#endif

    int64_t current_mtime = cbm_file_size(s->root_path);
    if (current_mtime != CBM_NOT_FOUND && current_mtime != s->last_dir_mtime) {
        s->last_dir_mtime = current_mtime;
        int current_count = count_files_recursive_impl(s->root_path, "", s->gitignore);
        if (current_count != s->last_file_count) {
            s->last_file_count = current_count;
            s->file_count = current_count;
            return true;
        }
    }

    return false;
}

/* Context for poll_once foreach callback */
typedef struct {
    cbm_watcher_t *w;
    int64_t now;
    int reindexed;
} poll_ctx_t;

static void poll_project(const char *key, void *val, void *ud) {
    (void)key;
    poll_ctx_t *ctx = ud;
    project_state_t *s = val;
    if (!s) {
        return;
    }

    /* Initialize baseline on first poll */
    if (!s->baseline_done) {
        init_baseline(s);
        return;
    }

    /* Respect adaptive interval */
    if (ctx->now < s->next_poll_ns) {
        return;
    }

    /* Check for changes */
    bool changed = check_changes(s);
    if (!changed) {
        s->next_poll_ns = ctx->now + ((int64_t)s->interval_ms * US_PER_MS);
        return;
    }

    /* Trigger reindex */
    cbm_log_info("watcher.changed", "project", s->project_name,
                 "strategy", s->is_git ? "git" : "dirscan");
    if (ctx->w->index_fn) {
        int rc = ctx->w->index_fn(s->project_name, s->root_path, ctx->w->user_data);
        if (rc == 0) {
            ctx->reindexed++;
            if (s->is_git) {
                /* Update HEAD after successful reindex */
                git_head(s->root_path, s->last_head, sizeof(s->last_head));
                /* Refresh file count for interval */
                s->file_count = git_file_count(s->root_path);
            } else {
                /* Refresh file count for interval */
                s->file_count = count_files_recursive_impl(s->root_path, "", s->gitignore);
                s->last_file_count = s->file_count;
                s->last_dir_mtime = cbm_file_size(s->root_path);
            }
            s->interval_ms = cbm_watcher_poll_interval_ms(s->file_count);
        } else {
            cbm_log_warn("watcher.index.err", "project", s->project_name);
        }
    }

    s->next_poll_ns = ctx->now + ((int64_t)s->interval_ms * US_PER_MS);
}

/* Callback to snapshot project state pointers into an array. */
typedef struct {
    project_state_t **items;
    int count;
    int cap;
} snapshot_ctx_t;

static void snapshot_project(const char *key, void *val, void *ud) {
    (void)key;
    snapshot_ctx_t *sc = ud;
    if (val && sc->count < sc->cap) {
        sc->items[sc->count++] = val;
    }
}

int cbm_watcher_poll_once(cbm_watcher_t *w) {
    if (!w) {
        return 0;
    }

    /* Snapshot project pointers under lock, then poll without holding it.
     * This keeps the critical section small — poll_project does git I/O
     * and may invoke index_fn which runs the full pipeline. */
    cbm_mutex_lock(&w->projects_lock);
    int n = cbm_ht_count(w->projects);
    if (n == 0) {
        cbm_mutex_unlock(&w->projects_lock);
        return 0;
    }
    project_state_t **snap = malloc(n * sizeof(project_state_t *));
    if (!snap) {
        cbm_mutex_unlock(&w->projects_lock);
        return 0;
    }
    snapshot_ctx_t sc = {.items = snap, .count = 0, .cap = n};
    cbm_ht_foreach(w->projects, snapshot_project, &sc);
    cbm_mutex_unlock(&w->projects_lock);

    poll_ctx_t ctx = {
        .w = w,
        .now = now_ns(),
        .reindexed = 0,
    };
    for (int i = 0; i < sc.count; i++) {
        poll_project(NULL, snap[i], &ctx);
    }
    free(snap);
    return ctx.reindexed;
}

/* ── Blocking run loop ──────────────────────────────────────────── */

void cbm_watcher_stop(cbm_watcher_t *w) {
    if (w) {
        atomic_store(&w->stopped, 1);
    }
}

int cbm_watcher_run(cbm_watcher_t *w, int base_interval_ms) {
    if (!w) {
        return CBM_NOT_FOUND;
    }
    if (base_interval_ms <= 0) {
        base_interval_ms = POLL_BASE_MS;
    }

    cbm_log_info("watcher.start", "interval_ms", base_interval_ms > 999 ? "multi-sec" : "fast");

    while (!atomic_load(&w->stopped)) {
        cbm_watcher_poll_once(w);

        /* Sleep in small increments to allow responsive shutdown */
        int slept = 0;
        while (slept < base_interval_ms && !atomic_load(&w->stopped)) {
            int chunk = base_interval_ms - slept;
            if (chunk > SLEEP_CHUNK_MS) {
                chunk = SLEEP_CHUNK_MS;
            }
            cbm_usleep((unsigned)chunk * CBM_MSEC_PER_SEC);
            slept += chunk;
        }
    }

    cbm_log_info("watcher.stop");
    return 0;
}
