# CS2 Selfiestick

CS2 Selfiestick is a Counter-Strike 2 spectator camera DLL for HLAE. It provides an ImGui control panel and two camera modes for cinematic demo work:

- Player Selfie: the original player-mounted selfiestick.
- Prop Selfie: a projectile-mounted selfiestick for smoke, molotov/incendiary, HE, flashbang, and decoy projectiles.

Latest source/release package in this repository: `v0.3.0`.

## Supported Versions

This build is marked for:

- HLAE: `2.190.2`
- Counter-Strike 2: Steam app `730`, buildid `23669931`
- CS2 app manifest timestamp used locally: `2026-06-11 08:24:56 +08:00`

CS2 and HLAE update often. If Valve or HLAE changes schema, entity layout, or `SetUpView`, load may fail by design and the panel will show a reason instead of silently using an unsafe fallback.

## Download

The repository tracks current release DLLs here:

```text
release/en-US/Lyan's selfiestick.dll
release/zh-CN/Lyan_CS2自拍杆.dll
```

The same current DLLs are also built in:

```text
bin/selfiestick_hlae_en-US.dll
bin/selfiestick_hlae_zh-CN.dll
```

## Quick Start

1. Start HLAE `2.190.2` and launch CS2.
2. Open or play a demo in spectator mode.
3. Load one DLL from the HLAE console.
4. Press `Ins` to open the panel.
5. Press `F8` or click the panel button to enable the camera.

English DLL:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

Chinese DLL:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\zh-CN\Lyan_CS2自拍杆.dll"
```

## Hotkeys

| Key | Action |
| --- | --- |
| `Ins` | Show or hide the ImGui panel |
| `F8` | Enable or disable Selfiestick |
| `F9` | Lock the current spectator target |
| `F10` | Return target mode to Follow |

The panel hotkey can be changed inside the panel. The setting is saved to `selfiestick.ini` next to the loaded DLL.

## Player Selfie Mode

Use `Camera Mode -> Player Selfie` for the original selfiestick behavior.

Controls:

- `Selfie Left`: selfie-facing composition from the target's left side.
- `Selfie Right`: selfie-facing composition from the target's right side.
- `Forward`: forward-facing camera mode.
- `Follow`: use the current spectator target each frame.
- `Lock Current`: lock the current target agent.
- `Clear`: clear the locked target.
- `R / B / U`: local trim for Right / Back / Up.

The built-in player framing is applied first. `R / B / U` are extra local trim values on top of that framing.

## Prop Selfie Mode

Use `Camera Mode -> Prop Selfie` to follow projectiles thrown by the current target agent.

Supported projectile types:

- smoke
- molotov / incendiary
- HE grenade
- flashbang
- decoy

Behavior:

- The target agent still comes from the same `Follow` / `Lock Current` logic as Player Selfie.
- The DLL scans only projectiles owned/thrown by that target pawn or controller.
- If exactly one valid flying projectile exists and no prop is locked, the DLL auto-locks it.
- If multiple valid projectiles exist, it does not auto-switch. Use `LOCK` on the desired candidate.
- `CLEAR PROP` clears the current prop lock. If exactly one valid prop still exists on the next frame, it can auto-lock again.
- Tracking is only for flying projectile entities. When the projectile ends or disappears, the prop camera stops overriding and reports the reason.

Prop controls:

- `Prop X / Y / Z`: local prop camera offset.
- `Pitch / Yaw / Roll`: local view angle trim.
- Recent Props: shows recent projectile candidates, validity, handle, age, and lock buttons.

The prop camera uses a smoothed, predicted projectile anchor to reduce stepping and jitter across slow motion and normal demo speed.

## Status And Debugging

The Runtime panel shows:

- enabled state
- camera mode
- look mode
- panel hotkey
- target mode
- locked/current target handles
- prop lock and candidate list
- last override result
- failure reason

Debug trace is written next to the loaded DLL:

```text
selfiestick_trace.log
```

The current trace marker is:

```text
schema-probe-v20-left-selfie-offset-tune
```

If the marker is older, HLAE/CS2 is loading an old DLL. Fully restart HLAE and CS2 after replacing DLLs because Windows keeps loaded DLLs inside the process.

## Build From Source

Requirements:

- Visual Studio 2022
- MSVC C++ toolchain
- Windows SDK

Native solution:

```text
native_dll/selfiestick_hlae.sln
```

Validation executable:

```text
native_dll/selfiestick_hlae_validation/
```

The validation project checks compatibility gates, schema helpers, SetUpView patch helpers, prop projectile classification, owner matching, auto-lock policy, prop camera math, and camera smoothing helpers.

Do not run `npm run build` during agent sessions for this repository. The native DLL flow is independent of the Next.js app guidance.

## Repository Layout

```text
native_dll/selfiestick_hlae/             Main DLL source and Visual Studio project
native_dll/selfiestick_hlae_validation/  Lightweight native validation executable
native_dll/third_party/imgui/            Vendored Dear ImGui sources used by the DLL panel
release/en-US/                           English release DLL and manual
release/zh-CN/                           Chinese release DLL and manual
bin/                                     Local build outputs
selfiestick_bind.cfg                     Hotkey reference
```

## Safety Notes

- This DLL is intended for HLAE demo and cinematic workflows.
- It is not intended for matchmaking, live play, or anti-cheat protected servers.
- Joining VAC-protected servers with HLAE or injected tools can be unsafe.

## License

MIT License. See [LICENSE](./LICENSE).
