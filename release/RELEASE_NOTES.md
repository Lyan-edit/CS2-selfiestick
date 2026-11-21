# CS2 Selfiestick v0.5.0

## 对应版本 / Runtime target

- **CS2：1.42.0.0 / Steam build 26798866**（2026-11-21 本机版本复核）。
- **HLAE：2.193.1 / AfxHookSource2 0.42.1**。
- Windows x64，中文／English；正式版。

## 相比 v0.4.9 的更新

本版适配 CS2 与 HLAE 更新，保留已有相机算法。

- 更新 client.dll、engine2.dll 精确版本指纹；未知版本仍由保护逻辑拒绝，不放宽检查。
- 核验 **32 个 Schema 字段**，本次偏移均保持一致，包括人物朝向 `0x3630`；现有七类特征码仍匹配。
- 重新定位人物原生 SetUpView 最终覆写位置，验证完整重放指令、函数边界、栈对齐、最终历史写入与返回路径。
- 更新人物渲染时钟的 SetGlobals 入口、Globals 存储和默认对象地址，保留按 Demo 渲染时间推进、暂停停止及跨线程共享偏航的录制防抖逻辑。
- 更新道具／人物绝对坐标接口、脏变换重算入口、实体创建／移除目标和 Demo file getter。短寿命道具捕获、固定距离、自动接续、爆开后 **Demo 时间 2 秒**停留、无限锁定和手动返回保持。
- 更新诊断标记为 `schema-probe-v44-prop-origin-cs2-26798866`，同步中英文说明和发布工具默认参数。

## 保留功能与验证

- 人物身体固定机位、左右自拍、越肩预设、R/B/U 和角度微调、FOV 保持、防穿模间距，以及 0.4.9 的热补丁、扫描器和诊断争锁保护保留。
- HLAE 官方 2.193.1 已适配 CS2 1.42.0.0。其实体句柄改为 uint32／无效值 0xFFFFFFFF；自拍杆原实现已使用该表示。
- PowerShell 7 构建中英文 `/MT` DLL、Validation 和隐藏生产代码回归，全部通过；依赖检查通过。
- 覆盖人物最终回调机器码、持枪／切刀／FOV、同帧跨线程、1200 FPS 时钟、暂停／跳转、道具／UI 回归；12 个确认稳定区段原样保留。
- 新指纹下故意恢复旧人物尾部、时钟、实体和坐标地址的 **8 个负例均被拒绝**。
- 本轮后台人物 1200 回调中位数／P95 均约 **0.003 ms**；未改变每帧相机算法。

**验证范围：本轮仅做后台／离线／隐藏进程检查，没有启动、注入或前台操作 CS2，未新增实机 Demo、ReShade 共存或真实 FPS 测试。剩余探员动画抽搐仍需真实 1200 FPS 录制确认，不能据后台回归承诺完全消除或零掉帧。自动地图碰撞避让尚未实现。**

## 升级和加载

1. 完整退出并重启 HLAE/CS2，使用 **HLAE 2.193.1** 启动上述 CS2 版本并打开 Demo。
2. 只加载一个语言版本：`mirv_loadlibrary "<DLL 的绝对路径>"`。
3. **Ins** 打开面板，**F8** 启用；build 应显示 **v0.5.0**。
4. 个人 INI 可保留；ZIP 提供干净默认配置。覆盖已加载 DLL 不会热更新旧 Hook。
5. 人物诊断仍为最近 8,192 条／46 列；道具诊断为最近 1,024 条，均通过显式工作线程导出。

## English

v0.5.0 targets **CS2 1.42.0.0 / build 26798866** and **HLAE 2.193.1 / AfxHookSource2 0.42.1**.

Exact PE guards and relocated player final-view, render-clock, resolved-origin, lifecycle and Demo-file targets were updated. All 32 consumed schema fields and seven signature families were checked against the installed binaries. Existing body placement, recording-time/cross-thread yaw stabilization, FOV, short-lived prop capture, handoff and effect-hold behavior remain intact. Eight old-address fixtures remain rejected under the new build fingerprint.

Bilingual native regressions, compatibility checks, failure guards and offline benchmarks pass. Validation is background-only: no new live-demo, ReShade coexistence, real FPS or 1200 FPS recording test was performed. Fully restart HLAE/CS2 when upgrading, then load one language DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.5.0-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.5.0-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.5.0-CS2-26798866-HLAE-2.193.1-windows-x64.zip` — 双语 DLL、手册、默认配置与热键参考。
- `Lyan_selfiestick-v0.5.0-source.zip` — 完整项目源码、许可、构建与验证工具。
- `SHA256SUMS.txt` — 上述四个附件的 SHA-256。
