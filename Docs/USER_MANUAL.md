# Lim.one 使用说明书

版本：0.0.10

## 简介

Lim.one 是一个两段式响度处理器：

- **Clipper（削波器）**：用于在限制器前做可控的削波/削峰，提升可用响度并减轻后级压力。
- **Limiter（限制器）**：提供 Classic / Modern 两种风格；Modern 带前瞻（Lookahead）、自适应包络、立体声链接等逻辑。

界面主要控件（与实际 UI 一致）：

- 顶部：`Modern/Classic` 模式按钮、`True Peak` 开关、`Simple/Adv` UI 模式
- 旋钮：`Clip Gain`、`Knee`、`Limit Gain`、`Character`、`Ceiling`
- 底部：`Oversampling` 倍率按钮、右下角齿轮（授权/设置）
- 表头计量：`CLIP` / `LIMIT` 两条 GR 表（左右声道） + 右侧 LUFS（ST / INT / MOM）
- 设置面板：`UI Scale` 下拉（缩放比例）、授权状态与激活/取消激活

## 安装（macOS）

如果你使用 `.pkg` 安装包：

- 双击运行 `.pkg`，按提示安装即可。

如果你手动安装（拷贝插件文件）：

- AU：`/Library/Audio/Plug-Ins/Components/`
- VST3：`/Library/Audio/Plug-Ins/VST3/`
- AAX：`/Library/Application Support/Avid/Audio/Plug-Ins/`

安装后重启宿主（Logic / Live / Pro Tools 等），或让宿主重新扫描插件。

## 授权（Activation）

Lim.one 未激活时会**静音输出**（音频会被清空），用于避免未授权使用。

激活步骤：

1. 打开插件界面，点击右下角 **settings（齿轮）**。
2. 在弹出的 **Authorization** 面板里，找到 **Activate** 区域。
3. 在 `License Key (20 chars)` 输入框粘贴你的 License Key。
4. 点击 **Activate**，状态 `Status` 变为 `Activated` 即完成。

取消激活（把授权迁移到其它机器）：

- 同一面板点击 **Deactivate**。

## 快速上手（推荐工作流）

目标：在不爆音、不明显失真的前提下，提高响度并稳定峰值。

1. **Ceiling**：建议先设到 `-1.0 dB`（流媒体）或 `-0.3 ~ -0.1 dB`（更激进）。
2. **Oversampling**：
   - 制作/实时：`Off` 或 `2x`（更低延迟/更低 CPU）
   - 导出/母带：`4x` 或更高（更少混叠、更稳）
3. **True Peak**：
   - 导出/交付建议打开（会在输出端追加重建安全限幅；CPU 与延迟会上升）。
4. **GR 分配（推荐）**：先把响度主要交给 Clipper，再让 Limiter 做最后 2~4 dB 的“收尾”更自然。
   - `CLIP`（Clipper GR）：约 `~3 dB` 左右（瞬态更高一点也可以，但听到明显毛刺/齿音边缘就回退）
   - `LIMIT`（Limiter GR）：约 `2~4 dB`
5. **Clip Gain + Knee**：先把 Clipper 推到目标 `CLIP` 范围，再用 Limiter 收尾。
   - `Knee` 越大越柔和，失真更少但也更“圆”；`Knee` 越小越硬、更容易听到削波质感。
6. **Limit Gain**：在 Clipper 已经分担后，再把 `LIMIT` 控制在 `2~4 dB` 的范围内。
7. **Character**：
   - 低值更稳、更保守
   - 高值更激进、更偏向瞬态响度（也更容易带来泵感/失真）

## 参数说明（与 UI 一致）

### Clip Gain（drive）

- 位置：左侧旋钮 `Clip Gain`
- 作用：进入 Clipper 的前置增益（dB）
- 建议：用来“顶”到 Clipper，让 Clipper 承担一部分削峰，减少 Limiter 的负担

### Knee（knee）

- 位置：左侧旋钮 `Knee`
- 作用：Clipper 的软硬程度（0% 更硬，100% 更软）
- 建议：
  - 硬：更响、更直接，但更容易出质感/齿音边缘
  - 软：更平滑、更宽容

### Limit Gain（limiter_drive）

- 位置：中间旋钮 `Limit Gain`
- 作用：进入 Limiter 的输入增益（dB）
- 建议：这是主要的“响度推进”旋钮；配合 `LIMIT` 表判断压缩量

### Character（character）

- 位置：大旋钮 `Character`
- 作用：宏参数，决定 Modern 的攻击/释放倾向与立体声链接倾向（Simple 模式下也会驱动一组高级参数映射）
- 建议：越往右越激进，越可能提高短期响度，但也更容易产生泵感/失真

### Ceiling（ceiling）

- 位置：右侧旋钮 `Ceiling`
- 作用：最终输出天花板（dBFS）
- 注意：开启 True Peak 时，内部会再留出一小段 margin，以降低下采样峰值回长导致的超限风险

### Mode（limiter_mode）

- 位置：左上角 `Modern/Classic`
- 作用：
  - Classic：更简单、偏“硬墙/经典数字限制器”手感
  - Modern：前瞻 + 自适应双包络，通常更稳、更透明，适合母带/总线

### True Peak（true_peak）

- 位置：`True Peak` 开关
- 作用：提高对 inter-sample peak 的安全性（并可能提高 CPU/延迟）
- 策略：开启时若当前 Oversampling 低于 4x，会在输出端追加 4x 重建检测与安全限幅，保证最终 `Ceiling` 更稳

### Oversampling（oversampling）

- 位置：底部 `Oversampling` 按钮
- 作用：过采样倍率（Off/2x/4x/8x/16x）
- 影响：
  - 倍率越高：混叠更少、限制更稳，但 CPU 更高、延迟更大
  - 倍率切换会改变插件 latency（宿主会补偿）
  - True Peak 开启且倍率低于 4x 时，会额外做 4x 重建限幅兜底

### UI Mode (ui_mode)

- 位置：右上角 `Simple/Adv`
- 作用：
  - **Simple**：隐藏高级参数，仅保留核心旋钮。此时 `Character` 宏参数会自动控制内部的高级参数。
  - **Adv**：在波形图区域展开 **Advanced 面板**。你可以查看并微调 `Lookahead`、`Release` 等每一个细节参数。
  - *注：切换到 Adv 模式不会改变声音，只是暴露更多控制权。*

### UI Scale (ui_scale)

- 位置：设置面板 `UI` 区域下拉框
- 作用：调整插件界面缩放比例（50%~200%）
- 建议：高分辨率屏幕可提升到 125%~175%，笔记本可保持 100% 或更小

### Adv 面板参数（Modern）

- 说明：以下参数只对 **Modern** 生效；在 `UI Mode = Adv` 时，滑杆会直接参与 Modern 算法。`UI Mode = Simple` 时，其中一部分会被 `Character` 的宏映射覆盖。
- Lookahead（lookahead，ms）：前瞻时间。越大越能提前“抓瞬态”，更稳但延迟更高。
- Hold（modern_hold_ms，ms）：增益衰减保持时间。越大越不易抖动（chatter），但恢复会更慢、更“粘”。
- Hold Rel（modern_hold_release_ms，ms）：从 Hold 进入释放的过渡时间。越大越平滑但更慢。
- Atk Tau Div（modern_attack_tau_div）：攻击时间常数的除数。越小攻击越快/更硬；越大越慢/更软。
- Rel Smth B（modern_release_smooth_base_ms，ms）：释放平滑基准时间。越大越顺滑但回弹更慢。
- Rel Smth R（modern_release_smooth_range_ms，ms）：释放平滑的可变范围。越大越“自适应”（在低频/大 GR 时更倾向额外变慢）。
- Adpt Fast（modern_adapt_fast_strength）：快释放自适应强度。越大时，低频/大压缩下快释放会被拉长（更稳、更不毛，但更可能带来泵感/回弹变慢）。
- Adpt Slow（modern_adapt_slow_strength）：慢释放自适应强度。越大时，低频/大压缩下慢释放会进一步拉长（更稳但更慢）。
- SC HPF（modern_sc_hpf_hz，Hz）：侧链高通。越高越不容易被低频触发，低频更“保住”，但也可能让低频峰值更难被压住。
- Ratio Base（modern_ratio_base）：瞬态判定门限基值（fast/slow 的比值门限）。越大越不容易判为“瞬态”（更偏延音逻辑、更稳）；越小越更容易按瞬态处理（更响但可能更跳）。
- Ratio Slope（modern_ratio_slope）：门限随 `Character` 变化的斜率。越大时 `Character` 对瞬态倾向的影响越明显。
- Tr Mix（modern_transient_mix）：瞬态判定时 fast/slow 的混合。越大越把 slow 包络也算进去（更稳、更少跳动，但瞬态更容易被压住）。
- Soft Safe（modern_soft_safety_strength）：软安全限制强度（硬墙前的“软保护”）。越大越不容易超限尖峰，但波形更圆、质感更明显。
- Rel Fast（modern_release_fast_ms，ms）：快释放基准时间。越小回弹更快、更响，但更容易失真/抖动。
- Rel Slow（modern_release_slow_ms，ms）：慢释放基准时间。越大越稳但更慢；越小更响但更易泵。
- Link Tr（modern_link_transients）：瞬态阶段立体声链接。越大 L/R 越一致（像更稳），但立体声更“收中”。
- Link Rel（modern_link_release）：释放阶段立体声链接。越大越不易左右晃动，但宽度会变小。

### 连动系统 (Link System)

v0.0.9 引入了智能参数连动系统：

- **Reset Links**：点击此按钮会将所有高级参数重置为当前 `Character` 宏参数所对应的推荐值，并开启连动开关。这是快速回归“标准声音”的最佳方式。
- **LINK 按钮**：每个高级参数旁都有一个 `LINK` 按钮。
  - **亮起 (On)**：该参数被“托管”，其数值会自动跟随 `Character` 旋钮的变化而平滑调整。此时手动调整该参数会暂时失效或立即触发解绑。
  - **熄灭 (Off)**：该参数处于“手动模式”，数值由用户自由设定，不再受 `Character` 影响。
- **手势操作**：当你在 Adv 面板手动拖动某个已 Link 的参数时，系统会自动检测手势并将该参数的 Link 状态设为 Off（解绑），确保你的手动调整不会被宏参数覆盖。

## 计量（Meters）

### CLIP / LIMIT（GR）

- `CLIP`：Clipper 的衰减/削波量趋势（越长代表削得越多）
- `LIMIT`：Limiter 的增益衰减量趋势

建议把削峰任务在两级间分配：

- 若 `LIMIT` 长期大幅工作（例如持续 10 dB 以上），优先增加 Clipper 分担（或降低目标响度）
- 若 `CLIP` 已经很重但仍不够响，说明已经接近失真边界，建议回退设置

### LUFS（Loudness）

右侧显示：

- `ST`：短期（Short-term）
- `INT`：综合（Integrated）
- `MOM`：瞬时（Momentary）
- `Reset`：清零 LUFS 统计

## 常见问题

### 1）打开插件后没有声音

- 未激活时插件会静音输出。请按“授权（Activation）”完成激活。

### 2）打开 True Peak / 高倍过采样后 CPU 很高

- 这是正常现象：过采样倍率越高 CPU 越高。
- 建议：实时制作用 `2x/4x`，只在导出时用 `8x+True Peak`。

### 3）听到明显失真/毛刺

- 降低 `Clip Gain` 或减小 `Limit Gain`
- 增大 `Knee`（更软）
- 降低 `Character`
- 把 `Ceiling` 稍微下调（留更多余量）

## 技术细节（可选阅读）

更详细的 DSP 口径与实现细节见：[TECHNICAL_DETAILS.md](file:///Users/loyan/Documents/Mixing/Limiter/Docs/TECHNICAL_DETAILS.md)。
