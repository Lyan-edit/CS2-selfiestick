# CS2 Selfiestick v0.3.1 Release Notes

## Supported Runtime

- HLAE: `2.190.2`
- Counter-Strike 2: Steam app `730`, buildid `23669931`
- Local CS2 app manifest timestamp: `2026-06-11 08:24:56 +08:00`

This build is intended for HLAE demo/cinematic workflows. CS2 and HLAE updates can break schema, entity layout, or `SetUpView`; if that happens, the DLL reports a clear Runtime reason instead of silently falling back to unsafe behavior.

## Assets

- English DLL: `release/en-US/Lyan's selfiestick.dll`
- Chinese DLL: `release/zh-CN/Lyan_CS2自拍杆.dll`
- English manual: `release/en-US/Manual.md`
- Chinese manual: `release/zh-CN/操作手册.md`

## What's New

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
