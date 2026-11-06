# CS2 Selfiestick v0.4.5beta

## 支持版本 / Supported runtime

- CS2: Steam app `730`, buildid `26445667` (installed update: `2026-11-05 20:10:37 +08:00`).
- HLAE: `2.192.10`, AfxHookSource2: `0.41.9`.
- Windows x64; intended for HLAE demo / cinematic workflows.
- GitHub Pre-release / Beta：这是测试版。

## 本版更新

- 使用真正的 Dear ImGui 控件和官方 Dark 风格；十二个位置/角度参数提供滑块与数值输入，运行状态可折叠。
- 保留 CPU/GDI 软件后端，不创建 D3D/DXGI 设备、交换链或 Present，减少与 ReShade 图形钩子的交互。
- 修复软件渲染的斜线接缝和滑块残影；拖动结束后才保存配置，减少反复写入 INI。
- 人物自拍从枪口/刀前侧回看探员上半身中心，左右机位对称；提供居中自拍与越肩预设，并限制相机穿到探员背后。
- 道具自拍使用出手方向的水平反向轴，镜头瞄准道具当前原点；改进短寿命道具捕获及消失后的短暂镜头保留。
- 修复配置版本被错误写成 v3、导致下次启动重复迁移的问题，当前构图配置为 `FramingVersion=5`。
- Schema 使用与当前 client.dll PE 指纹绑定的偏移表，避免调用猜测的 TypeScope 虚函数造成 Fatal error。未知版本会拒绝启用相机。
- 保留 HLAE 2.192.10 已改写 SetUpView 位点的串接处理，以及分批实体扫描和 attachment 缓存。

## 适配确认与验证范围

2026-11-06 对本机 build `26445667` 重新检查：

- 所需实体/附件/SetUpView 特征码匹配，附件调用目标和 16 字节 SetUpView 指令边界、RBX 视角指针上下文通过。
- 直接从本机 client.dll 的静态 schema 表逐类核对全部 22 个字段，偏移一致。
- PE 指纹：timestamp `0x6AEBC4DA`，image `0x029A3000`，checksum `0x0256CDA1`。
- 发布前重新编译中英文 DLL，执行兼容与相机数学回归、隐藏窗口面板渲染和依赖审计。
- 按用户要求仅在后台验证，未进入 demo 实测。游戏内构图、ReShade 共存和实际帧率仍需实机确认，不作零闪退/零掉帧承诺。

## 使用

1. 完整重启 HLAE/CS2，打开 demo。
2. 使用 HLAE 控制台 `mirv_loadlibrary "<DLL 的绝对路径>"` 加载中英文 DLL 之一。
3. `Ins` 打开面板，`F8` 启用相机。
4. 人物 `R / B / U` 分别为右、后、上；自拍时 B 减小拉远、增大拉近。道具 Y 默认 `-13.5`，更负更远。
5. 已加载的旧 DLL 不会因覆盖文件而更新，必须重启后加载新版。

## English

This beta updates the exact compatibility gate to installed CS2 build `26445667` and HLAE `2.192.10` / AfxHookSource2 `0.41.9`. All required signatures, attachment CALL targets, SetUpView instruction boundaries and all 22 schema fields were verified offline against the installed binary.

The release includes genuine Dear ImGui Dark controls, a CPU/GDI renderer, centered player lookback and over-shoulder presets, projectile launch-axis lookback, bounded discovery and framing-persistence fixes. Background validation covers native compatibility/camera regression, hidden-window UI rendering and release dependencies. No in-demo, ReShade coexistence or frame-time test was performed.

Restart HLAE/CS2 after replacing a loaded DLL. Press Ins for the panel and F8 to enable. Player R/B/U mean Right/Back/Up; decrease B to move farther from the subject.

## Downloads / 下载

- `Lyan_selfiestick-v0.4.5beta-zh-CN.dll` — 中文 DLL。
- `Lyan_selfiestick-v0.4.5beta-EN.dll` — English DLL.
- `Lyan_selfiestick-v0.4.5beta-CS2-26445667-HLAE-2.192.10-windows-x64.zip` — 双语 DLL、手册、默认配置及热键参考。
- `Lyan_selfiestick-v0.4.5beta-source.zip` — 完整项目源码（含第三方许可与构建工具）。
- `SHA256SUMS.txt` — four asset SHA-256 checksums.
