# CS2 Selfiestick DLL

`selfiestick` is a Counter-Strike 2 spectator camera DLL for `HLAE`.

This repository is organized to keep the **source code and documentation** in Git, while excluding generated binaries and intermediate build output.

## What It Does

- Injects as a DLL through `HLAE`
- Opens a standalone movable / resizable control panel
- Supports `FOLLOW / LOCK CURRENT / CLEAR`
- Supports `SELFIE LEFT / SELFIE RIGHT / FORWARD`
- Supports local trim controls: `R / B / U`
- Saves the panel toggle key to `selfiestick.ini`
- Stabilizes gun-mounted camera shake while firing
- Preserves the original FOV while spectating a scoped `AWP`

## Current Supported Weapons

- `AK-47`
- `M4A4`
- `M4A1-S`
- `AWP`

## Repository Layout

- `native_dll/`
  Visual Studio solution and C++ source code
- `build_dll.bat`
  Release build script
- `使用说明.md`
  Main Chinese development / usage notes
- `release/zh-CN/操作手册.md`
  Chinese end-user manual
- `release/en-US/Manual.md`
  English end-user manual
- `selfiestick_bind.cfg`
  Reference hotkey hints

## Build

Requirements:

- Visual Studio 2022 with C++ toolchain
- `MSBuild`

Build both localized release packages:

```bat
build_dll.bat
```

The Visual Studio solution is:

```text
native_dll/selfiestick_hlae.sln
```

## Load In HLAE

Example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\zh-CN\Lyan_CS2自拍杆.dll"
```

or:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

## Documentation

- Chinese notes: [使用说明.md](./使用说明.md)
- Chinese manual: [release/zh-CN/操作手册.md](./release/zh-CN/操作手册.md)
- English manual: [release/en-US/Manual.md](./release/en-US/Manual.md)

## Notes

- This repo intentionally does **not** track compiled DLLs, PDBs, zip packages, or Visual Studio intermediate files.
- The runtime implementation is currently centered in:

```text
native_dll/selfiestick_hlae/selfiestick_hlae.cpp
```
