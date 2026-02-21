# Release Scripts - Quick Reference

## 🔧 Setup (First Time)

```bash
./release-diagnose.sh    # Check all paths and dependencies
./release-init.sh        # Add version to CMakeLists.txt (if needed)
```

## 🚀 One-Liner Commands

### Check Status
```bash
./release-quick.sh status
```

### Commit & Push
```bash
./release-quick.sh commit "your message here"
```

### Update Version
```bash
./release-quick.sh version
```

### Build
```bash
./release-quick.sh build
```

### Package
```bash
./release-quick.sh package
```

### Full Release (All in one)
```bash
./release-quick.sh full
```

## 📋 Interactive Menu

```bash
./release.sh
```
Then select from menu (1-7)

## 🛠️ Make Commands

```bash
make -f Makefile.release help           # Show all commands
make -f Makefile.release status         # Git status
make -f Makefile.release quick-commit MSG="message"  # Commit
make -f Makefile.release version        # Update version
make -f Makefile.release build          # Build
make -f Makefile.release package        # Package
make -f Makefile.release full           # Full release
```

## 🪟 Windows (PowerShell)

```powershell
.\release.ps1                           # Interactive menu
.\release.ps1 -Command "status"         # Check status
.\release.ps1 -Command "commit"         # Commit (interactive)
.\release.ps1 -Command "version"        # Update version
.\release.ps1 -Command "build"          # Build
.\release.ps1 -Command "package"        # Package
.\release.ps1 -Command "full"           # Full release
```

## 📝 Typical Release Workflow

```bash
# 1. Check status
./release-quick.sh status

# 2. Commit changes
./release-quick.sh commit "feat: add new feature"

# 3. Update version
./release-quick.sh version
# Select: 1 (Major), 2 (Minor), 3 (Patch), or 4 (Custom)

# 4. Build
./release-quick.sh build

# 5. Create package
./release-quick.sh package

# Output: dist/Lim.one-X.Y.Z/
```

## ⚡ Quick Release (All Steps)

```bash
./release-quick.sh full
```

## 📁 Where Things Go

| What | Where |
|------|-------|
| Build output | `build_limone/` |
| Release packages | `dist/` |
| Version file | `CMakeLists.txt` |
| Config | `.releaserc.json` |

## 🎨 Output Colors

- 🟢 **Green**: Success
- 🔴 **Red**: Error
- 🟡 **Yellow**: Warning
- 🔵 **Blue**: Info

## ❓ Need Help?

```bash
./release-quick.sh help           # Show help
cat RELEASE.md                    # Full documentation
cat .releaserc.json              # Configuration
```

## 🔍 Before Release

```bash
git status                  # Check uncommitted changes
git log --oneline -5       # Review recent commits
./release-quick.sh status  # Confirm everything is clean
```

## ✅ After Release

Packages created in: `dist/Lim.one-X.Y.Z/`
- `Lim.one-X.Y.Z.tar.gz` (macOS/Linux)
- `Lim.one-X.Y.Z.zip` (Windows)

Check git:
```bash
git log --oneline -3       # Confirm new commits
git tag                    # If tags were created
```

---

**💡 Tip**: For most cases, just run `./release-quick.sh full` for complete release!
