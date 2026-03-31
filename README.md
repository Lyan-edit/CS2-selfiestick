# CS2 Selfiestick

`CS2-selfiestick` is a Counter-Strike 2 spectator camera DLL for `HLAE`.

It gives you a standalone control panel and a gun-mounted selfie camera workflow for cinematic spectator shots in CS2.

## Highlights

- DLL-based workflow for `HLAE`
- Standalone movable and resizable control panel
- `FOLLOW / LOCK CURRENT / CLEAR` target control
- `SELFIE LEFT / SELFIE RIGHT / FORWARD` look modes
- Local trim controls: `R / B / U`
- Persisted panel hotkey via `selfiestick.ini`
- Gun camera shake damping while firing
- Preserves unscoped framing and FOV while spectating an `AWP`

## Current Supported Weapons

- `AK-47`
- `M4A4`
- `M4A1-S`
- `AWP`

## Download

- Source code: this repository
- Prebuilt binaries: GitHub Releases

If you only want to use the DLL, download the latest release package instead of building from source.

## Quick Start

1. Download a release package.
2. Copy either `zh-CN` or `en-US` to a stable location.
3. Start `HLAE` and enter `CS2`.
4. Load the DLL in the HLAE console.

Chinese DLL example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\zh-CN\Lyan_CS2自拍杆.dll"
```

English DLL example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

Default hotkeys:

- `Ins`: show / hide panel
- `F8`: enable / disable selfiestick
- `F9`: lock current target
- `F10`: return to follow mode

## Build From Source

Requirements:

- Visual Studio 2022 with C++ toolchain
- `MSBuild`

Build both localized release packages:

```bat
build_dll.bat
```

Main solution:

```text
native_dll/selfiestick_hlae.sln
```

## Repository Layout

- `native_dll/` - Visual Studio solution and C++ source code
- `build_dll.bat` - build script for localized release packages
- `使用说明.md` - Chinese project notes and usage details
- `release/zh-CN/操作手册.md` - Chinese end-user manual
- `release/en-US/Manual.md` - English end-user manual
- `selfiestick_bind.cfg` - hotkey reference

## Documentation

- Chinese notes: [使用说明.md](./使用说明.md)
- Chinese manual: [release/zh-CN/操作手册.md](./release/zh-CN/操作手册.md)
- English manual: [release/en-US/Manual.md](./release/en-US/Manual.md)

## Source Notes

- This repository tracks source code, project files, and documentation.
- Compiled DLLs, PDBs, zip packages, and Visual Studio intermediates are intentionally excluded from Git.
- The current runtime implementation is centered in:

```text
native_dll/selfiestick_hlae/selfiestick_hlae.cpp
```

## License

This project is licensed under the MIT License. See [LICENSE](./LICENSE).
