# Git Hooks

This directory contains git hooks to maintain synchronization between `MacFlim Source Code.dsk` and the `macsrc/` directory.

## Installation

To enable the pre-commit hook, run this command from the repository root:

```bash
git config core.hooksPath hooks
```

This needs to be done once per clone.

## What the Hook Does

The pre-commit hook automatically extracts source files from `MacFlim Source Code.dsk` whenever you commit changes to the disk image. This ensures the `macsrc/` directory stays synchronized with the source of truth.

**Workflow with hook installed:**
1. Edit source code in Mini vMac emulator
2. Save changes to `MacFlim Source Code.dsk`
3. `git add "MacFlim Source Code.dsk"`
4. `git commit` → Hook automatically runs `macsrc/doit.sh` and stages extracted files

**Without the hook:**
You must manually run `macsrc/doit.sh` after editing the .dsk file, or the CI build will fail with a sync error.

## Why This Matters

The CI/CD pipeline validates that `macsrc/` matches the content of the disk image. If they're out of sync, the build fails. The hook prevents this by automating the extraction step.
