# CS2 Selfiestick Manual

## Package Contents

This English release folder contains:

- `Lyan's selfiestick.dll`
- `Manual.md`
- `selfiestick_bind.cfg`

Only the DLL is required at runtime. The manual and cfg are reference files.

The DLL is built with the MSVC runtime statically linked. You do not need to install the Visual C++ Redistributable just to load it with HLAE.

## Supported Versions

This package is marked for:

- HLAE: `2.192.6`
- Counter-Strike 2: Steam app `730`, buildid `25687242`

Use it for HLAE demo/cinematic workflows only.

## Load The DLL

1. Copy the `en-US` folder to a stable location.
2. Start HLAE `2.192.6`, launch CS2, and open a demo in spectator mode.
3. Run this in the HLAE console:

```cfg
mirv_loadlibrary "<absolute path to Lyan's selfiestick.dll>"
```

Example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

If loading succeeds, the console prints a selfiestick ready message. Press `Ins` to open the Win32/GDI pseudo-ImGui panel. It creates no D3D11 device or DXGI swap chain and makes no Present call, so it can coexist with ReShade injection.

If HLAE prints `mirv_loadlibrary failed`, fully restart HLAE/CS2 and verify the absolute path. Current release DLLs do not depend on `MSVCP140.dll`, `VCRUNTIME140.dll`, UCRT `api-ms-win-crt-*` DLLs, or any `D3DCOMPILER_*.dll`.

Current release DLLs also avoid heavy startup schema resolving and no longer install the SetUpView camera hotpatch during `mirv_loadlibrary`. The hotpatch is installed only when you press `F8` or click Enable. If HLAE 2.192.6 already patched the same site, the DLL chains HLAE's trampoline first and then runs the Selfiestick callback. If the current CS2 build is incompatible, the DLL stays loaded and shows a Runtime reason instead of taking the demo process down. Debug trace is off by default in the packaged `selfiestick.ini`; only enable `[Debug] TraceEnabled=1` when you need to send a diagnostic log.

If you already have an old `selfiestick.ini` next to the DLL, delete it or set `[Debug] TraceEnabled=0` before testing this release.

The current v0.4.3 DLL no longer imports `d3d11.dll` or `dxgi.dll`. If a new dump still passes through a ReShade-provided `dxgi.dll`, first verify that the loaded DLL is really v0.4.3, then inspect the remaining graphics injectors independently.

Recommended setup speed: use the CS2 demo UI / HLAE playback controls, or the game console command below when available, to tune Prop Selfie shots at `0.1x` speed:

```cfg
demo_timescale 0.1
```

## Hotkeys

- `Ins`: show / hide the panel by default
- `F8`: enable / disable Selfiestick
- `F9`: lock the current spectator target
- `F10`: return to Follow target mode

The panel hotkey can be changed in the panel and is saved to `selfiestick.ini` next to the loaded DLL.

## Player Selfie

Select `Player Selfie` in Camera Mode for the original player-mounted selfiestick.

Controls:

- `Selfie Left`: selfie composition from the target's left side
- `Selfie Right`: selfie composition from the target's right side
- `Forward`: forward-facing camera mode
- `Follow`: use the current spectator target
- `Lock Current`: lock the current target agent
- `Clear`: clear target lock
- `Player R / B / U`: Right / Back / Up local camera trim through native numeric fields
- `Player Pitch / Yaw / Roll`: local angle trim through native numeric fields

Player angle trim is applied after the normal Selfie Left, Selfie Right, or Forward view is calculated.

## Prop Selfie

Select `Prop Selfie` in Camera Mode to follow a projectile thrown by the current target agent.

Recommended workflow: set demo playback to `0.1x`, lock or follow the target agent, wait for the projectile candidate to appear, use auto-lock for a single projectile or `LOCK` for one candidate, then tune `X / Y / Z` and `Pitch / Yaw / Roll`. After framing is stable, raise demo speed as needed.

Supported projectiles:

- smoke
- molotov / incendiary
- HE grenade
- flashbang
- decoy

Behavior:

- The target agent is still controlled by `Follow` / `Lock Current`.
- The prop scanner only accepts projectiles thrown or owned by the current target.
- If there is exactly one valid flying projectile and no prop lock, it auto-locks.
- If multiple valid projectiles exist, choose one manually from Recent Props with `LOCK`.
- `CLEAR PROP` clears the current prop lock.
- The camera direction prefers the actual projectile thrower, so look-back direction stays correct even if the spectator target changes.
- Smoke projectile shots keep a short post-flight hold after the flying smoke entity disappears. Other projectile types end normally when the flying entity ends.
- Tracking combines the projectile's schema velocity direction with its observed movement rate, so slow motion and normal playback use the correct prediction speed. It stops extrapolating when playback pauses and resets prediction when the bounce counter changes; the guarded measured-motion fallback remains active if either field is unavailable.
- Projectile discovery is split into bounded batches, while the locked projectile's position uses the direct scene-node origin path on the supported build to avoid a full dynamic-origin search on every camera callback.

Prop controls:

- `Prop X / Y / Z`: local prop camera offset through native numeric fields
- `Pitch / Yaw / Roll`: angle trim through native numeric fields
- `IMAGE STYLE`: writes the shared screenshot-style prop preset: `X=-8`, `Y=-22`, `Z=4`, `Pitch=1`, `Yaw=0`, `Roll=0`
- `Recent Props`: candidate list with validity, handle, thrower handle, age, type, and lock state

## Failure States

The Runtime diagnostics bar is collapsed by default. Click it to expand the live state when the camera is blocked. Common examples:

- no observer target
- target unresolved
- prop not locked
- prop ended
- schema or SetUpView compatibility failure

Trace logging is off by default. To collect diagnostics, set `[Debug] TraceEnabled=1` in `selfiestick.ini` next to the DLL and fully restart HLAE/CS2. The DLL then writes `selfiestick_trace.log` next to the loaded DLL.

If you replace the DLL, fully restart HLAE and CS2 before testing again.
