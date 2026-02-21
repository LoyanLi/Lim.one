# Version Synchronization Guide

This guide explains how to keep version numbers synchronized across your entire Lim.one plugin project.

## Overview

Version numbers appear in multiple places in a JUCE plugin project:

1. **CMakeLists.txt** - Main build configuration
   - `project(LimOne VERSION X.Y.Z)`
   - `juce_add_plugin(LimOne VERSION X.Y.Z ...)`

2. **Source Code** - Hardcoded version strings
   - Version constants
   - About dialogs
   - Plugin metadata

3. **.version file** - Version manifest
   - Single source of truth
   - Used by build tools

## Quick Start

### View Current Versions

```bash
./release-sync-version.sh
# Select: 1) Show current versions
```

### Update Version

```bash
./release-sync-version.sh
# Select: 2) Update version (Major/Minor/Patch)
# Choose: 1 (Major), 2 (Minor), or 3 (Patch)
```

### Set Custom Version

```bash
./release-sync-version.sh
# Select: 3) Set custom version
# Enter: 1.2.3
```

## Version Locations

### CMakeLists.txt

**Before:**
```cmake
project(LimOne)

juce_add_plugin(LimOne
    COMPANY_NAME "LuminousLab"
    ...
    PRODUCT_NAME "Lim.one"
)
```

**After:**
```cmake
project(LimOne VERSION 1.0.0)

juce_add_plugin(LimOne
    VERSION 1.0.0
    COMPANY_NAME "LuminousLab"
    ...
    PRODUCT_NAME "Lim.one"
)
```

### Source Code Examples

**PluginProcessor.h**
```cpp
#define LIMONE_VERSION "1.0.0"
```

**PluginEditor.cpp**
```cpp
info.getDynamicObject()->setProperty("version", JucePlugin_VersionString);
```

## Version Numbering (Semantic Versioning)

Use standard semantic versioning:

```
MAJOR.MINOR.PATCH

Examples:
- 0.0.1  → First alpha/beta release
- 1.0.0  → First stable release
- 1.1.0  → New features added (backward compatible)
- 1.1.1  → Bug fix
- 2.0.0  → Breaking changes
```

### When to Bump Each Number

- **MAJOR**: Breaking changes to plugin API or behavior
  - Example: Change parameter range, remove features

- **MINOR**: New features added (backward compatible)
  - Example: New DSP mode, new UI elements

- **PATCH**: Bug fixes and improvements
  - Example: Fix audio glitches, improve performance

## Typical Release Workflow

### 1. Development Phase

Work on features and bug fixes normally. Version stays the same.

### 2. Release Preparation

When ready to release:

```bash
# 1. Check what's changed
./release-quick.sh status

# 2. Commit your changes
./release-quick.sh commit "feat: add new feature"

# 3. Update version
./release-sync-version.sh
# Or use traditional method:
./release-quick.sh version
```

### 3. Build & Package

```bash
# 4. Build
./release-quick.sh build

# 5. Package
./release-quick.sh package
```

## Verifying Versions

### Check All Versions

```bash
./release-sync-version.sh
# Select: 1) Show current versions
```

### Find Version Strings in Code

```bash
./release-sync-version.sh
# Select: 4) List files with version strings
```

Or manually:

```bash
grep -r "VERSION\|version" Limone/Source --include="*.h" --include="*.cpp"
```

## Advanced: Manual Version Updates

If you need fine-grained control, update files manually:

### Update CMakeLists.txt

Edit `Limone/CMakeLists.txt`:

```cmake
# Line 3
project(LimOne VERSION 1.2.3)

# Line 22 (in juce_add_plugin)
juce_add_plugin(LimOne
    VERSION 1.2.3
    ...
)
```

### Update Source Code

Search for version defines and update them:

```bash
grep -r "VERSION\|version.*=" Limone/Source
```

Update each location:

```cpp
// Example
#define LIMONE_VERSION "1.2.3"
const char* VERSION = "1.2.3";
```

### Update .version File

```bash
echo "1.2.3" > .version
```

## Continuous Integration

Version numbers in releases are automatically included in:

- Build artifacts
- Package names: `Lim.one-1.2.3.tar.gz`
- Manifest files
- Git tags (if using automation)

## Troubleshooting

### Version Not Updating

Check:
1. File permissions: `ls -l CMakeLists.txt`
2. Syntax: Ensure proper CMake syntax
3. Location: Confirm you're editing `Limone/CMakeLists.txt` not root

### Version String Not Found

The script searches for version strings in source files. If your version string is in:
- Compiled binaries (won't be found)
- Generated files
- External configuration

You need to update it manually.

### Mismatched Versions

If different files show different versions:

```bash
# Check all versions
./release-sync-version.sh  # Select: 1

# Fix manually
./release-sync-version.sh  # Select: 3) Set custom version
```

## Best Practices

1. **Update Together**: Always update CMakeLists.txt AND source code together
2. **Use Script**: Let the sync script handle updates when possible
3. **Commit with Version**: When committing version changes, use message:
   ```bash
   git commit -m "chore: bump version to 1.2.3"
   ```

4. **Test Build**: After version change, do a test build:
   ```bash
   ./release-quick.sh build
   ```

5. **Document Changes**: Update CHANGELOG or release notes with version

## Version Metadata

### .version File

The `.version` file is a simple text file with format:

```
1.2.3
```

Used by:
- Build scripts
- Package naming
- Version reporting

Create it manually:
```bash
echo "1.0.0" > .version
```

## See Also

- **RELEASE.md** - Full release guide
- **RELEASE-CHEATSHEET.md** - Quick commands
- **release.sh** - Interactive release menu
- **release-quick.sh** - Command-line shortcuts

---

**Last Updated**: February 2024
**Version**: 1.0.0
