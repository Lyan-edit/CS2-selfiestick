# CS2 Selfiestick

CS2 Selfiestick is a Counter-Strike 2 spectator camera DLL for HLAE. It provides a ReShade-safe Dear ImGui control panel rendered by a custom CPU/GDI software backend and two camera modes for cinematic demo work:

- Player Selfie: the original player-mounted selfiestick.
- Prop Selfie: a projectile-mounted selfiestick for smoke, molotov/incendiary, HE, flashbang, and decoy projectiles.

Current release: [v0.4.6](https://github.com/Lyan-edit/CS2-selfiestick/releases/tag/v0.4.6) (regular GitHub Release / Latest).

Version 0.4.6 includes the user-confirmed shake fix and the two-demo-second automatic effect hold followed by native-agent return. Resolved-origin sampling, rigid framing, birth capture and per-shot FOV are preserved. The [stable baseline](docs/2026-11-08-confirmed-stable-fix5.md), [lifecycle update](docs/2026-11-08-prop-hold-fix6.md) and [release validation](docs/2026-11-08-v0.4.6-release-validation.md) record the changes and checks.

Player Selfie binds to the agent's `weapon_hand_r` skeleton attachment for every held item. The active weapon is optional FOV/viewmodel metadata. Missing hand transforms use a short pawn-local cache, then a body-relative hand position; camera facing comes from the pawn, not an unrelated observer view. Exact handle serial validation rejects reused entity slots, and weapon restoration never falls back to an unvalidated saved pointer. Framing v6 backs up old settings to `selfiestick.ini.pre-hand-anchor.bak` before resetting old player trim once; prop trim is retained. These fixes are included in v0.4.5; see [the hand-anchor investigation](docs/2026-11-06-hand-anchor-fix.md).

The pawn render-eye function uses verified vtable slot 173 (also used by HLAE 2.192.10). The previous slot 166 forwarded to a zero-return stub without writing the output, which could place the camera near world origin. Version v0.4.5 uses NaN sentinels, validates eye coordinates against the selected pawn's scene origin, rejects uninitialized actor poses and guards non-finite final writes. The offline binary checker verifies the pawn RTTI/vtable and eye-output path. See [the eye-origin investigation](docs/2026-11-07-eye-origin-fix.md). This corrects world placement; automatic map-geometry collision avoidance is not implemented. Validation remains background/offline only.

## Supported Versions

This build is marked for:

- HLAE: `2.192.10` (AfxHookSource2 `0.41.9`)
- Counter-Strike 2: Steam app `730`, buildid `26445667`
- CS2 app manifest LastUpdated: `2026-11-05 20:10:37 +08:00`

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

If HLAE prints `mirv_loadlibrary failed`, fully restart HLAE/CS2 and verify the path is correct. Current `v0.4.6` DLLs do not depend on `MSVCP140.dll`, `VCRUNTIME140.dll`, UCRT `api-ms-win-crt-*` DLLs, `D3DCOMPILER_*.dll`, `D3D11.dll`, or `DXGI.dll`.

If the console prints `LoadLibraryA Ok.` followed by `SetUpView hook blocked by compatibility gate`, the DLL did load, but the compatibility gate rejected hook installation. Use this build with CS2 buildid `26445667`, or rebuild after validating a newer game binary.

If the DLL is loaded while a demo is already playing, `v0.4.6` installs the SetUpView camera hotpatch lazily only when the user presses `F8` or clicks Enable. HLAE 2.192.10 / AfxHookSource2 0.41.9 patches the same 16-byte site before a later `mirv_loadlibrary`; this build recognizes that HLAE trampoline, replays it first, preserves its `IsPlayingDemo` result, and then runs the Selfiestick camera callback instead of rejecting or bypassing HLAE's patch. The `Ins` panel is created lazily and now uses genuine Dear ImGui widgets with a custom CPU/GDI software renderer. It creates no D3D device, DXGI swap chain, OpenGL context, or Present call, so it stays outside ReShade's graphics-hook chain. The renderer redraws a complete off-screen frame before copying it to the panel, eliminating native Trackbar trail/smear artifacts. Schema resolution uses only the offset table tied to the exact supported client PE fingerprint; unknown builds fail closed without probing TypeScope virtual functions. Player mode reuses the last trusted follow target between 50 ms refreshes and caches attachment indices. Prop mode uses a resumable all-slot sweep with cached vtable kinds and a 0.5 ms soft scan budget on each active callback, and reads projectile position through the verified engine absolute-origin accessor, which resolves dirty scene transforms before returning the vector. A failed hook attempt leaves the DLL loaded with a Runtime reason instead of crashing or freezing the demo process. Trace logging remains off by default.

If you previously used an older build that created `selfiestick.ini`, delete that old ini or make sure `[Debug] TraceEnabled=0`; an existing ini value overrides the new default.

For slow-motion setup, use the CS2 demo UI / HLAE playback controls, or run this in the game console when available:

```cfg
demo_timescale 0.1
```

## Hotkeys

| Key | Action |
| --- | --- |
| `Ins` | Show or hide the Dear ImGui software-rendered panel |
| `F8` | Enable or disable Selfiestick |
| `F9` | Lock the current spectator target |
| `F10` | Return target mode to Follow |

The panel hotkey can be changed inside the panel. The setting is saved to `selfiestick.ini` next to the loaded DLL.

## Player Selfie Mode

Use `Camera Mode -> Player Selfie` for the original selfiestick behavior.

Controls:

- `Selfie Left`: selfie-facing composition from the target's left side.
- `Selfie Right`: selfie-facing composition from the target's right side.
- `Over Shoulder`: right-shoulder forward view; the agent stays in the left foreground like a third-person shoulder camera.
- `Apply Over-Shoulder Preset`: switches to Player Selfie, clears the extra trim, and applies the built-in shoulder framing.
- `Follow`: use the current spectator target each frame.
- `Lock Current`: lock the current target agent.
- `Clear`: clear the locked target.
- `Player R / B / U`: local position trim for Right / Back / Up, editable in native numeric fields or the matching 0.25-step sliders.
- `Player Pitch / Yaw / Roll`: local angle trim on top of the existing player selfie view, editable in native numeric fields or sliders.

The default left/right selfie camera is placed in front of the gun muzzle or knife and looks back at the agent's centered upper body. The old cross-frame target bias is removed. The built-in player framing is applied first; `R / B / U` and `Pitch / Yaw / Roll` remain extra local trim values, and left/right stay symmetric.

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
- `AUTO HANDOFF` follows through detonation, holds the final view for **2 demo seconds**, then returns to the agent's native view while staying armed for future throws. Pause stops this timer; slow playback extends its wall-clock duration. A newer valid throw takes over immediately, including during the hold.
- `PIN CURRENT PROP` follows the currently selected shot through detonation and retains its final view indefinitely, ignoring other throws. A row `SELECT + PIN` chooses a specific live candidate and applies the same pin policy. AUTO HANDOFF unpins; if two demo seconds have already elapsed, it returns to the agent unless another shot takes over.
- `CLEAR LOCK / RETURN TO AGENT` immediately releases the view and suppresses automatic recapture of the cleared/existing candidates. Future throws still trigger auto capture. The lower target `Clear Lock` also releases the prop view and returns target selection to Follow. Explicit row selection can reopen a live cleared candidate.
- Landing and scheduled explosion/smoke ticks do not stop tracking. Actual effect flags or entity removal start the final-view hold while game effects continue. A projectile born and destroyed entirely between camera callbacks still requires slower playback.

Prop controls:

- `Prop X / Y / Z`: native numeric fields plus 0.25-step sliders for local prop camera position.
- `Pitch / Yaw / Roll`: native numeric fields plus sliders for local view angle trim.
- `Centered Lookback Preset`: places the camera directly on the outgoing trajectory axis: `X=0`, `Y=-13.5`, `Z=2.25`, `Pitch=0`, `Yaw=0`, `Roll=0`.
- Recent Props: shows recent projectile candidates, validity, handle, age, and lock buttons.

The prop camera latches the first valid inverse horizontal launch yaw for that exact handle. A temporary fallback can be promoted once when initial velocity arrives; later updates cannot rotate the established rig. Both position and aim use the same current scene sample, so `sqrt(X*X + Y*Y + Z*Z)` is the prop-relative distance, independent of playback speed, FPS or refresh. Legacy `PropFollowSpeed` is retained in INI files but no longer limits the carrier. Guessed memory-vector origin searches remain removed. Temporarily missing data keeps the last validated view while allowing recovery; a pending new shot takes over only when its pose is usable. Creation observation preserves the original engine/HLAE call chain; complete 0..32767 slot coverage remains the fallback if observation cannot be installed.

## Status And Debugging

For same-shot motion problems, expand Runtime and select `EXPORT MOTION DIAGNOSTICS`. It explicitly exports the most recent 1,024 in-memory camera samples to `selfiestick_prop_motion.csv` beside the loaded DLL. The file records build/marker, selected/displayed handles, phases, cached/resolved origins, camera pose and FOV. Camera callbacks perform no diagnostic file writes; export uses a worker. These are callback samples, not a measured game FPS trace.

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
schema-probe-v42-prop-origin-cs2-26445667
```

If the marker is older, HLAE/CS2 is loading an old DLL. Fully restart HLAE and CS2 after replacing DLLs because Windows keeps loaded DLLs inside the process.

The `v0.4.6` panel has no `D3D11.dll` or `DXGI.dll` import and does not call Present. If a new dump still points through a ReShade-provided `dxgi.dll`, verify that the build label and DLL hash belong to `v0.4.6`, then inspect the remaining graphics injectors independently. The diagnostic marker above identifies the retained origin implementation; the build label identifies the release version.

## Build From Source

Requirements:

- Visual Studio 2022
- MSVC C++ toolchain
- Windows SDK

PowerShell 7 build (static MSVC runtime, no PDB compiler dependency):

```powershell
pwsh -NoProfile -File tools/Build-Native.ps1 -Target Dll -Language zh-CN
pwsh -NoProfile -File tools/Build-Native.ps1 -Target Dll -Language en-US
pwsh -NoProfile -File tools/Build-Native.ps1 -Target Validation
pwsh -NoProfile -File tools/Build-Native.ps1 -Target UiPreview -Language zh-CN
```

Outputs are under `native_dll/bin/native-<Target>-<Language>/`. The UI preview executable renders the production player/prop panel in a hidden standalone process and checks default ImGui styling, slider-frame cleanup and shared-edge rasterization. Run it with a writable working directory for its BMP captures. It never starts the game or installs hooks. See [the background validation report](docs/2026-11-01-validation.md).

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

This check validates the PE fingerprint, signature match counts, attachment call targets, the SetUpView 16-byte instruction boundary, the `RBX` origin/angle context, and all 22 build-gated fields against the PE-resident static schema tables. It does not load the game DLL or call schema interfaces.

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
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File tools\New-SelfiestickRelease.ps1 -Version v0.4.5 -Cs2Build 26445667 -HlaeVersion 2.192.10
```

After committing and pushing the matching tag, the publisher creates a draft GitHub Release, uploads all five assets through the GitHub REST API, verifies remote byte sizes and SHA-256 hashes, and only then publishes it. Beta versions are marked as GitHub pre-releases; the stable latest release remains unchanged. Authentication is read from Git Credential Manager and is never printed.

```powershell
pwsh -NoLogo -NoProfile -ExecutionPolicy Bypass -File tools\Publish-GitHubRelease.ps1 -Version v0.4.5
```

## Repository Layout

```text
native_dll/selfiestick_hlae/             Main DLL source and Visual Studio project
native_dll/selfiestick_hlae_validation/  Lightweight native validation executable
native_dll/third_party/imgui/            Dear ImGui 1.92 core and Win32 platform backend used by the software-rendered panel
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
