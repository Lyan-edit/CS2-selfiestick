# CS2 Selfiestick v0.4.7

## 对应版本 / Runtime target

- **CS2**：Steam app `730`，build **26445667**，游戏版本 **1.41.9.6**。
- **HLAE**：**2.193.0**，AfxHookSource2 **0.42.0**。
- Windows x64；中英文界面；正式版，设为 Latest。

HLAE 2.193.0 的本地包、官方相机 Hook 源码与串接格式已静态核对。本轮仅后台／隐藏进程验证，未启动、注入或前台操作游戏；不将静态兼容检查表述为完整实机验证。

## 相比 v0.4.6 的更新

- **人物相机脱离手部和武器动画**：改用探员身体固定局部机位，保留左右居中自拍、越肩预设及 R/B/U 微调。枪械、刀和动画眼位不再驱动相机位置；身体坐标由引擎重算，下蹲高度来自经核验的移动服务字段。
- **人物最终视角覆写**：在原生 SetUpView 后续位置／角度／FOV 处理结束后写入人物构图，随后由原生路径保存最终历史。保留早期 HLAE trampoline 串接，道具继续使用原回调路径。
- **人物 FOV 按探员／启用代次保持**，并在回调线程之间共享，切枪、切刀不重新捕获 FOV。需要采用新的 HLAE FOV 时，关闭再开启人物自拍。
- 人物暂时缺少场景或武器元数据时复用同一有效探员的已验证构图；完整句柄、实体系统、启用代次和跳转校验限制缓存复用。
- 慢放时对人物局部偏航采用短时 Demo 时间过渡，处理角度跨界、暂停和跳转；身体原点和相机身体间距保护保持当前实现。
- **人物运动诊断**：人物模式导出 `selfiestick_player_motion.csv`，最多 1,024 条、34 列，包含原始／处理后偏航、估计 Demo 倍率、原生输入 FOV、线程 ID 和早期／末尾回调阶段。相机回调不写文件。
- **道具稳定基线保留**：五个 `prop_*.inl` 文件保持当前版本原样；短寿命捕获、快速接续、爆开后 Demo 时间 2 秒停留、无限锁定和手动返回继续保留。
- 保留真正的 Dear ImGui Dark 面板、参数滑轨、可折叠运行状态，以及 CPU/GDI 软件渲染后端，无新增 D3D/DXGI/Present Hook。
- 附带 MulNX_CS2-Beta17 人物实现对比报告与可重复分析工具；本版没有直接移植其武器三骨骼、同 tick 跳过写入或世界坐标平滑逻辑。

## 验证与已知限制

- PowerShell 7 构建中英文 Windows x64 `/MT` DLL、原生兼容验证及隐藏生产代码测试。
- 当前 client/schema 的 **32 个 schema 字段**、相关特征码、人物末尾补丁边界、Demo 时钟／暂停接口通过离线检查。
- 后台实际执行生产末尾跳板机器码，覆盖后续原生视角改写、连续枪／刀切换、FOV 交替、寄存器恢复、最终历史拷贝、跨线程 FOV、0.1x 转向、关闭后原生返回等。
- 人物／道具、暂停／慢放／跳转、锁定和 ImGui framebuffer 回归通过；DLL 依赖审计、稳定区段哈希与附件校验按发布流程检查。

**验证边界**：道具 fix5 无抖动，以及 body-fix2 人物不再穿模，已有用户实测反馈。后续末尾覆写修正尚未收到实机确认转身／切刀残余抽搐已消除；本版不承诺所有场景无抖动或无掉帧。未进行本轮实机 Demo、ReShade 共存或真实 FPS 测量。自动地图碰撞避让尚未实现；身体间距保护不等于地图碰撞检测。

## 加载和升级

1. 完整重启 HLAE/CS2，使用 HLAE **2.193.0** 打开 Demo，只加载一种语言的一个 DLL。
2. 控制台执行 `mirv_loadlibrary "<DLL 的绝对路径>"`；`Ins` 打开面板，`F8` 启用。
3. build 应显示 **v0.4.7**；人物锚点为 **pawn_body**。覆盖磁盘上的旧 DLL 不会更新已加载进程内的 Hook。
4. 个人 INI 保留；整包附带干净默认配置。旧构图迁移时只重置过时人物微调并备份，保留道具参数。
5. 若人物仍抖动，在人物模式“运行状态→导出抖动诊断”导出 CSV。

## English

v0.4.7 targets **CS2 build 26445667 (1.41.9.6)** and **HLAE 2.193.0 / AfxHookSource2 0.42.0**.

Player framing now uses a fixed body-local rig, independent of hand, weapon and animated eye position. The player pose is applied at the verified final native SetUpView stage, retaining the original HLAE chain and prop callback path. Player FOV is shared across threads and captured per agent/enable session. Toggle Player Selfie off/on to adopt a new HLAE FOV. A short demo-time yaw transition and bounded 34-column player CSV diagnostics are included.

The confirmed prop implementation, two-demo-second effect hold, quick handoff, indefinite pins and manual native return are preserved. MulNX Beta17 was reviewed, without directly importing its camera implementation.

This release passed background/native checks only. The user confirmed the earlier prop shake fix and body-fix2 clipping improvement; final-view turning/knife jitter has not been confirmed resolved in a live demo. No new ReShade coexistence or in-game FPS measurement was performed. Automatic map collision avoidance is not implemented. Fully restart HLAE/CS2 and load one updated DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.4.7-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.7-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.7-CS2-26445667-HLAE-2.193.0-windows-x64.zip` — 双语 DLL、手册、干净默认配置与热键参考。
- `Lyan_selfiestick-v0.4.7-source.zip` — 完整项目源码、许可、构建与验证工具。
- `SHA256SUMS.txt` — 两个 DLL 和两个 ZIP 的 SHA-256。
