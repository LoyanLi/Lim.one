# Release & Deployment Guide

Interactive scripts for managing git operations, version updates, and packaging for **Lim.one** project.

## Overview

This project includes automated scripts to streamline the release workflow:

- **Git management**: Commit, push, and status checking
- **Version management**: Automatic version bumping (major/minor/patch)
- **Build automation**: CMake-based building
- **Packaging**: Create release packages with metadata

## Setup

### First Time Setup

If this is the first time using release scripts, run the diagnostic:

```bash
./release-diagnose.sh
```

If CMakeLists.txt doesn't have a VERSION, initialize it:

```bash
./release-init.sh
```

This will add VERSION to CMakeLists.txt (e.g., `project(LimOne VERSION 1.0.0)`).

## Quick Start

### 1. Interactive Menu (Recommended for most operations)

**macOS/Linux:**
```bash
./release.sh
```

**Windows (PowerShell):**
```powershell
.\release.ps1
# or with command:
.\release.ps1 -Command "status"
```

### 2. Quick Commands

**macOS/Linux:**
```bash
./release-quick.sh commit "Your commit message"
./release-quick.sh version
./release-quick.sh build
./release-quick.sh package
./release-quick.sh full
./release-quick.sh status
```

**Windows (PowerShell):**
```powershell
.\release.ps1 -Command "commit" -ArgumentList "Your message"
.\release.ps1 -Command "version"
.\release.ps1 -Command "build"
.\release.ps1 -Command "package"
.\release.ps1 -Command "full"
.\release.ps1 -Command "status"
```

### 3. Diagnostic & Initialization

**Check setup:**
```bash
./release-diagnose.sh
```

**Initialize version (first time only):**
```bash
./release-init.sh
```

### 4. Make Commands

```bash
# View all commands
make -f Makefile.release help

# Run individual commands
make -f Makefile.release status
make -f Makefile.release quick-commit MSG="Your message"
make -f Makefile.release build
make -f Makefile.release package
make -f Makefile.release full
```

## Features

### Git Operations

- **Status Check**: Display current branch and uncommitted changes
- **Commit & Push**: Create commits and push to remote
- **Branch Awareness**: Automatically detects current branch

```bash
./release-quick.sh status
./release-quick.sh commit "fix: audio processing issue"
```

### Version Management

Automatic version bumping with three options:

1. **Major**: `1.0.0` → `2.0.0` (breaking changes)
2. **Minor**: `1.0.0` → `1.1.0` (new features)
3. **Patch**: `1.0.0` → `1.0.1` (bug fixes)
4. **Custom**: Enter custom version manually

```bash
./release-quick.sh version
```

Version files updated:
- `CMakeLists.txt` (primary)
- `Limone/CMakeLists.txt` (if exists)
- Other version files (automatically detected)

### Build Project

Builds using CMake with automatic parallel compilation:

```bash
./release-quick.sh build
```

Defaults:
- Build directory: `build_limone/`
- Configuration: Release
- Parallel cores: Auto-detected

### Create Release Package

Generates distributable packages with:

- Compiled binaries
- Documentation
- README and LICENSE
- Manifest file with build info

```bash
./release-quick.sh package
```

Supported formats:
- `.tar.gz` (macOS/Linux)
- `.zip` (all platforms)

**Package structure:**
```
Lim.one-1.0.0/
├── builds/          (compiled binaries)
├── docs/            (documentation)
├── README.md
├── LICENSE
└── MANIFEST.txt     (build metadata)
```

### Full Release Pipeline

Runs all steps in sequence:
1. Git commit & push
2. Version update
3. Build project
4. Create release package

```bash
./release-quick.sh full
# or
./release.sh          # then select option 6
```

## Configuration

Release settings can be customized in `.releaserc.json`:

```json
{
  "project": {
    "name": "Lim.one",
    "description": "Plugin description"
  },
  "build": {
    "directory": "build_limone",
    "type": "cmake",
    "config": "Release",
    "parallel": true
  },
  "version": {
    "file": "CMakeLists.txt"
  },
  "git": {
    "remote": "origin",
    "main_branch": "main",
    "auto_push": false
  }
}
```

## Usage Examples

### Example 1: Simple patch release

```bash
./release-quick.sh commit "fix: memory leak in gain calculator"
./release-quick.sh version    # select Patch option
./release-quick.sh build
./release-quick.sh package
```

### Example 2: Full automated release

```bash
./release-quick.sh full
# Runs: commit → version → build → package
```

### Example 3: Manual steps with git control

```bash
./release.sh
# Then select operations interactively:
# 1) Check git status
# 2) Commit & push
# 3) Update version
# 4) Build
# 5) Package
# 6) Full release
```

### Example 4: Via Make

```bash
make -f Makefile.release quick-commit MSG="Release: v1.1.0"
make -f Makefile.release build
make -f Makefile.release package
```

## Output Locations

| Output | Location |
|--------|----------|
| Build artifacts | `build_limone/` |
| Release packages | `dist/` |
| Version config | `CMakeLists.txt` |
| Commit history | `.git/` |

## Troubleshooting

### "Not a git repository!"
Run scripts from the project root directory:
```bash
cd /path/to/Lim.one
./release.sh
```

### Build fails
Check CMake installation and dependencies:
```bash
cmake --version
cd build_limone && cmake ..
```

### Permission denied
Make scripts executable:
```bash
chmod +x release.sh release-quick.sh
```

### Version not updating
Ensure `CMakeLists.txt` uses standard format:
```cmake
project(Limone VERSION 1.0.0)
```

## Platform Support

| Feature | macOS | Linux | Windows |
|---------|-------|-------|---------|
| release.sh | ✓ | ✓ | - |
| release.ps1 | - | - | ✓ |
| release-quick.sh | ✓ | ✓ | - |
| Makefile.release | ✓ | ✓ | - |

## Safety Features

- Confirms before pushing to remote
- Detects uncommitted changes
- Validates version format (X.Y.Z)
- Creates manifest with build metadata
- Color-coded output for clarity

## Advanced

### Dry run (check what would happen)

```bash
git status              # see uncommitted changes
git log --oneline -5    # see recent commits
ls -la dist/            # see existing packages
```

### Manual version update

Edit `CMakeLists.txt` directly:
```cmake
project(Limone VERSION 1.2.0)
```

Then commit:
```bash
git add CMakeLists.txt
git commit -m "chore: bump version to 1.2.0"
git push
```

### Custom build configuration

Modify `release.sh` or `.releaserc.json`:
- Change `BUILD_DIR` for different build location
- Modify `CMAKE_FILE` if using different config file
- Adjust parallel cores in `build_project()`

## Contributing

To improve release scripts:

1. Test changes locally
2. Update `.releaserc.json` if changing configuration
3. Ensure cross-platform compatibility
4. Document new features in this guide

## Support

For issues:
1. Check error messages (color-coded output)
2. Run `git status` to check repository state
3. Review troubleshooting section
4. Check `.releaserc.json` configuration

---

**Last Updated**: February 2024
**Project**: Lim.one v1.0.0+
