# Version Synchronization - Quick Start

## Problem Solved ✅

Previously, version numbers had to be updated manually in multiple places:
- `CMakeLists.txt` (project VERSION)
- `juce_add_plugin` configuration
- Source code version strings
- Version manifest files

**Now**: Use automatic version synchronization!

## Three Ways to Update Versions

### 1. Quick Interactive Sync (Recommended)

```bash
./release-sync-version.sh
```

Menu options:
1. Show current versions
2. Update (Major/Minor/Patch)
3. Set custom version
4. List files with versions

### 2. During Release

```bash
# Traditional release version bump
./release-quick.sh version

# Synced version bump
./release-sync-version.sh
```

### 3. Manual (for advanced users)

```bash
# Edit manually
nano Limone/CMakeLists.txt
nano .version
grep -r "VERSION\|version" Limone/Source
```

## Version Locations

After sync, version is updated in:

✅ `CMakeLists.txt` (project VERSION)
✅ `CMakeLists.txt` (juce_add_plugin VERSION)
✅ `.version` file (manifest)
✅ Build artifacts include version
✅ Release packages named: `Lim.one-X.Y.Z.tar.gz`

## Example Workflow

```bash
# 1. Initialize (first time only)
./release-init.sh
# Enter: 1.0.0

# 2. Work on features...
# (make changes, commit)

# 3. Ready to release?
./release-sync-version.sh

# Select: 2) Update version
# Choose: 3 (for patch version)
# Confirm: y

# Now version is 1.0.1 everywhere!

# 4. Build & package
./release-quick.sh build
./release-quick.sh package
```

## What Gets Updated

### CMakeLists.txt - Before
```cmake
project(LimOne)

juce_add_plugin(LimOne
    COMPANY_NAME "LuminousLab"
    ...
```

### CMakeLists.txt - After
```cmake
project(LimOne VERSION 1.0.1)

juce_add_plugin(LimOne
    VERSION 1.0.1
    COMPANY_NAME "LuminousLab"
    ...
```

### .version File
```
1.0.1
```

## Version Format

Always use: `X.Y.Z`

Examples:
- `1.0.0` - Release
- `0.1.5` - Alpha/Beta
- `2.0.0` - Major update

## Verification

Check all versions match:

```bash
# View current versions
./release-sync-version.sh
# Select: 1) Show current versions

# Expected output:
# CMakeLists.txt:   1.0.1
# .version file:    1.0.1
```

## Key Files

| File | Purpose |
|------|---------|
| `release-sync-version.sh` | Interactive version sync tool |
| `.version` | Version manifest (single source of truth) |
| `Limone/CMakeLists.txt` | Build config with version |
| `VERSION-SYNC.md` | Full documentation |

## Common Tasks

### Update for Bug Fix
```bash
./release-sync-version.sh  # Select 2, then 3
```

### Update for New Features
```bash
./release-sync-version.sh  # Select 2, then 2
```

### Update for Major Release
```bash
./release-sync-version.sh  # Select 2, then 1
```

### Set Exact Version
```bash
./release-sync-version.sh  # Select 3, enter 2.1.0
```

## Troubleshooting

**Version not updated?**
```bash
./release-init.sh  # Reinitialize
./release-sync-version.sh  # Show versions
```

**File permission error?**
```bash
chmod +x *.sh
```

**Need to rollback?**
```bash
git diff Limone/CMakeLists.txt  # See changes
git restore Limone/CMakeLists.txt  # Undo
```

## For More Information

Full documentation:
```bash
cat VERSION-SYNC.md       # Complete guide
cat RELEASE.md            # Release workflow
cat RELEASE-CHEATSHEET.md # Quick reference
```

---

**Summary**: Use `./release-sync-version.sh` to automatically update version numbers across your entire Lim.one project!
