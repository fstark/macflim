# MacFlim Source Code

This directory contains a copy of the source code present in the top-level `MacFlim Source Code.dsk` for ease of online browsing and diffing on GitHub. Does not contain the THINK C project files (.π), nor the resource files (.rsrc).

## Source of Truth

**The disk image `MacFlim Source Code.dsk` is the authoritative source.** All development should be done in the Mini vMac emulator by mounting and editing files on the disk image.

## Synchronization

The CI/CD pipeline enforces that this directory stays synchronized with the disk image. If they're out of sync, the build will fail.

**To keep them synchronized:**

1. **Recommended:** Install the git hook (see `hooks/README.md`):
   ```bash
   git config core.hooksPath hooks
   ```
   The hook automatically runs `./doit.sh` when you commit changes to the .dsk file.

2. **Manual:** Run `./doit.sh` after editing the disk image to extract files into this directory.

## Extraction Script

The `doit.sh` script uses `hfsutils` to extract source files from the disk image. It handles the filename conversion (spaces, special characters) to make files suitable for Unix filesystems.
