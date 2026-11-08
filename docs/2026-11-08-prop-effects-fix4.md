# Stable prop recovery and pinned effect views — 2026-11-08

Historical build, superseded by [prop-origin-fix5](2026-11-08-prop-origin-fix5.md), which corrects dirty absolute-coordinate sampling while retaining these shot modes.

Local build: `v0.4.5 prop-effects-fix4`.
Trace marker: `schema-probe-v41-prop-effects-cs2-26445667`.
This supersedes the local fix3 build. Previously published GitHub artifacts were not changed.

## Behavior

The prop panel now has **AUTO HANDOFF / 自动接续** and **PIN CURRENT PROP / 锁定当前道具**.

- Automatic mode gives newer usable throws priority. With no newer throw, the current shot continues through landing/detonation and keeps its final effect view. Retaining a view never blocks automatic acquisition.
- Pinning protects the selected live shot and its final view from later throws. The camera can remain at the explosion/smoke/fire/decoy location after the projectile entity is destroyed, while the game and effects keep playing normally. There is no fixed hold timer. Select Automatic mode to unpin without resetting the camera; a newer still-valid throw can take over on the next callback.
- Candidate-row LOCK also pins that shot. Locking the already selected shot does not clear its camera state.
- CLEAR PROP / 清除道具 exits the retained view and resets capture chronology. It can reacquire an eligible still-flying candidate. Disabling the camera or choosing Player Selfie exits the prop override.
- The panel reports Tracking, data recovery or final effect view. Retained views can still be reframed with the existing offset/angle controls.

## Flicker and early-end paths changed

The previous callback cleared the selection after any failed pose read. A transient scene/owner gap could therefore cut to the observer camera and reacquire the prop later. The implementation now separates selection, live entity access and the last validated displayed view.

Missing/invalid scene data or temporary owner loss retains that view and retries on subsequent callbacks without extrapolation or an arbitrary timeout ending the shot. During automatic handoff, the previous displayed view is retained until the new selection has usable geometry. This preserves the original render-bound position on valid frames without adding a world-position smoothing integrator or reducing capture frequency.

Each shot captures its first valid FOV. Subsequent spectator zoom values, including invalid values, cannot oscillate that shot's FOV. A new shot captures its own FOV. This intentionally also holds an externally requested FOV change until another shot/clear; no new FOV slider was added.

Common explosion/smoke TickBegin fields can be scheduled before the actual effect. Positive tick alone, landing and zero velocity no longer mean that a shot has ended. Actual client explosion/smoke flags, decoy activation or entity removal transition a validated shot to its final effect view. The final view uses the latest usable sample before/at that transition, rather than guessing arbitrary vectors from a destroyed or repurposed entity.

One-tick pawn simulation-time oscillation no longer resets shot chronology. The rewind threshold is now 0.25 seconds, and a temporary controller-handle change does not rebuild the pawn's capture context. Explicit CLEAR PROP resets chronology for smaller intentional replay seeks. Exact handle serial validation, owner filtering, effect candidate rejection and the fix3 entity-birth observer/full-slot fallback remain.

## Implementation boundaries

`PropViewCache` contains only a bounded copy of pose/basis/FOV and handle metadata, never a retained raw entity pointer. Effect holds do not dereference a destroyed projectile. Updating the view checks the currently selected handle, and state/view mutations share one lock order. A retained source pose may belong to the previous shot while a new selected handle awaits usable scene data; this is deliberate handoff buffering.

A pinned shot can retain/follow its view after the thrower pawn disappears, using saved reference/basis data. Large demo rewinds, explicit clear or a true context/selection reset can still discard cached views. Holding the camera does not pause the demo, extend any game effect or reconstruct frames that were never rendered. If no valid pose was ever obtained for a shot, its final explosion position cannot be promised.

## Background checks

- CS2 installed build `26445667`, PE timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`; HLAE target `2.192.10` / AfxHookSource2 `0.41.9`.
- Offline signature, all 30 schema entries, pawn eye-slot 173, lifecycle vtable/RVA and population-count checks passed; no new schema offset or hook was added by fix4.
- Fresh PowerShell 7 x64 static-runtime Chinese/English DLL builds, native compatibility regression and both dependency audits passed.
- Existing short-lived birth/sparse-slot capture, exact-serial, 1,800 fixed-distance camera poses, frame-rate/playback sampling, hand/eye, lifecycle-chain and ImGui framebuffer/smear regressions passed.
- New production-function cases passed: landing plus a future explosion tick continues tracking; missing scene pointer keeps the same camera; owner gaps recover even after a longer delay; one-tick simulation-time oscillation keeps shot state; per-shot FOV remains stable across 30/110/NaN/zero host inputs; effect view persists after destruction; automatic new-shot handoff buffers a missing first scene sample and keeps the old FOV until ready; pinning ignores a newer smoke; unpinning immediately resumes acquisition; same-row pin does not reset; detached thrower and explicit-clear behavior remain valid.
- The actual Chinese ImGui panel was rendered in the hidden standalone process and inspected: Automatic/Pin buttons and phase text are visible without expanding diagnostics. The panel retains its normal scrolling and collapsible Runtime bar.
- Final simulated 32,768 populated-slot discovery benchmark, 500 callbacks: median **0.272 ms**, p95 **0.324 ms**, maximum **0.500 ms**. This is not an in-game FPS or full-render-loop benchmark. Work limits remain soft, not real-time deadlines.
- Copied artifacts passed size/SHA-256, fix4/v41 marker and whitespace checks. Personal INI files were retained.

## Artifacts

| DLL | Bytes | SHA-256 |
| --- | ---: | --- |
| `release/zh-CN/Lyan_CS2自拍杆.dll` | 1,145,856 | `16E48426402BDE4DF606D03B00A651423F744CC124414427E7D151478A21AFC5` |
| `release/en-US/Lyan's selfiestick.dll` | 1,143,296 | `DC14F1D8F9689495B9087A93A5473050143749E1ADBF4D14A706742F7FA9DBBF` |

`release/zh-CN/Lyan_CS2自拍杆_prop-effects-fix4.dll` and `bin/selfiestick_hlae.dll` match the Chinese DLL.

No CS2 launch/injection, foreground input, live demo playback, ReShade coexistence test or in-game FPS measurement was performed. The changes eliminate the demonstrated clear/reacquire, scheduled-tick and FOV-state paths; they do not establish that every real source of twitching/flicker has been eliminated. Fully restart HLAE/CS2 and load one replacement DLL before evaluating the new modes.
