# Release Scripts - Fixes & Updates

## 🔧 Fixed Issues

### 1. ✅ CMake Configuration Error
**Problem**: "The source directory does not appear to contain CMakeLists.txt"

**Root Cause**:
- Project structure has CMakeLists.txt in `Limone/` subdirectory, not root
- Scripts were running cmake from wrong directory

**Solution**:
- Updated scripts to use modern CMake syntax: `cmake -S <source> -B <build>`
- source directory: `$REPO_ROOT/Limone`
- build directory: `$REPO_ROOT/build_limone`

**Files Updated**:
- `release.sh` - line ~11, ~180
- `release.ps1` - line ~7, ~119

### 2. ✅ Version Detection
**Problem**: "No VERSION found in CMakeLists.txt"

**Root Cause**:
- Original CMakeLists.txt doesn't have VERSION in project() command
- Version extraction regex was too strict

**Solution**:
- Made version detection more flexible
- Falls back to `1.0.0` if no version found
- First version update adds VERSION to project line

**File Updated**:
- `release.sh` - `get_current_version()` function

### 3. ✅ Version Update Function
**Problem**: sed regex wasn't matching project() without explicit VERSION

**Solution**:
- Updated to handle both existing and new VERSION entries
- Properly escapes sed delimiters
- Backs up file before modification

**File Updated**:
- `release.sh` - `update_version()` function

## 📦 New Scripts Added

### 1. `release-diagnose.sh` (NEW)
Checks the release system setup:
- ✓ Directory paths exist
- ✓ Git configuration
- ✓ CMake installation
- ✓ Version detection
- ✓ Available scripts

**Usage**:
```bash
./release-diagnose.sh
```

### 2. `release-init.sh` (NEW)
Initializes version in CMakeLists.txt for first use:
- Creates backup before modification
- Validates version format (X.Y.Z)
- Updates project() line with VERSION

**Usage**:
```bash
./release-init.sh
# Enter desired initial version (default: 1.0.0)
```

## 🛠️ Project Structure

```
/Users/loyan/Documents/Mixing/Limiter/
├── Limone/
│   ├── CMakeLists.txt      ← Main project file
│   └── Source/
├── build_limone/           ← Build artifacts
├── dist/                   ← Release packages
├── release.sh              ← Main interactive menu
├── release-quick.sh        ← Quick commands
├── release-diagnose.sh     ← Diagnostic tool
├── release-init.sh         ← Initialize version
├── release.ps1             ← Windows PowerShell
├── Makefile.release        ← Make targets
├── .releaserc.json         ← Configuration
├── RELEASE.md              ← Full documentation
├── RELEASE-CHEATSHEET.md   ← Quick reference
└── RELEASE-FIXES.md        ← This file
```

## 🚀 Typical Workflow (Updated)

### First Time Setup:
```bash
# 1. Check system setup
./release-diagnose.sh

# 2. If version not in CMakeLists.txt, initialize it
./release-init.sh
```

### Regular Release:
```bash
# 1. Check status
./release-quick.sh status

# 2. Commit changes
./release-quick.sh commit "feat: new feature"

# 3. Update version
./release-quick.sh version
# Select: Major (2.0.0), Minor (1.1.0), Patch (1.0.1), or Custom

# 4. Build
./release-quick.sh build

# 5. Package
./release-quick.sh package
```

### One-Command Release:
```bash
./release-quick.sh full
```

## 🧪 Test Results

✅ All scripts are executable
✅ Git commands work correctly
✅ CMake path is correct
✅ Version detection works
✅ Diagnostic tool passes

## 📊 Script Compatibility

| Feature | Bash (macOS/Linux) | PowerShell (Windows) |
|---------|-------------------|----------------------|
| Git ops | ✅ | ✅ |
| Version | ✅ (with fallback) | ✅ |
| Build | ✅ | ✅ |
| Package | ✅ | ✅ |
| Diagnose | ✅ | - |
| Init | ✅ | - |

## 🔍 Key Changes Summary

1. **CMake**: Now uses `-S <source> -B <build>` syntax
2. **Version**: Flexible detection with fallback to 1.0.0
3. **Paths**: Correctly references `Limone/` subdirectory
4. **Diagnostics**: New tool to verify setup
5. **Initialization**: New tool to add version to CMakeLists.txt

## ✨ Tips

- Always run `./release-diagnose.sh` if something doesn't work
- Use `./release-init.sh` once to set up version tracking
- The interactive menu (`./release.sh`) is safest for beginners
- PowerShell script works on Windows 10+ with CMake installed

---

**Last Updated**: February 21, 2024
**Status**: ✅ All fixes verified and tested
