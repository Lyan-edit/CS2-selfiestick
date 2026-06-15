# CS2 Selfiestick

CS2 Selfiestick is a Counter-Strike 2 spectator camera DLL for HLAE. It provides an ImGui control panel and two camera modes for cinematic demo work:

- Player Selfie: the original player-mounted selfiestick.
- Prop Selfie: a projectile-mounted selfiestick for smoke, molotov/incendiary, HE, flashbang, and decoy projectiles.

Latest source/release package in this repository: `v0.3.3`.

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

These DLLs are built with the MSVC runtime statically linked. Users do not need to install the Visual C++ Redistributable just to load the DLL with HLAE.

Local native builds write the latest intermediate DLL to `bin/selfiestick_hlae.dll`; the packaged bilingual DLLs are copied into `release/en-US` and `release/zh-CN` by `build_dll.bat`.

## Quick Start

1. Start HLAE `2.190.2` and launch CS2.
2. Open or play a demo in spectator mode.
3. Load one DLL from the HLAE console.
4. Press `Ins` to open the panel.
5. Press `F8` or click the panel button to enable the camera.
6. Recommended for Prop Selfie tuning: set demo playback to `0.1x` first, then raise speed after the camera framing is stable.

English DLL:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

Chinese DLL:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\zh-CN\Lyan_CS2自拍杆.dll"
```

If HLAE prints `mirv_loadlibrary failed`, fully restart HLAE/CS2 and verify the path is correct. Current `v0.3.3` DLLs no longer depend on `MSVCP140.dll`, `VCRUNTIME140.dll`, the UCRT `api-ms-win-crt-*` DLLs, or a load-time `D3DCOMPILER_47.dll`, so a missing VC++ runtime should not block loading.

For slow-motion setup, use the CS2 demo UI / HLAE playback controls, or run this in the game console when available:

```cfg
demo_timescale 0.1
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
- `Player R / B / U`: local position trim for Right / Back / Up, editable by numeric input or by the labeled `RIGHT`, `BACK`, and `UP` sliders.
- `Player Pitch / Yaw / Roll`: local angle trim on top of the existing player selfie view, editable by numeric input or by the labeled `PITCH`, `YAW`, and `ROLL` sliders.

The built-in player framing is applied first. `R / B / U` and `Pitch / Yaw / Roll` are extra local trim values on top of that framing, so Selfie Left, Selfie Right, Forward, Follow, and Lock Current keep their existing behavior.

## Prop Selfie Mode

Use `Camera Mode -> Prop Selfie` to follow projectiles thrown by the current target agent.

Recommended workflow: play the demo at `0.1x`, lock the target agent, throw or scrub to the projectile moment, let the unique projectile auto-lock or choose one from Recent Props, then tune `X / Y / Z` and `Pitch / Yaw / Roll`. `0.1x` gives the smoothest setup window and makes it easier to keep fast projectiles centered.

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
- `X`, `Y`, `Z` sliders: medium-sensitivity prop position sliders in the `-64` to `64` range for quick anti-clipping adjustments.
- `Pitch / Yaw / Roll`: local view angle trim, editable by numeric input or by the labeled angle sliders.
- Angle sliders: medium-sensitivity `Pitch`, `Yaw`, and `Roll` sliders in the `-45` to `45` degree range.
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

The validation project checks compatibility gates, schema helpers, SetUpView patch helpers, prop projectile classification, owner matching, auto-lock policy, prop camera math, player angle trim, and camera smoothing helpers.

Release dependency check:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_dll_dependencies.ps1 -DllPath "release\en-US\Lyan's selfiestick.dll"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_dll_dependencies.ps1 -DllPath "release\zh-CN\Lyan_CS2自拍杆.dll"
```

The check fails if a release DLL has load-time dependencies on VC++ runtime DLLs or `D3DCOMPILER_47.dll`.

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
