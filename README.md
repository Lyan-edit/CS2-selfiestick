# CS2 Selfiestick

CS2 Selfiestick is a Counter-Strike 2 spectator camera DLL for HLAE. It provides a ReShade-safe Win32/GDI control panel styled like ImGui and two camera modes for cinematic demo work:

- Player Selfie: the original player-mounted selfiestick.
- Prop Selfie: a projectile-mounted selfiestick for smoke, molotov/incendiary, HE, flashbang, and decoy projectiles.

Latest source/release package in this repository: `v0.4.4`.

## Supported Versions

This build is marked for:

- HLAE: `2.192.10` (AfxHookSource2 `0.41.9`)
- Counter-Strike 2: Steam app `730`, buildid `26144732`
- CS2 app manifest timestamp used locally: `2026-10-23 10:15:51 +08:00`

CS2 and HLAE update often. If Valve or HLAE changes schema, entity layout, or `SetUpView`, load may fail by design and the panel will show a reason instead of silently using an unsafe fallback.

## Download

The repository tracks current release DLLs here:

```text
release/en-US/Lyan's selfiestick.dll
release/zh-CN/Lyan_CS2自拍杆.dll
```

The GitHub Release page also provides ASCII-named copies for easier downloading on every browser:

```text
Lyan_selfiestick_en-US.dll
Lyan_CS2_selfiestick_zh-CN.dll
```

These DLLs are built with the MSVC runtime statically linked. Users do not need to install the Visual C++ Redistributable just to load the DLL with HLAE.

Local native builds write the latest intermediate DLL to `bin/selfiestick_hlae.dll`; the packaged bilingual DLLs are copied into `release/en-US` and `release/zh-CN` by `build_dll.bat`.

## Quick Start

1. Start HLAE `2.192.10` and launch CS2.
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

If HLAE prints `mirv_loadlibrary failed`, fully restart HLAE/CS2 and verify the path is correct. Current `v0.4.4` DLLs no longer depend on `MSVCP140.dll`, `VCRUNTIME140.dll`, UCRT `api-ms-win-crt-*` DLLs, `D3DCOMPILER_*.dll`, `D3D11.dll`, or `DXGI.dll`.

If the console prints `LoadLibraryA Ok.` followed by `SetUpView hook blocked by compatibility gate`, the DLL did load, but the compatibility gate rejected hook installation. Use `v0.4.4` with CS2 buildid `26144732`, or rebuild after validating a newer game binary.

If the DLL is loaded while a demo is already playing, `v0.4.4` installs the SetUpView camera hotpatch lazily only when the user presses `F8` or clicks Enable. HLAE 2.192.10 / AfxHookSource2 0.41.9 patches the same 16-byte site before a later `mirv_loadlibrary`; this build recognizes that HLAE trampoline, replays it first, preserves its `IsPlayingDemo` result, and then runs the Selfiestick camera callback instead of rejecting or bypassing HLAE's patch. The `Ins` panel is also created lazily, but uses only native Win32 controls and GDI drawing. It does not create a D3D device, DXGI swap chain, or Present call, so it does not enter ReShade's graphics-hook chain. Schema resolution starts with exact class lookup instead of a broad raw payload scan. The flatter nSkinz-inspired panel keeps Runtime diagnostics collapsed by default and repaints only after state changes. Player mode reuses the last trusted follow target between 50 ms refreshes and caches attachment indices. Prop mode spreads entity discovery across bounded 192-entity batches, limits an in-progress scan to one batch per 8 ms, and normally reads projectile position directly from `CGameSceneNode::m_vecAbsOrigin` instead of running the expensive dynamic-origin search every camera callback. A failed hook attempt leaves the DLL loaded with a Runtime reason instead of crashing or freezing the demo process. Trace logging remains off by default.

If you previously used an older build that created `selfiestick.ini`, delete that old ini or make sure `[Debug] TraceEnabled=0`; an existing ini value overrides the new default.

For slow-motion setup, use the CS2 demo UI / HLAE playback controls, or run this in the game console when available:

```cfg
demo_timescale 0.1
```

## Hotkeys

| Key | Action |
| --- | --- |
| `Ins` | Show or hide the native pseudo-ImGui panel |
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
- `Player R / B / U`: local position trim for Right / Back / Up, editable in native numeric fields.
- `Player Pitch / Yaw / Roll`: local angle trim on top of the existing player selfie view, editable in native numeric fields.

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
- `X / Y / Z`: native numeric fields for local prop camera position.
- `Pitch / Yaw / Roll`: native numeric fields for local view angle trim.
- Recent Props: shows recent projectile candidates, validity, handle, age, and lock buttons.

The prop camera uses a smoothed, predicted projectile anchor to reduce stepping and jitter across slow motion and normal demo speed. It combines the current schema velocity direction (`C_BaseEntity::m_vecVelocity`) with the observed movement rate so prediction respects demo playback speed, falls back to measured position deltas when unavailable, stops extrapolating when demo motion pauses, and resets stale prediction immediately when `C_BaseCSGrenadeProjectile::m_nBounces` changes.

## Status And Debugging

The Runtime diagnostics bar is collapsed by default. Click it to expand the following live state:

- enabled state
- camera mode
- look mode
- panel hotkey
- target mode
- locked/current target handles
- prop lock and candidate list
- last override result
- failure reason

Debug trace is off by default. To collect diagnostics, set this in `selfiestick.ini` next to the loaded DLL, then fully restart HLAE/CS2:

```ini
[Debug]
TraceEnabled=1
```

When enabled, debug trace is written next to the loaded DLL:

```text
selfiestick_trace.log
```

The current trace marker is:

```text
schema-probe-v27-cs2-26144732
```

If the marker is older, HLAE/CS2 is loading an old DLL. Fully restart HLAE and CS2 after replacing DLLs because Windows keeps loaded DLLs inside the process.

The `v0.4.4` panel deliberately has no `D3D11.dll` or `DXGI.dll` import and does not call Present. If a new dump still points through a ReShade-provided `dxgi.dll`, verify that the trace marker and DLL hash belong to `v0.4.4`, then inspect the remaining graphics injectors independently.

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

Installed-CS2 binary compatibility check:

```powershell
py -3 tools\check_cs2_binary_compatibility.py --client "<CS2>\game\csgo\bin\win64\client.dll" --schema "<CS2>\game\bin\win64\schemasystem.dll"
```

This check validates the PE fingerprint, signature match counts, attachment call targets, the SetUpView 16-byte instruction boundary, and the `RBX` origin/angle context used by entity, attachment, schema, and camera access.

Release dependency check:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_dll_dependencies.ps1 -DllPath "release\en-US\Lyan's selfiestick.dll"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_dll_dependencies.ps1 -DllPath "release\zh-CN\Lyan_CS2自拍杆.dll"
```

The check fails if a release DLL has load-time dependencies on VC++ runtime DLLs, `D3DCOMPILER_*.dll`, `D3D11.dll`, or `DXGI.dll`, or imports `D3DCompile`.

Do not run `npm run build` during agent sessions for this repository. The native DLL flow is independent of the Next.js app guidance.

## Release Packaging

The release packager follows the same five-asset layout used by nSkinz: English DLL, Chinese DLL, bilingual Windows ZIP, complete source ZIP, and `SHA256SUMS.txt`.

Use PowerShell 7 (`pwsh`) for the release scripts:

```powershell
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File tools\New-SelfiestickRelease.ps1 -Version v0.4.4 -Cs2Build 26144732 -HlaeVersion 2.192.10
```

After committing and pushing the matching tag, the publisher creates a draft GitHub Release, uploads all five assets through the GitHub REST API, verifies remote byte sizes and SHA-256 hashes, and only then publishes it as the latest release. Authentication is read from Git Credential Manager and is never printed.

```powershell
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File tools\Publish-GitHubRelease.ps1 -Version v0.4.4
```

## Repository Layout

```text
native_dll/selfiestick_hlae/             Main DLL source and Visual Studio project
native_dll/selfiestick_hlae_validation/  Lightweight native validation executable
native_dll/third_party/imgui/            Legacy vendored source retained for history; not built into v0.4.4
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
