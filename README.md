# CS2 Selfiestick

CS2 Selfiestick is a Counter-Strike 2 spectator camera DLL for HLAE. It adds a small in-game control panel and camera override for cinematic demo work, including left/right selfie views, forward view, target locking, and local camera trim.

Latest release: [v0.2.0](https://github.com/Lyan-edit/CS2-selfiestick/releases/tag/v0.2.0)

## What It Does

- Works as an HLAE-loaded DLL inside CS2.
- Provides a movable control panel, opened with `Ins` by default.
- Supports `Selfie Left`, `Selfie Right`, and `Forward` view modes.
- Supports `Follow`, `Lock Current`, and `Clear` target control.
- Provides local camera trim through `R / B / U`.
- Keeps a debug trace in `selfiestick_trace.log` next to the loaded DLL.
- Includes English and Chinese DLL builds.
- Current build trace marker: `schema-probe-v20-left-selfie-offset-tune`.

## Download

Download the latest DLLs from GitHub Releases:

- English: `Lyan-selfiestick-en-US.dll`
- Chinese: `Lyan-CS2-selfiestick-zh-CN.dll`

Release page:

```text
https://github.com/Lyan-edit/CS2-selfiestick/releases/latest
```

The repository also tracks the current release DLLs here:

```text
release/en-US/Lyan's selfiestick.dll
release/zh-CN/Lyan_CS2自拍杆.dll
```

## Quick Start

1. Start HLAE and launch CS2.
2. Open or play a demo in a spectator context.
3. Load one DLL from the HLAE console.

English DLL example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\Lyan-selfiestick-en-US.dll"
```

Chinese DLL example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\Lyan-CS2-selfiestick-zh-CN.dll"
```

After loading, open the panel with `Ins`, then enable Selfiestick from the panel or with `F8`.

## Hotkeys

| Key | Action |
| --- | --- |
| `Ins` | Show or hide the control panel |
| `F8` | Enable or disable Selfiestick |
| `F9` | Lock the current spectator target |
| `F10` | Return to follow mode |

`Ins` can be changed in the panel. The setting is saved to `selfiestick.ini` next to the loaded DLL.

## Camera Controls

The panel exposes three local trim values:

- `R`: right offset in the camera basis
- `B`: backward offset in the camera basis
- `U`: upward offset in the camera basis

The default built-in framing is applied first; panel values are trim values on top of that framing.

The v0.2.0 build includes the latest left-side selfie tuning:

- left selfie `R`: original value + 15
- left selfie `B`: original value + 10
- left selfie `U`: unchanged

## Current Compatibility

This project is maintained against the current CS2/HLAE demo workflow. Recent CS2 updates changed schema and view setup behavior, so the DLL now includes compatibility probing and fallback logic for:

- schema field discovery
- SetUpView hook discovery
- spectator follow target resolution
- attachment-unavailable fallback basis
- viewmodel suppression during active camera override

If the UI is red or the camera does not override, check `selfiestick_trace.log`. The first lines should include:

```text
schema-probe-v20-left-selfie-offset-tune
```

If the trace marker is older, HLAE/CS2 is still loading an old DLL.

## Build From Source

Requirements:

- Visual Studio 2022
- MSVC C++ toolchain
- Windows SDK

Main solution:

```text
native_dll/selfiestick_hlae.sln
```

Build script:

```bat
build_dll.bat
```

The validation project is included in the solution:

```text
native_dll/selfiestick_hlae_validation/
```

It checks the compatibility gate, schema probing helpers, SetUpView patch helpers, viewmodel suppression conditions, and the current left selfie offset rule.

## Repository Layout

```text
native_dll/selfiestick_hlae/             Main DLL source and Visual Studio project
native_dll/selfiestick_hlae_validation/  Lightweight validation executable
release/en-US/                           English release DLL and manual
release/zh-CN/                           Chinese release DLL and manual
使用说明.md                              Chinese development and usage notes
selfiestick_bind.cfg                     Hotkey reference
```

## Notes

- This DLL is intended for HLAE/demo/cinematic workflows.
- It is not a gameplay cheat and is not intended for matchmaking or anti-cheat protected live play.
- Fully restart HLAE/CS2 after replacing the DLL; Windows will otherwise keep the old DLL loaded in the process.

## License

MIT License. See [LICENSE](./LICENSE).
