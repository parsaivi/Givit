# Givit

A version control system implemented in C, inspired by Git. Givit tracks file changes, supports branching, merging, stashing, tagging, and pre-commit hooks — all from the Linux terminal.

```
   ▄██████▄   ▄█   ▄█    █▄   ▄█      ███
  ███    ███ ███  ███    ███ ███  ▀██████████▄
  ███    █▀  ███▌ ███    ███ ███▌    ▀███▀▀██
 ▄███        ███▌ ███    ███ ███▌     ███   ▀
▀▀███ ████▄  ███▌ ███    ███ ███▌     ███
  ███    ███ ███  ███    ███ ███      ███
  ███    ███ ███  ███▄  ▄███ ███      ███
  ████████▀  █▀    ▀██████▀  █▀      ▄████▀
```

---

## Table of Contents

- [Building & Installing](#building--installing)
- [Quick Start](#quick-start)
- [Commands](#commands)
  - [Repository](#repository)
  - [Staging](#staging)
  - [Committing](#committing)
  - [History & Inspection](#history--inspection)
  - [Branching & Navigation](#branching--navigation)
  - [Diff & Merge](#diff--merge)
  - [Tagging](#tagging)
  - [Stashing](#stashing)
  - [Pre-Commit Hooks](#pre-commit-hooks)
  - [Grep](#grep)
- [Project Architecture](#project-architecture)
- [Data Structures](#data-structures)
- [Internal Storage Layout](#internal-storage-layout)

---

## Building & Installing

**Requirements:** GCC, Make, Linux (Ubuntu)

```bash
# Build
make

# Install system-wide (adds 'givit' to PATH)
sudo make install

# Uninstall
sudo make uninstall

# Clean build artifacts
make clean
```

---

## Quick Start

```bash
# Initialize a new repository
givit init

# Configure identity
givit config user.name "Parsa"
givit config user.email "parsa@example.com"

# Stage and commit files
givit add main.c
givit commit -m "initial commit"

# Check status and history
givit status
givit log
```

---

## Commands

### Repository

#### `givit init`
Initializes a new Givit repository in the current directory. Creates the `.givit/` metadata directory with all required internal files and folders. Errors if a repository already exists in the current or any parent directory.

#### `givit config <key> <value>`
Sets a local repository configuration value.

```bash
givit config user.name "Alice"
givit config user.email "alice@example.com"
givit config alias.ci "commit -m"
```

#### `givit config --global <key> <value>`
Sets a global configuration value applied to all repositories.

```bash
givit config --global user.name "Alice"
givit config --global user.email "alice@example.com"
givit config --global alias.st "status"
```

Local settings override global ones.

---

### Staging

#### `givit add <file|directory>`
Stages files for the next commit.

```bash
givit add main.c               # Stage a single file
givit add src/                  # Stage all files in a directory
givit add -f file1.c file2.c   # Stage multiple files
givit add *.c                   # Wildcard support
givit add -redo                 # Re-stage previously staged files
givit add -n 3                  # Show file tree up to depth 3 with staged status
```

The `-n` flag displays a tree where staged files appear in white and unstaged files in red.

#### `givit reset <file|directory>`
Unstages files from the staging area.

```bash
givit reset main.c              # Unstage a file
givit reset src/                # Unstage a directory
givit reset -f file1.c file2.c  # Unstage multiple files
givit reset -undo               # Undo the last add operation (up to 10 steps)
```

#### `givit status`
Displays the state of files in the working directory.

```
└─── main.c +M
└─── new.txt -A
└─── old.c +D
```

- **First column:** `+` = staged, `-` = not staged
- **Second column:** `M` = modified, `A` = added (new), `D` = deleted

---

### Committing

#### `givit commit -m "<message>"`
Creates a commit from all staged files. Maximum message length: 72 characters.

```bash
givit commit -m "add login feature"
givit commit -s bugfix             # Use a message shortcut
```

#### Commit Message Shortcuts

```bash
givit set -m "fix typo" -s typo         # Create shortcut
givit replace -m "fix bug" -s typo      # Update shortcut
givit remove -s typo                     # Delete shortcut
givit commit -s typo                     # Commit using shortcut
```

---

### History & Inspection

#### `givit log`
Displays commit history from newest to oldest.

```
><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<
commit id: 3
message: add login feature
user: Alice
email: alice@example.com
branch: master
parent branch: master
time: 2026-02-22 14:30:00
snapshot: .givit/commits/3
><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<><<<<<
```

**Filters:**

```bash
givit log -n 5                  # Last 5 commits
givit log -branch dev           # Commits on 'dev' branch
givit log -author Alice         # Commits by author
givit log -since 2026-01-01     # Commits after date
givit log -before 2026-12-31    # Commits before date
givit log -search login auth    # Commits with matching keywords
```

#### `givit grep -f <file> -p <pattern> [-c <commit-id>] [-n]`
Searches for a text pattern in files.

```bash
givit grep -f main.c -p "TODO"         # Search in working file
givit grep -f main.c -p "TODO" -n      # With line numbers
givit grep -f main.c -p "TODO" -c 3    # Search in a specific commit
```

---

### Branching & Navigation

#### `givit branch [<name>]`

```bash
givit branch              # List all branches
givit branch feature      # Create a new branch from current HEAD
```

#### `givit checkout <target>`

```bash
givit checkout dev        # Switch to branch
givit checkout 3          # Switch to commit by ID
givit checkout HEAD       # Switch to latest commit
givit checkout HEAD-2     # Go back 2 commits from HEAD
```

When checking out a past commit (not the latest on a branch), Givit enters a **detached HEAD** state where new commits are not allowed until you return to a branch head.

#### `givit revert <target>`

```bash
givit revert 3                    # Revert to commit 3 and create new commit
givit revert HEAD-1               # Revert to previous commit
givit revert -m "undo" 3          # Revert with custom message
givit revert -n 3                 # Revert working directory only (no commit)
```

---

### Diff & Merge

#### `givit diff`

```bash
# Compare two files with optional line ranges
givit diff -f file1.c file2.c
givit diff -f file1.c file2.c --line1 1-50 --line2 1-50

# Compare two commits
givit diff -c 1 2
```

Differences are displayed with color coding — yellow for the first source, white for the second.

#### `givit merge -b <branch1> <branch2>`
Merges two branches. When conflicts arise, Givit presents each conflicting line interactively:

```
feature-12:
    int x = 10;
main-12:
    int x = 20;
```

Enter `1` to keep the first version, `2` for the second, `quit` to abort, or type a custom replacement line.

---

### Tagging

#### `givit tag`

```bash
givit tag                                    # List all tags (alphabetical)
givit tag -a v1.0                            # Tag current commit
givit tag -a v1.0 -m "first release"         # Tag with message
givit tag -a v1.0 -m "release" -c 3          # Tag specific commit
givit tag -a v1.0 -f                         # Overwrite existing tag
givit tag show v1.0                          # Show tag details
```

---

### Stashing

Temporarily saves uncommitted changes on a stack.

```bash
givit stash push                    # Stash current changes
givit stash push -m "wip auth"     # Stash with message
givit stash list                    # List all stashes
givit stash show 0                  # Show diff for stash@{0}
givit stash pop                     # Apply and remove top stash
givit stash drop 0                  # Remove specific stash
givit stash clear                   # Remove all stashes
```

Output format:
```
stash@{0}: On master: wip auth
stash@{1}: On master: debugging
```

---

### Pre-Commit Hooks

Validation hooks that run on staged files before committing.

```bash
givit pre-commit                           # Run active hooks on staged files
givit pre-commit hooks list                # List all available hooks
givit pre-commit applied hooks             # List currently active hooks
givit pre-commit add hook todo-check       # Activate a hook
givit pre-commit remove hook todo-check    # Deactivate a hook
givit pre-commit -f main.c utils.c         # Run hooks on specific files
```

**Available Hooks:**

| Hook | Description |
|------|-------------|
| `todo-check` | Ensures `//TODO` exists in `.c`/`.cpp` files; forbids `TODO` in `.txt` |
| `eof-blank-space` | Checks for trailing whitespace at end of file |
| `format-check` | Validates file has a recognized extension (`.c`, `.cpp`, `.txt`, `.mp3`, `.mp4`, `.mkv`) |
| `balance-braces` | Verifies balanced `{` `}` pairs in code files |
| `file-size-check` | Rejects files larger than 5 MB |
| `character-limit` | Rejects code files with more than 20,000 characters |
| `time-limit` | Rejects audio/video files exceeding 10 minutes |
| `static-error-check` | Checks for compilation errors in `.c`/`.cpp` files |

Output per file:
```
"main.c": "todo-check" ................PASSED
"main.c": "balance-braces" ..........FAILED
```

---

## Project Architecture

```
givit/
├── Makefile
├── README.md
├── include/
│   ├── givit.h          # Core types (Commit, StashEntry) and constants
│   ├── utils.h          # File/path utility functions
│   ├── repository.h     # init, config
│   ├── staging.h        # add, reset, status
│   ├── commit.h         # commit, snapshots, shortcuts
│   ├── log.h            # log with filters
│   ├── branch.h         # branch, checkout, revert
│   ├── diff.h           # diff, merge
│   ├── tag.h            # tag management
│   ├── hooks.h          # pre-commit hooks
│   ├── grep.h           # text search
│   └── stash.h          # stash operations
├── src/
│   ├── main.c           # Entry point and command dispatch
│   ├── utils.c          # File I/O, path helpers, directory ops
│   ├── repository.c     # Repository initialization and config
│   ├── staging.c        # Staging area management
│   ├── commit.c         # Commit creation, serialization, shortcuts
│   ├── log.c            # Commit history display and filtering
│   ├── branch.c         # Branch, checkout, revert logic
│   ├── diff.c           # File and commit comparison, merge
│   ├── tag.c            # Tag CRUD and sorting
│   ├── hooks.c          # Hook implementations and runner
│   ├── grep.c           # Pattern search in files
│   └── stash.c          # Stash stack operations
└── build/               # Compiled object files (generated)
```

The codebase is organized in two layers:

- **CLI Layer** (`main.c`): Parses `argv[1]` and dispatches to the appropriate handler. Also resolves aliases.
- **Logic Layer** (all other `.c` files): Each module implements one feature domain. Modules depend on `utils.c` for file operations and `commit.c` for commit data access, but never depend on each other circularly.

---

## Data Structures

### Commit

```c
typedef struct Commit {
    int id;                              // Auto-incrementing integer
    time_t timestamp;                    // Full UNIX timestamp
    int branch_parent_id;                // Previous commit on the same branch
    int merge_parent_id;                 // Merge parent (0 if not a merge commit)
    char message[MAX_MSG_LEN];           // Commit message
    char branch[MAX_NAME_LEN];           // Branch name
    char parent_branch[MAX_NAME_LEN];    // Branch this was created from
    char author_name[MAX_NAME_LEN];      // Author username
    char author_email[MAX_NAME_LEN];     // Author email
    char snapshot_path[MAX_PATH_LEN];    // Path to full file snapshot
    struct Commit *prev;                 // Linked list pointer (newer → older)
} Commit;
```

Commits are stored as a linked list serialized to `.givit/commitsdb`. Each commit's ID is an auto-incrementing integer managed by `.givit/state`. Full file snapshots are stored under `.givit/commits/<id>/`.

### StashEntry

```c
typedef struct StashEntry {
    int index;                           // Stack index
    char branch[MAX_NAME_LEN];           // Branch when stash was created
    char message[MAX_MSG_LEN];           // User-provided description
    char path[MAX_PATH_LEN];            // Path to stash snapshot
    time_t timestamp;                    // When the stash was created
} StashEntry;
```

Stashes are stored as directories under `.givit/stashes/` with metadata in `.givit/stash_list`.

---

## Internal Storage Layout

When `givit init` is run, the following structure is created:

```
.givit/
├── config              # username, email, current branch
├── state               # Next commit ID counter
├── detached            # "1" = on branch head, "0" = detached HEAD
├── commitsdb           # Serialized commit linked list
├── staging             # List of currently staged file paths
├── tracks              # List of all tracked file paths
├── havebeenstaged      # Files ever staged (for -redo)
├── branches            # Branch registry: "name parent\n"
├── aliases             # Command aliases: '"name" "command"\n'
├── shortcuts           # Message shortcuts: '"shortcut" "message"\n'
├── tags                # Tag entries (sorted alphabetically)
├── hooks               # Available hook IDs
├── applied_hooks       # Currently active hook IDs
├── stash_list          # Stash metadata entries
├── commits/            # Commit snapshots: commits/<id>/
├── staged/             # Current staging area (file copies)
├── stashes/            # Stash snapshots: stashes/<index>/
├── undo/               # Staging undo history (10 slots)
│   ├── 1/ ... 10/
├── files/              # File metadata
└── valid/              # Temporary workspace for diff operations
```

---

**Author:** Parsa Malekian
