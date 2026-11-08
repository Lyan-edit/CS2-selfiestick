# 道具镜头保留与释放：fix6（2026-11-08）

本地构建：`v0.4.5 prop-hold-fix6`。既有 GitHub v0.4.5 Release 未改动。

## 稳定基线

用户已确认 fix5 的抽搐、背景抖动完全解决。修改前已保存 [完整稳定基线](2026-11-08-confirmed-stable-fix5.md)，包括源代码、中英文 DLL、工作区差异、逐文件校验清单和 12 个关键区段哈希。

fix6 的这 12 个区段与基线逐项一致：引擎绝对坐标接口调用、脏场景坐标重算、坐标读取、刚性构图、初始方向锁存、实体访问、实体出生监听、逐道具 FOV 和诊断采样实现均未修改。没有新增图形 Hook、运动预测、坐标平滑或镜头速度积分。

## 用户可见行为

| 操作/模式 | 行为 |
| --- | --- |
| 自动接续 | 飞行、落地后继续跟随；实际爆开或实体移除后保持最终有效机位 **Demo 时间 2 秒**，然后返回游戏原生探员视角。道具模式保持启用，后续新出手仍会自动捕获。 |
| 快速衔接新道具 | 新的有效投掷立即接管，无需等待上一颗的 2 秒结束。新实体坐标尚未准备好时继续保留上一幅有效画面，避免闪回观战。 |
| 锁定当前道具 | 锁住当前选中的这一颗，跟到实际爆开后无限保留最终机位，其它投掷不打断。 |
| 列表“选中并锁定” | 选择指定的有效候选道具，然后应用相同的锁定规则。它负责选择哪一颗，上方按钮负责锁住当前这一颗。无缓存的已结束候选仍禁用。 |
| 自动接续（解除锁定） | 恢复自动接管；没有后续候选且爆开后已满 2 Demo 秒时，下一回调返回探员。切换本身不重建飞行机位。 |
| 清除锁定并返回探员 | 立即释放当前缓存、道具选择和锁定状态；保留投掷顺序、压制已清除的现存候选，避免下一帧重复抓回。显式列表选择仍可重新锁定有效候选。 |
| 下方“跟随目标 / 清除锁定” | 道具模式下同时执行上述道具释放，并把探员目标选择恢复为“跟随”。人物模式的原有目标清除行为保留。 |

“落地”“速度归零”和预定起爆 tick 仍不代表爆开。终止判断继续使用实际效果标志或实体移除。最终效果画面只是固定机位，Demo 继续播放，烟雾、火焰等效果继续发展。

返回探员通过提前退出 SetUpView 覆写实现：不再写相机位置、角度或道具 FOV，也不切到人物自拍机位。

## Demo 时钟

用户指定按 Demo 时间计算。直接读取 HLAE SDK 同一接口：`Source2EngineToClient001` 的 `GetDemoFile`（槽 69）及 `IDemoFile::GetDemoTick`（槽 3），使用 64 tick/秒。因此 2 秒是 128 个 Demo tick；暂停不消耗计时，0.1 倍速下约需 20 秒现实时间。

依据：[HLAE SDK 接口](https://github.com/advancedfx/advancedfx-prop/blob/prop/cs2/sdk_src/public/cdll_int.h)、本地 HLAE 2.192.10 `AfxHookSource2/MirvTime.cpp`，以及本机 engine2.dll RTTI/函数体独立核对。

时钟只在已验证 client 和 engine2 精确指纹下调用，并核对实际方法地址、捕获访问异常。engine2 指纹为 timestamp `0x6AE91EC4`、image `0x00A15000`、checksum `0x00734CA7`。`GetDemoFile` RVA `0x76840` 是只读指针 getter；`GetDemoTick` RVA `0x35A80` 调用 tick 时钟并减去 Demo 起始 tick。

时钟不可用时保留最终镜头，等待有效 Demo 时钟，不擅自使用系统时间或可能休眠的探员模拟时间。时钟回退会重新建立保留起点；既有发现逻辑仍处理 Demo 回退时的投掷顺序重置。

## 后台验证

- PowerShell 7 中英文 x64 `/MT` DLL 构建、原生兼容性回归、两版依赖检查通过。
- 当前 CS2 build `26445667` / HLAE `2.192.10` 的特征码、30 个 schema 字段、render-eye 槽、出生监听和绝对坐标 getter 离线检查通过。
- 新增 Demo 时钟检查独立解析 engine2 主 RTTI/vtable、方法槽和函数体。错误 tick RVA、错误 engine2 指纹均被拒绝。
- 两版隐藏原生面板回归通过；新增边界覆盖：第 127 tick 仍保留、第 128 tick 释放、长时间暂停、0.1 倍播放、提前接续、锁定无限保留、解除已超时锁定、时钟失效/恢复/回退、清除后延迟归属不重新捕获、手动行锁仍生效、后续新投掷仍捕获。
- 原有 480 次脏缓存重算、1,800 次固定距离构图、30/60/144/500 FPS 与 0.1/1/8 倍模拟时钟、短寿命道具、稀疏高槽位、临时坐标/归属恢复、FOV、句柄重用、出生事件调用链和 ImGui 涂抹检查继续通过。
- 最新中文版模拟 32,768 个槽位、500 次回调：中位数 0.248 ms、p95 0.300 ms、最大 0.531 ms。这是离线发现开销，不代表游戏 FPS。
- `check_fix5_preservation.py` 对归档 manifest 验证：12 个受保护关键区段全部未变。

本次没有启动、注入或前台操作 CS2。fix5 的无抖动已有用户实机确认；fix6 的新生命周期完成后台测试，尚未取得新版实机 Demo 验收结果。

可重复检查：

```powershell
& 'D:/Anadonda3/python.exe' tools/check_fix5_preservation.py --manifest dist/checkpoints/2026-11-08-fix5-user-confirmed-114029/manifest.json
& 'D:/Anadonda3/python.exe' tools/check_prop_demo_clock.py --engine 'E:/SteamLibrary/steamapps/common/Counter-Strike Global Offensive/game/bin/win64/engine2.dll'
```

## 交付文件

| 文件 | 字节 | SHA-256 |
| --- | ---: | --- |
| `release/zh-CN/Lyan_CS2自拍杆.dll` | 1,162,752 | `548A85EBEA32682D63EF43297ACB1CC56910AB39C5D656949A1C96A0511C34B8` |
| `release/en-US/Lyan's selfiestick.dll` | 1,160,192 | `114C298438831DECC58FA3F185A7EC75280EF3B6FD094BC456183D30EAE9022F` |

`release/zh-CN/Lyan_CS2自拍杆_prop-hold-fix6.dll` 与 `bin/selfiestick_hlae.dll` 都与中文 DLL 一致。旧命名 `prop-origin-fix5.dll` 和稳定基线归档保留原样。个人 INI 未覆盖。完整重启 HLAE/CS2 后只加载一个新版 DLL。
