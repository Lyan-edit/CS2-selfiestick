# CS2 Selfiestick v0.4.3 Release Notes

## Supported Runtime

- HLAE: `2.192.6`
- Counter-Strike 2: Steam app `730`, buildid `25687242`
- Local CS2 app manifest timestamp: `2026-10-03 08:39:58 +08:00`

This build is intended for HLAE demo/cinematic workflows. CS2 and HLAE updates can break schema, entity layout, or `SetUpView`; if that happens, the DLL reports a clear Runtime reason instead of silently falling back to unsafe behavior.

## Assets

- English DLL: `release/en-US/Lyan's selfiestick.dll`
- Chinese DLL: `release/zh-CN/Lyan_CS2自拍杆.dll`
- GitHub Release English download: `Lyan_selfiestick_en-US.dll`
- GitHub Release Chinese download: `Lyan_CS2_selfiestick_zh-CN.dll`
- English manual: `release/en-US/Manual.md`
- Chinese manual: `release/zh-CN/操作手册.md`

## What's New

- Updated and rebuilt against the current local CS2 app `730` buildid `25687242` and HLAE `2.192.6`.
- Fixed the current-build startup failure: an incomplete broad raw-schema scan no longer suppresses the exact schema interface path. Production resolution now starts from an exact class lookup and scans only that class's declared fields, avoiding inherited-field ambiguity.
- Disabled the expensive broad raw payload diagnostic pass in production. It remains available only as an offline compatibility probe, removing the multi-second scan/log burst seen during demo activation.
- Restyled the ReShade-safe Win32/GDI panel with the flatter neutral nSkinz visual hierarchy, single borders, and clearer active states while retaining the no-D3D/no-Present backend.
- Added a collapsed-by-default Runtime diagnostics bar. Expanding it resizes the panel to reveal the full live status; collapsing it restores the compact layout.
- Removed the unconditional 33 ms window repaint. The panel now repaints only when state, focus, input, or visibility changes, reducing UI work while a demo is playing.
- Replaced the in-process Dear ImGui/DX11 panel with a native Win32/GDI pseudo-ImGui window. Pressing `Ins` no longer creates a D3D device, DXGI swap chain, or Present call inside the ReShade-injected CS2 process.
- Removed direct `D3D11.dll` and `DXGI.dll` imports from both release DLLs. The dependency audit now rejects either import so the ReShade-safe UI path cannot regress silently.
- Preserved native controls for Player `R/B/U`, Player `Pitch/Yaw/Roll`, Prop `X/Y/Z`, Prop `Pitch/Yaw/Roll`, camera/target/look modes, prop candidates, hotkey capture, runtime status, and the Image Style preset.
- Fixed live loading after HLAE 2.192.6 has already patched the shared SetUpView site. Selfiestick now recognizes HLAE's 16-byte trampoline, chains it first, preserves its return value, and then applies the camera callback.
- Added native regression coverage for both the untouched CS2 SetUpView call site and the HLAE-prepatched call site, including byte-for-byte trampoline replay and `RBX` view-pointer forwarding.
- Recovered the unique current-client attachment signature with its second call at `+27`, plus the unique 16-byte SetUpView patch site used by HLAE 2.192.6.
- Updated the SetUpView trampoline to relocate the original `IsPlayingDemo` virtual call, preserve its return value, stop on an exact instruction boundary, and pass `CViewSetup` from `RBX`.
- Extended the installed-binary checker to verify both attachment call targets, the SetUpView data reference, the 16-byte boundary, and nearby `[RBX+0x4A0]` / `[RBX+0x4B8]` accesses.
- Updated current Source 2 schema layout handling: direct declared-class array at `0x470/0x478`, class field span at `0x24/0x30`, and field name/type order.
- Updated build-gated schema fallbacks for player/controller services, camera/observer services, eye angles, and grenade ownership in build `25687242`. Hardcoded fallbacks are disabled automatically when the client PE fingerprint changes.
- Moved viewmodel and grenade owner queries to the classes that currently declare those fields: `C_BaseEntity` and `C_BaseGrenade`.
- Player Selfie now reuses the last trusted target handle between 50 ms refreshes instead of repeating the full split-screen observer chain in every SetUpView callback; target changes still refresh promptly.
- Player and weapon attachment indices are cached per entity handle and attachment name, removing repeated `LookupAttachment` work from the per-frame camera path while invalidating safely when a transform fails.
- Prop Selfie discovery no longer scans the entire entity list in one SetUpView callback. Each callback processes at most 192 entities and commits candidates only after the pass completes, reducing frame-time spikes while the locked projectile still tracks every frame.
- In-progress Prop Selfie discovery is now limited to one 192-entity batch per 8 ms even if CS2 invokes SetUpView more than once in a rendered frame.
- Projectile position now uses the current `CGameSceneNode::m_vecAbsOrigin` schema path first. The older `0x1800` dynamic-origin search remains only as a guarded fallback, removing its roughly 1,536 candidate-vector checks from the normal camera hot path.
- Added build-gated/runtime schema support for `C_BaseEntity::m_vecVelocity` and `C_BaseCSGrenadeProjectile::m_nBounces`. Prop prediction combines the real velocity direction with measured movement rate so it respects demo timescale, keeps measured-delta fallback, stops extrapolating while playback is paused, and resets stale smoothing immediately after a bounce-counter change.
- Cached RTTI names per vtable and retained the 10 Hz unlocked / 4 Hz locked discovery cadence between completed prop scans.
- The DLL no longer installs the SetUpView camera hotpatch during `mirv_loadlibrary`; it loads first, then installs the hook lazily only when the user presses `F8` or clicks Enable.
- If SetUpView hook installation is not compatible with the current CS2 build, the DLL now stays loaded, keeps the camera disabled, and reports a Runtime reason instead of taking the demo process down.
- Fixed another external-PC demo-load no-response path found from a shared `selfiestick_trace.log`: the DLL was loaded successfully and `bridge-enter` was running, but startup schema diagnostics kept doing heavy schema probing and logging during early demo frames.
- Startup schema resolution is now lazy when the SetUpView hook can install without schema offsets. Schema offsets are resolved only when player/prop camera code actually needs them.
- Debug trace is now off by default both in code and in the packaged `selfiestick.ini`. Users can still enable `[Debug] TraceEnabled=1` when collecting diagnostics.
- Existing old `selfiestick.ini` files can still override the new default. Delete the old ini or set `[Debug] TraceEnabled=0` before testing this release on another PC.
- Removed an old packaged `selfiestick_trace.log` from the Chinese release folder so users do not copy stale debug output with the DLL.
- Fixed a demo-process freeze seen on some external PCs by removing heap allocation from the thread-suspended hotpatch window.
- Hotpatch installation now opens and stores thread handles before suspension, then only checks thread RIP, writes the SetUpView patch, and resumes threads inside the short suspended window.
- Fixed demo-time DLL loading hang by skipping the startup schema wait when SetUpView hook installation no longer depends on schema readiness.
- Added safer hotpatch installation that avoids writing the SetUpView patch while another thread is executing inside the patch range.
- Fixed external-user startup where HLAE printed `LoadLibraryA Ok.` but the DLL then stopped with `SetUpView hook blocked by compatibility gate` when schema offsets were not ready yet.
- SetUpView hook installation now requires only the trusted entity access, split-screen accessor, and patch site; schema offsets remain guarded lazily by the camera/target code that actually reads schema fields.
- Rebuilt English and Chinese release DLLs with the MSVC runtime statically linked, so users do not need the Visual C++ Redistributable to load the DLL.
- Removed runtime dependency on `D3DCOMPILER_*.dll`; v0.4.2 subsequently removed the DX11 UI backend entirely.
- Added release dependency checks for VC++ runtime DLLs, `D3DCOMPILER_*.dll`, and `D3DCompile` imports.
- Prop Selfie candidates now store and display the actual thrower handle.
- Prop Selfie direction now prefers the actual projectile thrower instead of the current spectator target, fixing wrong look-back direction when teams or viewpoints differ.
- Added a short smoke post-flight hold so smoke projectile shots do not end instantly when the flying smoke entity disappears.
- Added shared `Image Style` prop preset values: `X=-8`, `Y=-22`, `Z=4`, `Pitch=1`, `Yaw=0`, `Roll=0`.
- Added an `IMAGE STYLE` / `截图风格` button in the Prop Selfie panel.
- Documented the recommended Prop Selfie workflow: tune projectile camera shots at `0.1x` demo playback speed for the smoothest setup and centering.
- Added full six-parameter native controls for Player Selfie: `R / B / U` position trim plus `Pitch / Yaw / Roll` angle trim.
- Added native numeric fields for Player and Prop position/rotation tuning.
- Added persistent `PlayerPitch`, `PlayerYaw`, and `PlayerRoll` settings in `selfiestick.ini`.
- Fixed numeric editing for prop angle fields. Number input accepts typed digits.
- Added the standalone control panel, now rendered through Win32/GDI in v0.4.2.
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

- HLAE：`2.192.6`
- CS2：Steam app `730`，buildid `25687242`

新增内容：

- 已按当前本机 CS2 app `730` buildid `25687242` 和 HLAE `2.192.6` 重新构建。
- 修复当前版本的启动失败：原始 schema 扫描不完整时不再错误跳过精确 schema 接口；正式解析改为先按类名精确取类，再只遍历该类声明字段，避免继承字段重复导致歧义。
- 正式运行默认停用昂贵的全量原始 payload 诊断扫描，仅保留为离线兼容探针，消除启用自拍杆时持续数秒的扫描与日志突发。
- 在保持 Win32/GDI、无 D3D、无 Present 的前提下，改为更接近 nSkinz 的扁平中性层级、单线边框和清晰选中状态。
- 底部运行诊断改为默认收起的栏；展开时窗口自动增高显示完整实时状态，收起后恢复紧凑高度。
- 移除每 33ms 无条件重绘窗口，只有状态、焦点、输入或可见性发生变化时才重绘，减少 demo 播放期间的 UI 开销。
- 把进程内 Dear ImGui/DX11 面板替换为纯 Win32/GDI 绘制的伪 ImGui 独立窗口；按 `Ins` 不再在注入 ReShade 的 CS2 进程中创建 D3D 设备、DXGI 交换链或调用 Present。
- 中英文 DLL 已移除对 `D3D11.dll`、`DXGI.dll` 的直接导入；依赖审计会阻止这两个依赖重新进入发布包。
- 保留人物 `右/后/上`、人物 `俯仰/偏航/翻滚`、道具 `X/Y/Z`、道具 `俯仰/偏航/翻滚`、模式切换、道具候选、热键修改、运行状态与截图风格预设。
- 修复 HLAE 2.192.6 已先改写同一 SetUpView 位点后再加载自拍杆的兼容问题：现在会识别 HLAE 的 16 字节 trampoline，先串接执行 HLAE、保留返回值，再执行自拍杆相机回调。
- 新增原始 CS2 SetUpView 位点和 HLAE 已改写位点两条原生回归测试，校验 trampoline 原样重放与 `RBX` 视角指针传递。
- 恢复当前 client.dll 唯一命中的附件访问特征码（第二个调用位于 `+27`）和 HLAE 2.192.6 使用的 16 字节 SetUpView 补丁位点。
- SetUpView trampoline 会重放原始 `IsPlayingDemo` 虚调用并保留返回值，在完整指令边界结束，再从当前版本使用的 `RBX` 传入 `CViewSetup`，避免错误视角指针或半条指令导致闪退。
- 二进制检查器新增附件两个调用目标、SetUpView 数据引用、16 字节边界和附近 `[RBX+0x4A0]` / `[RBX+0x4B8]` 上下文校验。
- 更新当前 Source 2 schema 内存布局：声明类数组 `0x470/0x478`、类字段跨度 `0x24/0x30`，并修正字段名 / 类型顺序。
- 更新 build `25687242` 的人物/控制器服务、相机/观察服务、眼睛角度和手雷归属字段回退；client.dll PE 指纹变化后会自动禁用硬编码回退。
- 人物自拍在 50ms 刷新间隔内复用上次可信的目标句柄，不再在每次 SetUpView 回调里重复完整的分屏观战目标链，同时仍能及时响应目标切换。
- 人物与武器 attachment 索引按实体句柄和附件名缓存；transform 失败时会自动失效，减少每帧重复 `LookupAttachment`。
- 道具候选扫描改为每次 SetUpView 最多处理 192 个实体，完整一轮后才提交候选，消除单帧全实体遍历尖峰；已锁定道具仍保持逐帧跟随。
- 即使同一渲染帧内 SetUpView 被调用多次，进行中的道具扫描也限制为每 8ms 最多执行一批 192 个实体，避免批次在单帧内被连续跑完。
- 道具位置优先直接读取当前 `CGameSceneNode::m_vecAbsOrigin` schema 路径；原有 `0x1800` 动态原点搜索只作为受保护的回退，正常相机热路径不再每次检查约 1536 个候选向量。
- 新增 `C_BaseEntity::m_vecVelocity` 与 `C_BaseCSGrenadeProjectile::m_nBounces` 的运行时 schema / build 门禁回退。预测会把真实速度方向与实测移动速率结合，正确适配 demo 倍速；字段不可用时继续使用位置差分，播放暂停时停止外推，弹跳计数变化则立即清除旧预测，减少弹跳后拉扯和抖动。
- RTTI 类名继续按 vtable 缓存，并保留完整扫描后的未锁定 10 Hz / 已锁定 4 Hz 间隔。
- DLL 不再在 `mirv_loadlibrary` 加载瞬间安装 SetUpView 相机热补丁；现在会先完成加载，只有按 `F8` 或点击启用自拍杆时才延迟安装 hook。
- 如果当前 CS2 版本与 SetUpView hook 不兼容，DLL 会保持加载、保持相机关闭，并在 Runtime 区域显示原因，不再直接拖崩 demo 进程。
- 修复部分别人电脑上“大厅能加载 DLL，但进入 demo 后游戏进程无响应”的问题：热补丁安装时不再在暂停其它线程的窗口内分配堆内存，降低死锁风险。
- 根据别人电脑提供的 `selfiestick_trace.log` 继续修复一条进入 demo 后无响应路径：日志显示 DLL 已加载并进入 `bridge-enter`，但启动阶段仍在高频执行 schema 诊断和写日志。
- 当 SetUpView hook 已经不需要 schema offsets 就能安装时，启动阶段不再主动启动重 schema 解析；真正的人物 / 道具相机代码需要读取字段时再延迟解析。
- 代码默认值和 release 自带 `selfiestick.ini` 都已把 `TraceEnabled` 改为 `0`。只有排查问题时才建议手动打开 `[Debug] TraceEnabled=1`。
- 如果用户机器上已经有旧版生成的 `selfiestick.ini`，旧配置仍会覆盖新默认值；测试新版前请删除旧 ini，或确认 `[Debug] TraceEnabled=0`。
- 中文 release 目录中清理了旧的 `selfiestick_trace.log`，避免用户复制整包时把旧调试日志一起带走。
- 热补丁安装现在先打开并保存线程句柄，再进入极短暂停窗口；暂停窗口内只检查线程 RIP、写入 SetUpView 补丁并立刻恢复线程。
- 修复 demo 中加载 DLL 时的卡死：当 SetUpView hook 已不再依赖 schema 时，启动阶段不再等待 schema 解析。
- 新增更安全的热补丁安装流程：如果有线程正在补丁区内执行，先让线程退出该范围再写入 SetUpView 跳转。
- 修复别人电脑上出现 `LoadLibraryA Ok.` 但随后提示 `SetUpView hook blocked by compatibility gate` 的问题。原因是 schema offsets 还没准备好时，旧版本把 SetUpView hook 安装也一起阻塞了。
- SetUpView hook 现在只要求实体入口、分屏入口和 patch site 可信；真正读取 schema 字段的人物/道具逻辑仍然会延迟检查 schema offsets，不会盲目读字段。
- 中英文 release DLL 改为静态链接 MSVC 运行库，用户不需要安装 Visual C++ Redistributable 也能加载 DLL。
- 移除对 `D3DCOMPILER_*.dll` 的运行时依赖；v0.4.2 又进一步完全移除了 DX11 面板后端。
- 新增 release 依赖检查脚本，用来防止 VC++ 运行库、`D3DCOMPILER_*.dll` 和 `D3DCompile` 导入重新进入 DLL。
- 道具候选现在保存并显示实际投掷者 handle。
- 道具自拍回看方向优先使用实际投掷该道具的玩家，不再单纯跟随当前观战目标方向，改善 T 方烟或切视角时朝向不对的问题。
- 烟雾弹飞行实体消失后增加短暂镜头保留，避免烟刚爆开自拍镜头立刻结束。
- 新增统一截图风格道具参数：`X=-8`、`Y=-22`、`Z=4`、`俯仰=1`、`偏航=0`、`翻滚=0`。
- 道具自拍面板新增 `截图风格` 按钮，一键写入这组参数。
- 文档新增推荐使用方法：道具自拍建议先把 demo 调到 `0.1x` 倍速，再锁定道具并调节构图，最适合保持快速道具居中。
- 人物自拍保留完整六参数原生控件：`右 / 后 / 上` 位置微调，加 `俯仰 / 偏航 / 翻滚` 角度微调。
- 人物与道具位置/角度改为原生数字输入框。
- `selfiestick.ini` 新增 `PlayerPitch`、`PlayerYaw`、`PlayerRoll` 持久化键。
- 修复道具角度数字输入框无法输入数字、只能删除的问题。
- 控制面板在 v0.4.2 改为 Win32/GDI 伪 ImGui 绘制。
- 保留英文 DLL，同时新增完整中文 DLL。
- 新增 `人物自拍 / 道具自拍` 相机模式切换。
- 道具自拍支持烟、火、雷、闪、诱饵飞行实体。
- 当前目标探员只有一个有效飞行道具时自动锁定。
- 多个道具同时存在时不自动抢锁，需要手动选择候选道具。
- 道具相机支持独立 `X / Y / Z` 偏移和 `Pitch / Yaw / Roll` 旋转微调。
- 优化道具跟随平滑与预测，减少跳帧、抽搐、卡顿和速度不匹配。
- 修复中文 Runtime 状态框里朝向和目标模式后的乱码 / 问号问题。
