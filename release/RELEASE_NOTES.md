# CS2 Selfiestick v0.4.6

## 支持版本 / Supported runtime

- **CS2**：Steam app `730`，buildid **`26445667`**。
- **HLAE**：**`2.192.10`**，AfxHookSource2 `0.41.9`。
- Windows x64，HLAE demo / 影视制作工作流。
- **正式版**，设为 GitHub Latest，不勾选 Pre-release。

## 相比 v0.4.5 的更新

- 修复道具与背景在同一机位内抽搐、闪烁的问题：通过游戏自身绝对坐标接口处理待重算的场景变换，再读取坐标。用户已实测确认 fix5 的抖动完全解决，0.4.6 保留该坐标与构图实现。
- 道具创建事件优先捕获，并补充完整 `0..32767` 槽位的分摊扫描。修复短寿命道具漏跟随、稀疏高槽位遗漏、归属信息延迟到达和上一颗阻挡下一颗接续的问题。
- 相机位置和朝向使用同一次有效采样，锁存首次有效出手方向，保持固定相对距离；不再独立积分镜头速度，不依赖周期刷新拉回机位。
- 短暂坐标／归属缺失时保留有效画面；下一颗道具尚无可用机位时保持上一画面，减少闪回观战。每颗道具锁存 FOV。
- **自动接续**：落地后继续跟随，实际爆开后定格 **Demo 时间 2 秒**，随后返回当前探员原生主视角。暂停停止计时，慢放延长现实停留时间；继续等待后续新投掷。
- **快速衔接**：新的有效道具立即接管，无需等待上一颗 2 秒停留结束。
- **锁定当前道具**：一直跟随至爆开，之后无限保留最终机位，不被其它投掷打断。
- 列表按钮改为 **“选中并锁定”**：选择指定的有效候选，再应用同一锁定规则。上方按钮则锁住当前选中的道具；已结束且没有有效机位缓存的候选不可重新选择。
- **清除锁定并返回探员**：立即释放当前镜头，防止已清除道具下一帧又被自动抓回；后续新出手仍可自动捕获。下方“跟随目标→清除锁定”同时释放道具镜头并恢复探员目标跟随。
- 保留探员 `weapon_hand_r` 手部锚点、眼位第 173 槽、句柄序列校验、左右自拍与越肩预设，以及真正的 Dear ImGui Dark 面板、可折叠状态栏和 CPU/GDI 软件渲染后端。

## 验证范围

- 2026-11-08 对本机 CS2 `26445667` 重新核验：全部特征码、**30 个 schema 字段**、眼位虚表槽、实体创建调用链、绝对坐标重算路径，以及 engine2 Demo tick 接口。
- PowerShell 7 编译中英文 x64 静态运行库 DLL；原生兼容性、隐藏生产代码回归和两版依赖审计通过。
- 覆盖脏坐标缓存、固定相对距离、短寿命道具、跳转／暂停／慢放、临时坐标恢复、快速接续、128 Demo tick 释放、无限锁定、手动清除后不重新捕获，以及 ImGui 滑轨涂抹检查。
- 12 个稳定关键区段与用户确认的 fix5 基线哈希一致；源码包包含可重复运行的稳定区段校验清单与工具。
- 五个附件在草稿阶段全部回下载，核对大小和 SHA-256 后公开发布。

**验证边界**：fix5 的无抖动已有用户实机确认；0.4.6 新机位生命周期仅完成后台／隐藏进程验证，未进行本次实机 Demo、ReShade 共存或 FPS 测试。自动地图碰撞避让尚未实现。道具若完全处于两次可见相机帧之间，仍需慢放才能观察。

## 使用

1. 完整重启 HLAE/CS2，打开 demo，使用 `mirv_loadlibrary "<DLL 绝对路径>"` 加载中英文 DLL 之一。
2. `Ins` 打开面板，`F8` 启用。投掷前开启“道具自拍”，以提前接收道具创建通知。
3. 选择“自动接续”或“锁定当前道具”；需要立即回到探员时点击“清除锁定并返回探员”。
4. 人物 `R / B / U` 为右／后／上；道具 X/Y/Z 的直线距离为 `sqrt(X²+Y²+Z²)`。默认道具 Y 为 `-13.5`。
5. 个人 INI 不被更新流程覆盖；整包附带干净默认配置。覆盖已加载 DLL 不会更新进程内代码，需重启后重新加载。

## English

v0.4.6 is a regular release for **CS2 build `26445667`**, **HLAE `2.192.10` / AfxHookSource2 `0.41.9`**.

It retains the user-confirmed fix5 resolved-origin camera and improves short-lived projectile capture, sparse-slot discovery, delayed ownership, exact-handle handoff and temporary data recovery. The rigid prop-relative rig and per-shot FOV are unchanged.

AUTO HANDOFF follows through detonation, holds the final shot for **2 demo seconds**, then returns to the native agent camera while remaining armed for future throws. Pause stops the timer. New valid throws preempt the hold immediately. PIN CURRENT PROP retains the final view indefinitely; SELECT + PIN chooses a specific live candidate. CLEAR LOCK releases the view without automatically recapturing canceled shots.

The new lifecycle passed background/native regression checks only; no new live demo, ReShade coexistence or FPS test was performed. Automatic map collision avoidance is not implemented. Fully restart HLAE/CS2 after replacing a loaded DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.4.6-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.6-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.6-CS2-26445667-HLAE-2.192.10-windows-x64.zip` — 双语 DLL、手册、默认配置和热键参考。
- `Lyan_selfiestick-v0.4.6-source.zip` — 完整项目源码、第三方许可和构建／校验工具。
- `SHA256SUMS.txt` — 两个 DLL 与两个 ZIP 的校验清单。
