# Givit — Improvement Report

This document describes 15 fixes applied to the Givit codebase, organized into three categories: spec compliance fixes, reliability improvements, and correctness fixes.

---

## 1. Spec Compliance Fixes

These features were required by the project specification but were missing or incomplete.

### 1.1 Checkout: Uncommitted Changes Guard
**File:** `src/branch.c`

**Before:** `givit checkout` silently overwrote all working files, even if the user had uncommitted modifications or staged files.

**After:** A new `has_uncommitted_changes()` helper reads `.givit/staging` and `.givit/tracks`, comparing each tracked file against the last commit's snapshot. If any changes exist, checkout is blocked with a clear error message:
```
error: you have uncommitted changes, please commit or stash them first
```

### 1.2 Commit: Empty Staging Area Check
**File:** `src/commit.c`

**Before:** `givit commit -m "msg"` would create an empty commit even when no files were staged, wasting a commit ID and snapshot directory.

**After:** Before creating a snapshot, the `.givit/staging` file is read. If it contains no entries, the commit is rejected:
```
nothing to commit, staging area is empty
```

### 1.3 Config Alias: Command Validation
**File:** `src/repository.c`

**Before:** `givit config alias.x "nonexistent"` silently accepted any string, including invalid commands.

**After:** A new `is_valid_command()` helper validates the first word of the alias value against the 19 valid givit commands. Invalid aliases are rejected with:
```
error: 'nonexistent' is not a valid givit command
```
This applies to both local (`config alias.x`) and global (`config --global alias.x`) alias creation.

---

## 2. Reliability Improvements

These changes replace fragile `system()` shell calls with direct internal function calls.

### 2.1 Revert: Eliminated Self-Invocation
**File:** `src/branch.c`

**Before:** `run_revert()` called `system("givit checkout ...")`, `system("givit add *")`, and `system("givit commit -m ...")`. This broke if givit wasn't installed system-wide, was vulnerable to shell injection, and spawned three child processes.

**After:** All three operations use internal functions directly:
- `remove_working_files()` + `restore_snapshot()` instead of checkout
- `add_file_or_dir()` loop instead of `add *`
- `commit_create_snapshot()` + `commit_save_list()` instead of commit

### 2.2 Checkout: Replaced `system("cp -r")`
**File:** `src/branch.c`

**Before:** Three places used `system("cp -r %s/* .")` to restore files from a commit snapshot.

**After:** A new `restore_snapshot()` function uses `opendir()` + `copy_file()` / `copy_dir()` to restore files safely using internal C functions.

### 2.3 Commit: Replaced `system()` for File Operations
**File:** `src/commit.c`

**Before:** After committing, `system("cp ...")` and `system("rm -rf ...")` were used to backup staging state and clear the staging area.

**After:** Uses `copy_file()`, `copy_dir()`, `remove_dir()`, and `ensure_dir()` from `utils.c`.

### 2.4 Pre-Commit Hooks: Integrated Into Commit Flow
**Files:** `src/commit.c`, `src/hooks.c`, `include/hooks.h`

**Before:** The user had to manually run `givit pre-commit` before every `givit commit`. If they forgot, invalid files could be committed.

**After:** A new `precommit_check_staged()` function is exposed from `hooks.c` and called automatically inside `run_commit()`. If any active hook fails on staged files, the commit is aborted:
```
pre-commit hooks failed, commit aborted
```

---

## 3. Correctness Fixes

These fix bugs where the implemented behavior was incorrect or incomplete.

### 3.1 Commit: Staging File Now Cleared After Commit
**File:** `src/commit.c`

**Before:** After committing, the `staged/` directory was cleared but `.givit/staging` (the file path list) was not truncated. This caused `is_staged()` to return stale results, making `status` show files as staged when they weren't.

**After:** `.givit/staging` is truncated (opened with `"w"` mode) after each commit.

### 3.2 Status: Deleted Files Now Detected
**File:** `src/staging.c`

**Before:** `run_status()` only iterated the working directory with `opendir(".")`. Tracked files that were deleted from disk never appeared in the output.

**After:** A second pass reads `.givit/tracks` and checks each path with `file_exists()`. Files that no longer exist are printed with the `D` (deleted) indicator:
```
└─── old.c -D
```

### 3.3 Checkout: Consistent Detached HEAD State
**File:** `src/branch.c`

**Before:** Only commit-ID checkout updated `.givit/detached`. Branch checkout and HEAD checkout left it in whatever state it was in, allowing commits from detached HEAD when switching via HEAD-N.

**After:**
- Branch checkout → writes `"1"` (attached)
- `HEAD` checkout (offset=0) → writes `"1"` (attached)
- `HEAD-N` checkout (offset>0) → writes `"0"` (detached)
- Commit-ID checkout → unchanged (already correct)

### 3.4 Tag Force: No More Duplicate Entries
**File:** `src/tag.c`

**Before:** `givit tag -a v1.0 -f` skipped the "already exists" error but appended a new entry without removing the old one, creating duplicate tag entries.

**After:** A new `remove_tag()` function filters the old entry out of `.givit/tags` before `create_tag()` appends the new one.

### 3.5 Log: Files Changed Count
**File:** `src/log.c`

**Before:** The log output did not include the number of files in each commit's snapshot, which was required by the spec.

**After:** A new `count_snapshot_files()` function recursively counts regular files in the snapshot directory. The count is printed after the snapshot path:
```
files: 5
```

### 3.6 Stash Show: Subdirectory Support
**File:** `src/stash.c`

**Before:** `stash show` had an explicit `continue; /* simplified: skip subdirs */` that ignored all subdirectories in the stash.

**After:** A new `show_stash_diff_recursive()` function recurses into subdirectories, showing diffs for nested files and marking new files/directories correctly.

### 3.7 Init: Correct Error Return Code
**File:** `src/repository.c`

**Before:** `givit init` in an existing repository printed an error but returned `0` (success), misleading scripts and automation.

**After:** Returns `1` (failure) when the repository already exists.

### 3.8 EOF Blank Space Hook: Checks All Lines
**File:** `src/hooks.c`

**Before:** The `eof_blank_space` hook only checked the last line of the file, and used a `while` loop that immediately returned (logically an `if`).

**After:** The hook now iterates every line of the file and checks for trailing whitespace (spaces or tabs before the newline). Fails on the first occurrence found.

---

## Summary

| # | Fix | Category | Files Changed |
|---|-----|----------|---------------|
| 1 | Checkout uncommitted changes guard | Spec compliance | `branch.c` |
| 2 | Commit empty staging check | Spec compliance | `commit.c` |
| 3 | Config alias validation | Spec compliance | `repository.c` |
| 4 | Revert: no more `system()` | Reliability | `branch.c` |
| 5 | Checkout: no more `system("cp")` | Reliability | `branch.c` |
| 6 | Commit: no more `system()` | Reliability | `commit.c` |
| 7 | Pre-commit integrated into commit | Reliability | `commit.c`, `hooks.c`, `hooks.h` |
| 8 | Commit clears staging file | Correctness | `commit.c` |
| 9 | Status detects deleted files | Correctness | `staging.c` |
| 10 | Checkout: consistent detached HEAD | Correctness | `branch.c` |
| 11 | Tag force removes old entry | Correctness | `tag.c` |
| 12 | Log shows file count | Correctness | `log.c` |
| 13 | Stash show recurses subdirs | Correctness | `stash.c` |
| 14 | Init returns error code 1 | Correctness | `repository.c` |
| 15 | EOF hook checks all lines | Correctness | `hooks.c` |
