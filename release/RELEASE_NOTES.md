# CS2 Selfiestick v0.4.8

## 对应版本 / Runtime target

- **CS2：1.41.9.7 / Steam build 26598758**（2026-11-13 读取本机已安装版本）。
- **HLAE：2.193.0 / AfxHookSource2 0.42.0**。
- Windows x64，中英文界面；正式版，设为 Latest。

## 相比 v0.4.7 的更新

本版针对新 CS2 更新进行二进制适配，保留 0.4.7 人物／道具相机算法。

- 更新 client.dll、engine2.dll 精确版本指纹；不通过放宽版本校验加载旧地址。
- 从新 client 静态 schema 核验全部 **32 个字段**：人物 `C_CSPlayerPawn::m_angEyeAngles` 从 `0x3600` 改为 **`0x3630`**，其余被用到的字段保持核验结果。
- 重新定位人物 SetUpView 最后阶段覆写点，校验完整重放指令、栈对齐、历史保存及原生返回路径；保留 HLAE 早期回调串接。
- 重新定位绝对坐标读取／脏变换重算接口、实体创建／移除虚表和调用目标，保留已确认的道具防抖与短寿命捕获逻辑。
- 更新 Demo file、tick、暂停方法地址，并核验新暂停函数只读语义；仍使用 Demo 时间计算爆开后 2 秒停留。
- 更新诊断标记为 `schema-probe-v43-prop-origin-cs2-26598758`，同步回归夹具、默认发布参数和中英文说明。

## 保留功能

- 人物身体固定局部机位、左右居中自拍、越肩预设、R/B/U 和角度微调；武器／手臂动画不驱动机位几何。
- 人物末尾视角覆写、按探员／启用代次保持的共享 FOV、短时 Demo 时间偏航过渡和暂时数据缺失恢复。
- 道具短寿命捕获、固定相对距离、出手回看、自动接续、爆开后 **Demo 时间 2 秒**停留、无限锁定和手动返回。
- Dear ImGui Dark 面板、滑轨、可折叠运行状态、CPU/GDI 软件渲染及人物／道具运动诊断导出。

## 验证范围

- 旧版兼容检查在更新后可重复失败；完成适配后，同一套 client/schema、人物尾部和 Demo 时钟检查通过。
- 负例验证：即使使用新版本指纹，旧人物朝向偏移、旧尾部地址和旧 Demo 接口地址仍被拒绝。
- PowerShell 7 构建中英文 x64 `/MT` DLL、原生 Validation 和两版隐藏生产代码测试；DLL 依赖审计通过。
- 覆盖人物末尾跳板机器码、切枪／切刀／FOV 交替、慢放转向、暂停／跳转、短寿命道具、2 秒释放、无限锁定及 ImGui framebuffer 检查。
- 12 个 fix5 稳定区段、人物构图／偏航／FOV、身体间距保护，以及道具跟随算法保持原样。道具时钟仅更新已核验的版本／方法常量。

**验证边界**：本轮只做后台／离线／隐藏进程验证，没有启动、注入或前台操作 CS2；未进行新实机 Demo、ReShade 共存或真实 FPS 测量。0.4.7 的人物残余转身／切刀抖动仍未获实机确认消除；本版不新增防抖算法或宣称解决该问题。自动地图碰撞避让尚未实现。

## 升级和加载

1. 完整重启 HLAE/CS2，使用 **HLAE 2.193.0** 启动上述 CS2 版本并打开 Demo。
2. 只加载一种语言的一个 DLL：`mirv_loadlibrary "<DLL 的绝对路径>"`。
3. 按 **Ins** 打开面板，**F8** 启用，build 应显示 **v0.4.8**。
4. 个人 INI 保留；整包附带干净默认配置。替换已加载 DLL 后仍需重启，进程内旧 Hook 不会随文件覆盖更新。
5. 如人物仍抖动，在人物模式导出 `selfiestick_player_motion.csv`；道具模式导出 `selfiestick_prop_motion.csv`。

## English

v0.4.8 targets **CS2 1.41.9.7 / build 26598758** and **HLAE 2.193.0 / AfxHookSource2 0.42.0**.

This is a compatibility update. All 32 consumed schema fields were checked against the new client; player eye angles moved from 0x3600 to 0x3630. Exact PE guards, the final player view hook, resolved-origin methods, entity lifecycle targets and Demo clock/pause addresses were updated and verified. Old offsets/addresses remain rejected even under the new fingerprint.

The v0.4.7 player/prop camera algorithms, body clearance, short-lived projectile capture, demo-time effect hold, pins and UI are preserved. No new smoothing algorithm was introduced. Validation was background/offline only, without a new live demo, ReShade coexistence or in-game FPS test. Residual player turning/knife jitter has not received live confirmation of resolution. Fully restart HLAE/CS2 and load one updated DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.4.8-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.8-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.8-CS2-26598758-HLAE-2.193.0-windows-x64.zip` — 双语 DLL、手册、默认配置和热键参考。
- `Lyan_selfiestick-v0.4.8-source.zip` — 完整项目源码、许可、构建和验证工具。
- `SHA256SUMS.txt` — 两个 DLL 与两个 ZIP 的校验清单。
