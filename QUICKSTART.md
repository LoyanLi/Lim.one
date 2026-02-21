# Lim.one - 快速开始

一个简洁强大的发布管理系统。

## 🚀 30 秒快速开始

```bash
# 显示交互菜单
./release.sh

# 选择操作（1-8）
# 1: 检查状态
# 2: 提交并推送
# 3: 更新版本
# 4: 构建
# 5: 打包
# 6: 完整发布
# 8: 退出
```

## 📁 项目结构

```
Lim.one/
├── release.sh                ⭐ 主交互脚本
├── .version                  版本文件（当前：0.1.0）
├── scripts/                  辅助工具
│   ├── release-quick.sh      快速命令
│   ├── release-diagnose.sh   诊断工具
│   ├── release-init.sh       版本初始化
│   ├── release-sync-version.sh  版本同步
│   └── release.ps1           Windows版本
├── docs/release/             📖 完整文档
│   ├── RELEASE-README.md     使用指南
│   ├── RELEASE.md            完整功能文档
│   ├── RELEASE-CHEATSHEET.md 快速参考
│   ├── VERSION-SYNC.md       版本同步详解
│   └── ...
├── config/                   ⚙️ 配置文件
│   ├── .releaserc.json       发布配置
│   └── Makefile.release      Make接口
└── [项目文件...]
```

## 💡 常见操作

### 查看版本
```bash
cat .version
# 输出：0.1.0
```

### 提交并推送
```bash
./release.sh
# 选择 2
# 输入提交信息
# 确认推送
```

### 更新版本
```bash
./release.sh
# 选择 3
# 选择 Major/Minor/Patch/Custom
```

### 一键完整发布
```bash
./scripts/release-quick.sh full
# 或
./release.sh → 选择 6
```

### 快速命令
```bash
./scripts/release-quick.sh status          # 检查状态
./scripts/release-quick.sh commit "msg"    # 提交
./scripts/release-quick.sh version         # 版本更新
./scripts/release-quick.sh build           # 构建
./scripts/release-quick.sh package         # 打包
```

## 📚 文档位置

| 文档 | 位置 | 内容 |
|------|------|------|
| 快速指南 | `docs/release/RELEASE-README.md` | 使用说明 |
| 完整文档 | `docs/release/RELEASE.md` | 所有功能详解 |
| 快速参考 | `docs/release/RELEASE-CHEATSHEET.md` | 命令速查 |
| 版本同步 | `docs/release/VERSION-SYNC.md` | 版本管理详解 |

## 🔧 诊断

```bash
./scripts/release-diagnose.sh
```

检查：
- ✅ 目录结构
- ✅ Git 配置
- ✅ CMake 安装
- ✅ 版本状态
- ✅ 可用脚本

## ⚙️ 配置

所有配置文件存放在 `config/` 文件夹：

- `config/.releaserc.json` - 发布配置
- `config/Makefile.release` - Make 接口

## 🔄 版本管理

自动从 `.version` 文件读取当前版本：

```bash
# 查看当前版本
cat .version

# 更新版本（通过菜单）
./release.sh → 选择 3
```

版本会自动同步到：
- ✅ CMakeLists.txt
- ✅ juce_add_plugin
- ✅ .version 文件
- ✅ 发布包名称

## 📦 输出位置

```
build_limone/        编译产物
dist/                发布包
  └─ Lim.one-0.1.0/
     ├─ builds/      编译文件
     ├─ docs/        文档
     └─ MANIFEST.txt 清单
```

## ❓ FAQ

**Q: 脚本在哪里？**
A:
- 主脚本：`release.sh`
- 辅助脚本：`scripts/` 文件夹

**Q: 配置文件在哪里？**
A: `config/` 文件夹

**Q: 文档在哪里？**
A: `docs/release/` 文件夹

**Q: 如何查看版本？**
A: `cat .version`

**Q: 如何更新版本？**
A: `./release.sh` → 选择 3

**Q: 快速发布怎么做？**
A: `./scripts/release-quick.sh full`

## 🎯 典型工作流

```bash
# 1. 完成功能开发
git add .
git commit -m "feat: new feature"

# 2. 更新版本
./release.sh
# 选择 3 (更新版本)
# 选择 3 (Patch) 或其他

# 3. 构建
./release.sh
# 选择 4 (构建)

# 4. 打包
./release.sh
# 选择 5 (打包)

# 完成！包在 dist/ 文件夹中
```

或一键完成：
```bash
./scripts/release-quick.sh full
```

## 📞 需要帮助？

查看完整文档：
```bash
cat docs/release/RELEASE-README.md
```

快速参考：
```bash
cat docs/release/RELEASE-CHEATSHEET.md
```

诊断系统：
```bash
./scripts/release-diagnose.sh
```

---

**一切就绪，开始发布吧！** 🚀
