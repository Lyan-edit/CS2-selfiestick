# CS2 Selfiestick Manual

This folder contains release `v0.4.7`, including the player-final-view-fix4 implementation. The player camera now uses a fixed agent-body frame, independent of hand, weapon and render-eye animation. The render-eye call uses verified vtable slot 173. The body origin is engine-resolved; verified movement-service duck amount controls stance height. Locomotion and turning still update every callback. Temporary scene/weapon metadata gaps retain validated framing, FOV and model suppression. Local yaw now has a short demo-rate-aware transition that freezes on pause. Body position and clearance remain unchanged. Player motion diagnostics export selfiestick_player_motion.csv with raw/filtered yaw and estimated demo rate. Framing v6 backs up older settings as `selfiestick.ini.pre-hand-anchor.bak` before resetting player trim once; prop settings remain. The user confirmed that the fix5 prop-origin update resolved shaking; v0.4.7 retains that implementation. The new view lifecycle has background/offline validation only. Automatic map collision avoidance is not implemented.

v0.4.7 applies the player pose at the verified final stage of native SetUpView, after native position/angle/FOV processing. Props retain their previous callback path. Player FOV is captured for each enable/target session and shared across callback threads; gun/knife switches cannot recapture it. Toggle Player Selfie off and on to adopt a new HLAE FOV. The player CSV now has 34 columns, adding native input FOV, thread ID and early/final callback stage. Fully restart HLAE/CS2 and load one updated DLL when upgrading.

## Package Contents

This English release folder contains:

- `Lyan's selfiestick.dll`
- `Manual.md`
- `selfiestick_bind.cfg`

Only the DLL is required at runtime. The manual and cfg are reference files.

The DLL is built with the MSVC runtime statically linked. You do not need to install the Visual C++ Redistributable just to load it with HLAE.

## Supported Versions

This package targets the versions below. HLAE 2.193.0 was statically checked; no new live-demo validation was performed:

- HLAE: `2.193.0` (AfxHookSource2 `0.42.0`)
- Counter-Strike 2: Steam app `730`, buildid `26445667`

Use it for HLAE demo/cinematic workflows only.

## Load The DLL

1. Copy the `en-US` folder to a stable location.
2. Start HLAE `2.193.0`, launch CS2, and open a demo in spectator mode.
3. Run this in the HLAE console:

```cfg
mirv_loadlibrary "<absolute path to Lyan's selfiestick.dll>"
```

Example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

If loading succeeds, the console prints a selfiestick ready message. Press `Ins` to open the genuine Dear ImGui panel. Its custom CPU/GDI software renderer creates no D3D11 device, DXGI swap chain, OpenGL context, or Present call, so it can coexist with ReShade injection. Every frame is composed off-screen before display, preventing slider trail/smear artifacts.

If HLAE prints `mirv_loadlibrary failed`, fully restart HLAE/CS2 and verify the absolute path. Current release DLLs do not depend on `MSVCP140.dll`, `VCRUNTIME140.dll`, UCRT `api-ms-win-crt-*` DLLs, or any `D3DCOMPILER_*.dll`.

Current release DLLs also avoid heavy startup schema resolving and no longer install the SetUpView camera hotpatch during `mirv_loadlibrary`. The hotpatch is installed only when you press `F8` or click Enable. If HLAE 2.193.0 / AfxHookSource2 0.42.0 already patched the same site, the DLL chains HLAE's trampoline first and then runs the Selfiestick callback. If the current CS2 build is incompatible, the DLL stays loaded and shows a Runtime reason instead of taking the demo process down. Debug trace is off by default in the packaged `selfiestick.ini`; only enable `[Debug] TraceEnabled=1` when you need to send a diagnostic log.

If you already have an old `selfiestick.ini` next to the DLL, delete it or set `[Debug] TraceEnabled=0` before testing this release.

The current v0.4.7 DLL no longer imports `d3d11.dll` or `dxgi.dll`. If a new dump still passes through a ReShade-provided `dxgi.dll`, first verify that the loaded DLL is really v0.4.7, then inspect the remaining graphics injectors independently.

Recommended setup speed: use the CS2 demo UI / HLAE playback controls, or the game console command below when available, to tune Prop Selfie shots at `0.1x` speed:

```cfg
demo_timescale 0.1
```

## Hotkeys

The panel uses upstream Dear ImGui Dark styling. All twelve position/rotation parameters have standard sliders and numeric inputs with 0.25-step buttons. Runtime uses a standard collapsing header. The default player selfie aims from the weapon front toward the agent's upper-body center; use CENTERED PLAYER SELFIE to clear trim. Fully restart HLAE/CS2 after replacing a DLL that was already loaded.

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
- `Over Shoulder`: right-shoulder forward view with the agent in the left foreground
- `Apply Over-Shoulder Preset`: selects Player Selfie, clears extra trim, and applies the shoulder framing
- `Follow`: use the current spectator target
- `Lock Current`: lock the current target agent
- `Clear`: clear target lock
- `Player R / B / U`: Right / Back / Up local camera trim through native numeric fields and 0.25-step sliders
- `Player Pitch / Yaw / Roll`: local angle trim through native numeric fields and sliders

The default selfie uses the agent's right-hand skeleton attachment and looks back toward its upper-body center. Its geometry is independent of the gun/knife entity. Left and right are symmetric. `CENTERED PLAYER SELFIE` clears player trim. Framing v6 backs up old settings and resets muzzle-relative player trim once; prop trim remains. Player angle trim is applied after Selfie Left, Selfie Right, or Over Shoulder is calculated.

## Prop Selfie

Select `Prop Selfie` in Camera Mode to follow a projectile thrown by the current target agent.

Recommended workflow: enable Prop Selfie before the throw and follow or lock the target agent. New throws are captured automatically; use `LOCK` in Recent Props to pin one projectile. Tune `X / Y / Z` and `Pitch / Yaw / Roll` at `0.1x`, then raise demo speed after framing is stable.

Supported projectiles:

- smoke
- molotov / incendiary
- HE grenade
- flashbang
- decoy

Behavior:

- The target agent is still controlled by `Follow` / `Lock Current`.
- The prop scanner only accepts projectiles thrown or owned by the current target.
- The `v0.4.7` release retains the user-confirmed fix5 origin/rig implementation and orders throws by schema spawn time and persistent eligibility. Ownership arriving after 100 ms can still replace the previous automatic shot.
- AUTO HANDOFF follows through detonation, holds the final camera for 2 demo seconds, then returns to the agent's native view while awaiting future throws. Pause stops the timer; slow playback extends the wall-clock duration. New valid throws take over immediately without waiting for the hold to finish.
- PIN CURRENT PROP protects the current shot and final view indefinitely. Row SELECT + PIN chooses a specific live candidate and uses the same pin policy. AUTO HANDOFF unpins; an elapsed effect hold returns immediately unless another shot takes over.
- CLEAR LOCK / RETURN TO AGENT releases the view immediately without recapturing cleared/existing candidates. Future throws still auto-capture; explicit row selection can reopen a live cleared candidate. The lower target Clear Lock also releases the prop camera and restores Follow target selection.
- The camera latches its first valid inverse horizontal launch yaw. A temporary fallback can be promoted once when initial velocity arrives; later data changes/bounces cannot rotate the established rig.
- Entity-birth observation takes priority (up to 64 queued notifications per callback), preserving the original engine/HLAE chain. The fallback covers all slots 0..32767 under a resumable 0.5 ms soft scan budget. The old query returns population count, so it cannot be used as the highest occupied slot. Observation failure leaves the complete sweep available.
- Up to 128 known or pending projectile slots are revisited first every callback. Base grenade RTTI resolves its concrete type through HLAE-style `m_designerName`.
- New entities get a 100 ms initial-velocity grace, supplemented by newer spawn time and observed scene movement. Eligibility does not expire with this grace. Stationary baseline smoke/decoys cannot capture the camera through stale initial velocity.
- Landing, stopped motion and scheduled effect ticks do not end tracking. Actual client effect flags or removal start the final-view hold while game effects continue. One-tick time jitter is ignored; rewinds over 0.25 seconds reset chronology. Clear releases the view and preserves capture order; seek back before a throw to replay it.
- Position and aim share the same current render-scene sample. The relative distance remains `sqrt(X*X + Y*Y + Z*Z)` without independent velocity integration, prediction drift or a refresh-based correction. Legacy `PropFollowSpeed` is retained for configuration compatibility but no longer limits following speed.
- Origin sampling uses the verified engine accessor, resolving dirty scene transforms before returning absolute position. It does not directly trust the cached field or scan arbitrary vectors. Scene/owner gaps retain the validated view; pending throws keep the displayed view until ready. Per-shot FOV remains stable.
- If a projectile is born and destroyed entirely between camera frames, flight images cannot be reconstructed; slow the demo or seek back before the throw. Restart HLAE/CS2, load one DLL, and enable Prop Selfie before the throw so creation notifications are active.

Prop controls:

- `Prop X / Y / Z`: local prop camera offset through native numeric fields and 0.25-step sliders
- `Pitch / Yaw / Roll`: angle trim through native numeric fields and sliders
- `CENTERED LOOKBACK PRESET`: places the camera directly on the outgoing trajectory axis: `X=0`, `Y=-13.5`, `Z=2.25`, `Pitch=0`, `Yaw=0`, `Roll=0`
- `Recent Props`: candidate list with validity, handle, thrower handle, age, type, and lock state

## Failure States

For same-shot prop/background motion issues, expand Runtime and click `EXPORT MOTION DIAGNOSTICS`. It exports up to 1,024 recent callback samples to `selfiestick_prop_motion.csv` beside the loaded DLL, including build, handles, source origins, camera pose, phase and FOV. Normal capture writes only a bounded memory buffer; explicit export writes the file on a worker. Per-frame trace logging is not required.

The Runtime diagnostics bar is collapsed by default. Click it to expand the live state when the camera is blocked. Common examples:

- no observer target
- target unresolved
- prop not locked
- prop ended
- schema or SetUpView compatibility failure

Trace logging is off by default. To collect diagnostics, set `[Debug] TraceEnabled=1` in `selfiestick.ini` next to the DLL and fully restart HLAE/CS2. The DLL then writes `selfiestick_trace.log` next to the loaded DLL.

If you replace the DLL, fully restart HLAE and CS2 before testing again.
