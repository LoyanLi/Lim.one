# Lim.one Release Management System

简化、组织的发布管理系统。

## 📁 项目结构

```
/
├── release.sh              ⭐ 主交互式菜单（主要使用）
├── .version               📦 当前版本（自动读取）
├── scripts/               📁 辅助工具
│   ├── release-quick.sh         快速命令行
│   ├── release-diagnose.sh      系统诊断
│   ├── release-init.sh          版本初始化
│   ├── release-sync-version.sh  版本同步
│   └── release.ps1              Windows支持
├── RELEASE-README.md      📖 此文档
├── RELEASE.md             📖 完整文档
└── VERSION-SYNC.md        📖 版本同步指南
```

## 🚀 快速开始

### 主菜单（推荐）

```bash
./release.sh
```

**菜单选项：**
1. ✅ 检查 Git 状态
2. 📝 提交并推送
3. 📦 更新版本
4. 🔨 构建项目
5. 📦 创建发布包
6. 🚀 完整发布流程（2-5）
7. 📋 查看辅助脚本
8. 🚪 退出

### 快速命令

```bash
./scripts/release-quick.sh status           # 检查状态
./scripts/release-quick.sh commit "message" # 提交
./scripts/release-quick.sh version          # 更新版本
./scripts/release-quick.sh build            # 构建
./scripts/release-quick.sh package          # 打包
./scripts/release-quick.sh full             # 完整发布
```

## 🔧 辅助脚本

### 1. release-quick.sh
快速命令行接口，无需交互菜单

```bash
./scripts/release-quick.sh commit "feat: new feature"
```

### 2. release-diagnose.sh
检查系统设置和依赖关系

```bash
./scripts/release-diagnose.sh
```

### 3. release-init.sh
首次初始化版本

```bash
./scripts/release-init.sh
```

### 4. release-sync-version.sh
自动同步版本到各个文件

```bash
./scripts/release-sync-version.sh
```

### 5. release.ps1
Windows PowerShell 版本

```powershell
.\scripts\release.ps1
```

## 📦 版本管理

### 自动版本读取

脚本自动从 `.version` 文件读取当前版本：

```bash
$ cat .version
0.1.0
```

### 版本更新方式

1. **通过主菜单**
   ```bash
   ./release.sh    # 选择选项 3
   ```

2. **快速命令**
   ```bash
   ./scripts/release-quick.sh version
   ```

3. **版本同步工具**
   ```bash
   ./scripts/release-sync-version.sh
   ```

### 版本同步范围

更新版本会自动同步到：
- ✅ CMakeLists.txt (project VERSION)
- ✅ CMakeLists.txt (juce_add_plugin VERSION)
- ✅ .version 文件
- ✅ 发布包名称

## 💾 工作流示例

### 简单的补丁发布

```bash
# 1. 主菜单
./release.sh

# 2. 选择 2 (提交)
# 输入提交信息

# 3. 选择 3 (更新版本)
# 选择 3 (Patch)

# 4. 选择 4 (构建)
# 选择 5 (打包)
# 完成！
```

### 完整的一键发布

```bash
./scripts/release-quick.sh full
```

## 🛠️ 各操作说明

### Git 状态
检查当前分支和未提交的更改

```bash
./release.sh → 选项 1
```

### 提交和推送
```bash
./release.sh → 选项 2
# 输入提交信息，选择是否推送
```

### 版本更新
自动更新 Major/Minor/Patch 版本
```bash
./release.sh → 选项 3
# 选择版本类型或自定义
```

### 项目构建
CMake 配置和编译（使用 Release 模式）
```bash
./release.sh → 选项 4
```

### 创建发布包
生成包含构建产物、文档和清单的发布包
```bash
./release.sh → 选项 5
# 选择 tar.gz 和/或 zip 格式
```

### 完整发布流程
按顺序执行：提交 → 版本 → 构建 → 打包
```bash
./release.sh → 选项 6
```

## 📋 文件位置

### 输出位置

```
build_limone/       编译产物
dist/              发布包
  └─ Lim.one-X.Y.Z/
     ├─ builds/    编译文件
     ├─ docs/      文档
     ├─ README.md
     ├─ LICENSE
     └─ MANIFEST.txt
```

### 配置文件

```
.version            当前版本
Limone/CMakeLists.txt  版本配置
.releaserc.json     发布配置
Makefile.release    Make 接口
```

## ❓ 常见问题

**Q: 如何检查当前版本？**
```bash
cat .version
```

**Q: 版本号格式是什么？**
```
X.Y.Z
例如：1.0.0, 0.1.5, 2.0.0
```

**Q: 如何回滚版本更改？**
```bash
git log --oneline      # 查看历史
git reset HEAD~1       # 回退一个提交
```

**Q: 如何只构建而不打包？**
```bash
./release.sh → 选项 4
```

## 🔗 文档

- **RELEASE.md** - 完整功能文档
- **VERSION-SYNC.md** - 版本同步详细指南
- **RELEASE-CHEATSHEET.md** - 快速参考

## 💡 提示

✨ **推荐使用主菜单** (`./release.sh`) 以获得最佳体验

✨ **版本自动读取** - 无需手动输入当前版本

✨ **一键完整发布** - 使用菜单选项 6 或 `./scripts/release-quick.sh full`

---

**所有脚本都可执行，已测试，准备好使用！** 🎉
