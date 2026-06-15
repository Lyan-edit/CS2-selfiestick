# CS2 Selfiestick v0.3.3 Release Notes

## Supported Runtime

- HLAE: `2.190.2`
- Counter-Strike 2: Steam app `730`, buildid `23669931`
- Local CS2 app manifest timestamp: `2026-06-11 08:24:56 +08:00`

This build is intended for HLAE demo/cinematic workflows. CS2 and HLAE updates can break schema, entity layout, or `SetUpView`; if that happens, the DLL reports a clear Runtime reason instead of silently falling back to unsafe behavior.

## Assets

- English DLL: `release/en-US/Lyan's selfiestick.dll`
- Chinese DLL: `release/zh-CN/Lyan_CS2自拍杆.dll`
- GitHub Release English download: `Lyan_selfiestick_en-US.dll`
- GitHub Release Chinese download: `Lyan_CS2_selfiestick_zh-CN.dll`
- English manual: `release/en-US/Manual.md`
- Chinese manual: `release/zh-CN/操作手册.md`

## What's New

- Rebuilt English and Chinese release DLLs with the MSVC runtime statically linked, so users do not need the Visual C++ Redistributable to load the DLL.
- Removed load-time dependency on `D3DCOMPILER_47.dll`; the ImGui DX11 backend now loads D3DCompiler dynamically only when shader compilation is needed.
- Added release dependency checks for VC++ runtime DLLs and `D3DCOMPILER_47.dll`.
- Prop Selfie candidates now store and display the actual thrower handle.
- Prop Selfie direction now prefers the actual projectile thrower instead of the current spectator target, fixing wrong look-back direction when teams or viewpoints differ.
- Added a short smoke post-flight hold so smoke projectile shots do not end instantly when the flying smoke entity disappears.
- Added shared `Image Style` prop preset values: `X=-8`, `Y=-22`, `Z=4`, `Pitch=1`, `Yaw=0`, `Roll=0`.
- Added an `IMAGE STYLE` / `截图风格` button in the Prop Selfie panel.
- Documented the recommended Prop Selfie workflow: tune projectile camera shots at `0.1x` demo playback speed for the smoothest setup and centering.
- Added full six-parameter ImGui controls for Player Selfie: `R / B / U` position trim plus `Pitch / Yaw / Roll` angle trim.
- Added labeled Player Selfie sliders for `RIGHT`, `BACK`, `UP`, `PITCH`, `YAW`, and `ROLL`.
- Added labeled Prop Selfie position sliders for `X`, `Y`, and `Z`; prop angle sliders remain available for `Pitch`, `Yaw`, and `Roll`.
- Added persistent `PlayerPitch`, `PlayerYaw`, and `PlayerRoll` settings in `selfiestick.ini`.
- Fixed ImGui numeric editing for prop angle fields. Number input now accepts typed digits again.
- Added medium-sensitivity `Pitch`, `Yaw`, and `Roll` sliders for prop camera angle trimming.
- Added ImGui control panel.
- Added bilingual English / Chinese DLL builds.
- Added `Player Selfie` / `Prop Selfie` camera mode switch.
- Added Prop Selfie camera for smoke, fire, HE, flashbang, and decoy flying projectile entities.
- Added unique projectile auto-lock when the current target has exactly one valid flying projectile.
- Added manual prop candidate locking for multi-projectile situations.
- Added independent prop camera `X / Y / Z` offset and `Pitch / Yaw / Roll` rotation trim.
- Improved prop smoothing and predictive anchor tracking to reduce stepping, jitter, and speed mismatch.
- Fixed Chinese Runtime text corruption for look mode and target mode.

## 中文说明

当前版本支持：

- HLAE：`2.190.2`
- CS2：Steam app `730`，buildid `23669931`

新增内容：

- 中英文 release DLL 改为静态链接 MSVC 运行库，用户不需要安装 Visual C++ Redistributable 也能加载 DLL。
- 移除对 `D3DCOMPILER_47.dll` 的加载时依赖；ImGui DX11 后端只在需要编译 shader 时动态加载 D3DCompiler。
- 新增 release 依赖检查脚本，用来防止 VC++ 运行库和 `D3DCOMPILER_47.dll` 重新进入 DLL 导入表。
- 道具候选现在保存并显示实际投掷者 handle。
- 道具自拍回看方向优先使用实际投掷该道具的玩家，不再单纯跟随当前观战目标方向，改善 T 方烟或切视角时朝向不对的问题。
- 烟雾弹飞行实体消失后增加短暂镜头保留，避免烟刚爆开自拍镜头立刻结束。
- 新增统一截图风格道具参数：`X=-8`、`Y=-22`、`Z=4`、`俯仰=1`、`偏航=0`、`翻滚=0`。
- 道具自拍面板新增 `截图风格` 按钮，一键写入这组参数。
- 文档新增推荐使用方法：道具自拍建议先把 demo 调到 `0.1x` 倍速，再锁定道具并调节构图，最适合保持快速道具居中。
- 人物自拍新增完整六参数 ImGui 控制：`右 / 后 / 上` 位置微调，加 `俯仰 / 偏航 / 翻滚` 角度微调。
- 人物自拍新增标注清楚的 `右`、`后`、`上`、`俯仰`、`偏航`、`翻滚` 六条滑轨。
- 道具自拍补齐 `X`、`Y`、`Z` 位置滑轨，道具角度 `俯仰 / 偏航 / 翻滚` 滑轨继续保留。
- `selfiestick.ini` 新增 `PlayerPitch`、`PlayerYaw`、`PlayerRoll` 持久化键。
- 修复道具角度数字输入框无法输入数字、只能删除的问题。
- 给道具相机 `俯仰 / 偏航 / 翻滚` 增加中等灵敏度滑轨。
- 面板切换为 ImGui。
- 保留英文 DLL，同时新增完整中文 DLL。
- 新增 `人物自拍 / 道具自拍` 相机模式切换。
- 道具自拍支持烟、火、雷、闪、诱饵飞行实体。
- 当前目标探员只有一个有效飞行道具时自动锁定。
- 多个道具同时存在时不自动抢锁，需要手动选择候选道具。
- 道具相机支持独立 `X / Y / Z` 偏移和 `Pitch / Yaw / Roll` 旋转微调。
- 优化道具跟随平滑与预测，减少跳帧、抽搐、卡顿和速度不匹配。
- 修复中文 Runtime 状态框里朝向和目标模式后的乱码 / 问号问题。
