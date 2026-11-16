# CS2 Selfiestick v0.4.9

## 对应版本 / Runtime target

- **CS2：1.41.9.7 / Steam build 26598758**（2026-11-16 本机二进制复核）。
- **HLAE：2.193.0 / AfxHookSource2 0.42.0**。
- Windows x64，中文／English；正式版。

## 相比 v0.4.8 的更新

- **录制时序**：人物偏航过渡改为按核验后的 Demo tick 与渲染插值推进，暂停停止计时；移除编码器墙钟耗时对相机滤波的影响。相同 1200 FPS 录制帧配不同编码延迟的后台测试，最大输出角度差为 **0.000000°**。
- **跨线程人物机位**：偏航历史按目标完整句柄、实体地址、实体列表与启用会话共享，避免换线程从原始偏航重新开始。同帧真实跳板用例的旧角度差 **0.385986°**，修正后机位／角度差均在 **0.0001** 以内。
- **热补丁保护**：线程打开、枚举、暂停或上下文检查失败时拒绝不安全写入；忙碌时先恢复线程再有界重试。覆盖失败后的句柄关闭与线程恢复。
- **特征码扫描保护**：跳过不可读／Guard 页，合并相邻可读区；扫描期间保护变化则丢弃结果。增加 PE 头、负偏移、非法／残缺 pattern 和匹配上限校验。
- **非阻塞人物诊断**：争用视模抑制锁时跳过诊断样本，不等待相机回调。缓存扩展为 **8,192 条、46 列**，包含精确渲染时间、原生输入机位、下蹲量与身体高度；导出仍由工作线程写盘。
- **加载与体积**：初始化 worker 提前建立进程常驻保护，关闭初始化线程句柄。大诊断数组改为零初始化存储，容量保留；审查构建中文 DLL 从约 **2.89 MB 降至 1.19 MB**。

## 保留功能

人物身体固定局部机位、左右居中自拍、越肩预设、R/B/U 与角度微调、会话 FOV、防穿模安全距离保持。已确认稳定的道具坐标与构图、短寿命捕获、自动接续、爆开后 **Demo 时间 2 秒**停留、无限锁定和手动返回保持。Dear ImGui 面板继续使用 CPU/GDI 软件渲染，不新增 D3D/DXGI/Present Hook。

## 验证范围与边界

- PowerShell 7 构建中英文 `/MT` DLL、Validation 与隐藏生产代码测试，全部通过；DLL 依赖检查通过。
- 本机 32 个 Schema 字段、人物尾部重放、Demo tick／暂停 ABI、渲染时钟检查通过。生产扫描器以只读、不可执行映射检查实际 client/schema 文件，签名匹配与独立检查一致。
- 故障注入覆盖线程／热补丁失败、不可读页与保护变化、诊断争锁；人物跨线程同帧、1200 回调、持枪／切刀／FOV、暂停／跳转及道具／UI 回归通过。
- 12 项稳定区段检查通过；道具 discovery/view/clock/lifecycle/diagnostics、人物身体原点与间距保护等已稳定部分保留。
- 三轮交替后台对照：人物回调中位数／P95 均约 **0.003 ms**；道具扫描耗时处于相近范围。微基准不能替代真实游戏帧率测试。

**本次仅后台验证，没有启动、注入或前台操作 CS2，也没有新增实机 Demo、ReShade 共存或真实 FPS 测试。尚未通过新的 1200 FPS 实录确认探员转身／下蹲／切刀残余抖动已消除。自动地图碰撞避让尚未实现。**

## 升级和加载

1. 完整退出并重启 HLAE/CS2，使用 **HLAE 2.193.0** 启动上述版本游戏并打开 Demo。
2. 只加载一个语言版本：`mirv_loadlibrary "<DLL 的绝对路径>"`。
3. **Ins** 打开面板，**F8** 启用；build 应显示 **v0.4.9**。
4. 个人 INI 可保留；ZIP 提供干净默认配置。不能通过覆盖文件热更新已经加载的 Hook。
5. 如仍抖动，在人物模式复现后导出 `selfiestick_player_motion.csv`，并保留录制命令及原始短片段。

## English

v0.4.9 targets **CS2 1.41.9.7 / build 26598758** and **HLAE 2.193.0 / AfxHookSource2 0.42.0**.

Player yaw now uses verified Demo render time rather than encoder wall time and shares history across callback threads. The release also hardens hotpatch failure handling, readable-page signature scanning, DLL lifetime and initializer handles. Player diagnostics are nonblocking on suppression contention and retain 8,192 samples across 46 columns. Zero-initialized storage reduces the DLL file size while retaining the buffer capacity. Stable player body placement, FOV and prop follow/handoff/hold behavior remain intact.

Bilingual native regressions, deterministic 1200 FPS clock tests, failure injection, installed-binary checks and offline benchmarks pass. Validation is background-only; there is no new live Demo, ReShade coexistence or real FPS test, and remaining actor jitter has not been confirmed resolved by a new 1200 FPS capture. Fully restart HLAE/CS2 when upgrading and load one language DLL.

## 附件 / Assets

- `Lyan_selfiestick-v0.4.9-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.9-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.9-CS2-26598758-HLAE-2.193.0-windows-x64.zip` — 双语 DLL、手册、默认配置和热键参考。
- `Lyan_selfiestick-v0.4.9-source.zip` — 完整项目源码、许可、构建与验证工具。
- `SHA256SUMS.txt` — 上述四个附件的 SHA-256。
