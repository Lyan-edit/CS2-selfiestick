# 1200 FPS 人物录制时序检查与本地修正 — 2026-11-14

本地构建：`v0.4.8 recording-time-fix1`。目标仍为 CS2 1.41.9.7 / build 26598758、HLAE 2.193.0。
GitHub v0.4.8 Release 及本地标准名称 DLL 保留原发布文件。本次仅提供独立命名的本地测试 DLL。

## 用户反馈与复现边界

用户描述：镜头稳定，探员转身／下蹲时来回抽搐；使用 HLAE mirv_streams 以 1200 FPS 录制，Demo 倍率可能是 0.1 或更慢；相同录制设置关闭自拍杆后正常。

本次未取得原始视频、实机人物 CSV 或完整录制命令。后台假实体测试不能重建 CS2 的真实动画图和 GPU 骨骼画面，现有架构也没有真实模型渲染的离线回放接口。**以下已复现的是一个独立的相机录制时序缺陷，不能把它直接当作已证实的全部探员骨骼抽搐根因。**

排查顺序为：编码现实耗时影响采样；下蹲高度与渲染姿态不同步；重复视图／不同线程的历史状态。首先对第一项建立生产代码对照。

## 确认的缺陷及对照命令

旧人物偏航从整数 Demo tick 的现实间隔估算播放速率，再按现实回调耗时推进；超过 0.25 秒现实间隔还会重新建立偏航。逐帧录制的编码耗时不等于录制帧对应的 Demo 时间，这使相同录制内容可能产生不同相机输出。

生产 `FilterPlayerTurn` 对照输入为 3,600 个相同的 1200 FPS / 0.1x Demo 帧，使用同一组量化偏航；两路仅改变现实交付间隔，其中一路包含周期性 350 ms 编码停顿。

```powershell
pwsh -NoProfile -File tools/Build-Native.ps1 -Target UiPreview -Language zh-CN
# 在 native_dll/bin/native-UiPreview-zh-CN 中运行隐藏 EXE：
./selfiestick_ui_preview.exe
```

修改前：输出 `maximum yaw difference 4.089130 deg`，测试退出码 27。修改后，两版均为 `0.000000 deg`，全部测试退出码 0。该数值是确定性后台对照，不是实机探员动画抖动幅度。

## 本地修改

- 人物偏航仅按连续渲染 Demo 时间 `(demoTick + interpolation) / 64` 推进，现实编码耗时只作为诊断数据，不再决定相机结果。保留 0.012 秒 Demo 时间常数，没有增加平滑强度。
- 暂停保持、相同渲染时间的重复回调保持；跳转或回退重新建立状态。无法验证渲染时钟时停用该偏航过渡，不使用猜测的现实倍率。
- 从当前精确 client 的 `CSource2Client::SetGlobals` 原始方法静态定位 Globals 存储，只读读取 framecount 和插值。没有新增 Hook，没有写游戏时间、骨骼、姿态、下蹲量或插值设置。
- 人物诊断扩至 8,192 条、46 列，增加渲染 Demo 时间、渲染帧号、有效标记、插值分数、原生机位／角度、实际下蹲量和当前构图高度。沿用旧 34 列的前缀；仅显式导出时写文件。1200 FPS 早期／末尾双记录下约覆盖最近 3.4 秒。导出使用 17 位数字精度，以分辨高帧率慢放子帧。
- 已确认的身体原点／下蹲高度函数、身体间距保护、人物构图和 FOV 文件、全部五个道具文件、原早期／末尾跳板生成器均保持原样。没有以强制修改探员动画来掩盖问题。

## 渲染时钟证据

官方 HLAE v2.193.0 [MirvTime.cpp](https://github.com/advancedfx/advancedfx/blob/v2.193.0/AfxHookSource2/MirvTime.cpp) 使用 Globals +4 的 framecount、+56 的 interpolation_amount，并用 tick 与插值组成 Demo 时间。

安装版 client 的主 RTTI 确认 `CSource2Client` vtable[11] 为 RVA `0xB75C50`。原始函数加载／写入同一指针存储 `0x2221AB8`，无真实 Globals 时使用默认区 `0x2221AE0`。HLAE 修改其虚表调用目标，但继续调用原始 SetGlobals，因此本地只读访问不依赖 HLAE 私有导出。

`tools/check_player_render_clock.py` 核验精确 PE 指纹、主虚表槽、49 字节方法体、RIP 加载／存储目标及默认区。人为把存储地址错移 8 字节的负例被拒绝。运行时校验原方法的相关指令、拒绝空／默认指针、非有限或越界插值，以及读取中变化的帧号。

## 后台验证

- PowerShell 7 中英文 x64 `/MT` DLL 与隐藏生产测试构建通过；两版依赖审计通过。
- 原生尾部跳板实际执行、枪／刀／FOV 切换、暂停／跳转、恢复、原生返回及固定身体间距回归通过。
- 新精确时钟覆盖 0.01/0.1/1/8x × 30/60/144/500/1200 FPS；变动编码耗时对照、相同时间重复回调和无效时钟守卫通过。
- 1,200 次完整人物回调前后比较假探员对象及移动服务的全部受控字节，未出现写入。这是对直接写入行为的检查，不能替代真实引擎动画观察；引擎绝对坐标 getter 仍按原设计更新场景变换缓存。
- 新人物 CSV 独立解析：两版各 8,192 行 × 46 列，序号递增，至少 300 条有效渲染时钟样本及原生机位／下蹲字段。
- CS2 的 32 个 schema 字段、相关特征码、当前人物尾部边界、fix5 的 12 个保护区段及身体保护函数校验通过。
- 原道具刚性跟随、短寿命捕获、接续／停留／锁定和 ImGui framebuffer 回归通过。

修改前完整 v0.4.8 源码及发布 DLL 已归档：`dist/checkpoints/2026-11-14-v0.4.8-before-recording-fix/source-and-dlls.zip`。

## 试录与剩余证据

完整重启 HLAE/CS2，只加载新命名 DLL，沿用原来的 1200 FPS 录制配置，分别包含转身和下蹲。build 应显示 `v0.4.8 recording-time-fix1`。若仍抽搐，请在复现后尽快导出人物抖动诊断，并提供 CSV 及简短原始录制片段。`render_time_valid=1` 表示确实使用新时钟；不同线程、原生眼位与下蹲量可帮助判断剩余问题是否属于姿态采样或模型渲染。

本次没有实机录制或真实 FPS 测量，不能宣称人物骨骼抽搐已经完全解决。未改动系统设置、HLAE 配置或用户个人 INI，未发布／替换 GitHub Release。

| 文件 | 字节 | SHA-256 |
| --- | ---: | --- |
| `release/zh-CN/Lyan_CS2自拍杆_recording-time-fix1.dll` | 2,892,288 | `02A53820160A1CD96FEFF137852B4BAF82BAD6609B8EBFC3D013D9677CFC7B2E` |
| `release/en-US/Lyan_selfiestick_recording-time-fix1.dll` | 2,889,728 | `B3505668351737D550DC80210511B0A28ED1C5AE91E89652D27B37131E51B9A8` |
