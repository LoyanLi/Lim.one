# 更新日志

## 0.1.0 (2026-02-22)

- **开源发布**：工程托管至 GitHub，代码公开。
- **自动化构建**：接入 GitHub Actions，每次推送自动构建 macOS 与 Windows 版本。
- **安装包自动生成**：macOS 生成 `.pkg` 安装包（双击安装，插件自动部署至系统路径）；Windows 生成 `.zip`（手动安装）与 `.exe` NSIS 一键安装包。
- **自动发布**：推送版本 tag 后，GitHub Release 页面自动创建并附上全平台安装包。

## 0.0.10 (2026-02-21)

- **实时性优化**：Modern Limiter 的 Lookahead 缓冲区改为预分配环形缓冲，消除了参数调整时的动态内存分配风险。
- **文档重构**：参数列表提取为独立 CSV 文件，简化技术文档。
- **手册更新**：完善 UI Mode 与连动系统的说明。

## 0.0.9 (2026-02-21)

- **高级面板重构**：全屏覆盖可视化区域，根据模式动态显示参数。
- **连动系统修复**：修复 "Reset Links" 和 "Link" 按钮数值不同步及误触 Unlink 问题。
- **手势系统优化**：引入新手势机制，准确区分用户操作与自动化，增强稳定性。
- **交互体验提升**：优化高级面板滚轮滚动行为，防止误触修改数值。
- **底层架构调整**：优化参数同步与锁机制。

## 0.0.8 (2026-02-20)

- Clipper/Limiter 算法拆分为独立实现文件（Soft/Hard 与 Classic/Modern）
- 新增 Clipper 模式选择参数 `clipper_mode`
- UI：Clipper/Limiter 模式与 Oversampling 改为下拉菜单并移到旋钮左上角
- UI：下拉菜单禁用键盘焦点触发

## 0.0.7 (2026-02-20)

- Lookahead 纳入系统延迟补偿，Classic/Modern 延迟一致
- Oversampling 组合统一延迟并追加差值对齐（含 clip 旁路场景）
- Delta 功能：输出 Clip/Limiter 处理差值，驱动增益在 Limiter 后自动补偿，主页面加入 Delta 开关
- 修正过采样下输出电平偏低的问题（统一 +0.1 dB 补偿）
- 修复 Clipper 线性段 ADAA 导致的高频滚降
- ADAA 仅在 knee>0 时启用
- 过采样补偿移动到 Delta 差分前的处理段

## 0.0.6 (2026-02-16)

- 修复警告
- 添加缩放功能

## 0.0.5 (2026-02-16)

- 添加检查更新功能
- 修复检查更新一直 Checking 并最终 Timeout：requestId 溢出导致回包匹配失败

## 0.0.4 (2026-02-16)

- True Peak 与 ISP Safety 合并：True Peak 打开即默认启用后置 ISP 兜底
- True Peak 打开时不再强制 Limiter 至少 4x：Limiter 仍按 Oversampling 选择运行；当低于 4x 时在输出端追加 4x 重建检测与安全限幅
- True Peak/ISP 检测口径迁移到过采样域：移除 3 点抛物线插值近似

## 0.0.3 (2026-02-16)

- 重写 Web UI，添加完整可视化：波形、Clip/Limit GR、LUFS Short-Term 历史轨迹
- 可视化新增设置：Ticks / Range / Speed，并持久化保存与自动恢复
- 停止/暂停播放时可视化持续滚动：补点逻辑重构，消除卡顿与抽搐
- In/Out 旋钮添加：Out 最大值限制到 +1 dB、默认 Out=-0.1 dB

## 0.0.2 (2026-02-15)

- 插件统一命名为 Lim.one（产物名/界面文案一致）
- Windows 强制使用 WebView2 后端，避免 IE 内核导致的页面加载失败
- Web UI 资源全量离线化：递归打包 web 目录资源并扩展资源提供器支持任意路径
- 字体本地化：内置 Inter 与 JetBrains Mono（woff2/woff），确保离线完整样式
- macOS 安装包脚本修复：更稳健地定位 artefacts 并输出 pkg

## 0.0.1 (2026-02-14)

- 初始版本
