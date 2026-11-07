# CS2 Selfiestick v0.4.5

## 支持版本 / Supported runtime

- CS2: Steam app `730`, buildid `26445667`.
- HLAE: `2.192.10`, AfxHookSource2: `0.41.9`.
- Windows x64，HLAE demo / 影视制作工作流。
- 正式 Release，设为 GitHub Latest；不勾选 Pre-release。

## 相比 v0.4.5beta 的修复

- 人物主锚点改为探员骨架上的 `weapon_hand_r` 右手附件，解除相机构图对枪械/刀实体生命周期的依赖。
- 手部附件短暂不可用时使用人物局部坐标缓存，再回退至身体相对位置；换枪、切刀和暂无活动武器时保持相同构图基准。
- 人物朝向从自身偏航构建，保持世界上方向；瞄准探员上半身中心，保留左右自拍与越肩预设。
- 修复眼位接口误用第 166 槽的问题。本机 client.dll 中该槽不写出眼位，旧代码把零缓冲误当成有效坐标，可能把镜头放到地图原点附近。现使用已核验的第 173 槽。
- 眼位输出改用 NaN 哨兵，拒绝未写出、部分写出、零坐标和远离人物的异常结果；最终相机写入也拒绝非有限数值。
- 校验完整实体句柄及序列号，拒绝已销毁实体槽位的复用对象；武器显示状态恢复不再通过旧裸指针写入。
- 无观战目标时拒绝误锁观察员实体和处于观察模式的人物。
- 构图配置为 `FramingVersion=6`。从旧配置迁移时先保存 `selfiestick.ini.pre-hand-anchor.bak`，一次性重置旧人物微调；道具参数保留。
- 保留真正的 Dear ImGui Dark 面板、十二组滑块/数值输入、可折叠运行状态，以及不创建 D3D/DXGI/Present 的 CPU/GDI 软件后端。
- 保留道具出手方向回看、原点居中、分批实体扫描和短寿命道具捕获逻辑。

## 发布前验证与限制

2026-11-07 对本机 build `26445667` 核验：

- 全部 22 个 class-qualified schema 字段与静态偏移表一致。
- 实体/附件/SetUpView 特征码、附件调用目标、16 字节指令边界和 RBX 视角指针上下文通过。
- C_CSPlayerPawn 眼位虚表第 173 槽的输出/相机服务路径通过；第 166 槽负向检查会被拒绝。
- 中英文 DLL 重新编译；原生回归、真实生产函数的隐藏窗口手部/眼位/面板测试及依赖审计通过。
- 上传后下载校验全部五个附件，核对文件大小和 SHA-256。

按用户要求仅做后台验证，尚未进入 demo 测试构图、ReShade 共存或实际帧率，不承诺零闪退/零掉帧。此次眼位修复针对错误世界坐标；普通近墙场景的自动碰撞避让尚未实现。人物实体自身死亡重生或 demo 跳转后重建时，旧锁定会安全失效，需要重新锁定。

## 使用

1. 完整退出并重启 HLAE/CS2，打开 demo。
2. `mirv_loadlibrary "<DLL 的绝对路径>"` 加载中英文 DLL 之一，避免同一进程混用旧版 DLL。
3. `Ins` 打开面板，`F8` 启用相机；需要清除人物微调时点击“居中人物自拍”。
4. `R / B / U` 为右/后/上；人物自拍 B 减小拉远、增大拉近。道具 Y 默认 `-13.5`，更负更远。
5. 面板打开时使用“锁定当前”按钮；F9 热键在面板隐藏且游戏前台时生效。
6. 覆盖已加载的 DLL 文件不会更新进程内代码，必须重启后重新加载。

## English

v0.4.5 is a regular GitHub release for CS2 build `26445667` and HLAE `2.192.10` / AfxHookSource2 `0.41.9`. It includes agent-hand anchoring independent of weapon lifetime, local missing-hand fallback, exact handle-serial validation, safe viewmodel restoration and spectator-target rejection.

The render-eye call now uses verified pawn vtable slot 173. The obsolete slot 166 returned without writing the output; accepting its zero-initialized buffer could place the camera near world origin. NaN sentinels, actor-relative validation and non-finite pose-write rejection guard this path.

Framing v6 backs up older settings before migrating player trim once; prop settings remain. Genuine Dear ImGui Dark controls and the CPU/GDI software renderer are retained.

Validation was background/offline only. No in-demo, ReShade coexistence or FPS test was performed. Automatic map collision avoidance is not implemented. Restart HLAE/CS2 after replacing a loaded DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.4.5-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.5-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.5-CS2-26445667-HLAE-2.192.10-windows-x64.zip` — 双语 DLL、手册、默认配置和热键参考。
- `Lyan_selfiestick-v0.4.5-source.zip` — 完整项目源码、第三方许可和构建工具。
- `SHA256SUMS.txt` — 两个 DLL 与两个 ZIP 的校验清单。
