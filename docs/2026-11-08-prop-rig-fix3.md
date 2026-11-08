# Prop render rig and birth capture fix 3 — 2026-11-08

Historical build, superseded by [prop-effects-fix4](2026-11-08-prop-effects-fix4.md). Capture remains; pose recovery, stable FOV, scheduled-tick handling and explicit effect-view pin/unpin were added after follow-up feedback.

Local build: `v0.4.5 prop-rig-fix3`.
Diagnostic marker: `schema-probe-v40-prop-rig-cs2-26445667`.
The published GitHub v0.4.5 Release is unchanged; these are local replacement DLLs.

## Findings

The previous carrier integrated its own velocity, blended that velocity toward a measured/schema estimate, extrapolated a target and applied a small positional correction. Its aim target was the current raw scene origin. Thus the carrier and visible prop used different position/time histories. Repeated scene samples could let the carrier continue moving until the stale-motion reset snapped it back. Increasing polling frequency or follow speed did not remove that error source.

A deterministic test against the prior production function failed at the first updated sample: the carrier was already 0.146 units away from the supplied scene origin (exit 22). The new tests enforce a shared positional sample throughout repeats, pauses, gaps and velocity changes.

Discovery also used the wrong scan bound. The function reached at `kPatternEntityList + 27` is RVA `0x9F9580`; its current instructions return `[rcx+0x2120]`, or that member plus `[rcx+0x2124]`. These are population counters incremented/decremented by entity lifecycle methods. This is not the highest occupied slot. The local HLAE 2.192.10 reference independently notes this count/index distinction. A prop at a sparse high handle index could therefore never be reached by the old bounded sweep, even if it survived many frames.

## Render rig

The carrier now uses the same validated `CGameSceneNode::m_vecAbsOrigin` sample as the look target:

`camera = propOrigin + right * X + launchLookback * Y + up * Z`

The basis is orthonormal, so prop-relative distance is `sqrt(X*X + Y*Y + Z*Z)`. No separate wall-clock integration, extrapolation, velocity response, speed cap, pause timer or refresh correction changes that distance. The first valid inverse horizontal initial-velocity direction is latched for the exact handle; a temporary fallback may be promoted once when that metadata arrives.

The legacy `PropFollowSpeed` INI key is retained but no longer governs the carrier. Existing offsets and rotation trim are preserved. Guessed entity-memory vector scans and stale-origin holds are removed; missing/invalid scene data releases the pose and permits a valid latest prop to retry. This avoids following an unrelated float vector while waiting for a later refresh to select another origin.

## Birth capture and fallback

- Observe virtual `CGameEntitySystem::OnAddEntity` notifications after the original engine/HLAE target returns. Only the exact-build vtable slot 15 is atomically replaced. Slot 16 and function prologues are untouched. A HLAE detour already at the original entry remains in the call chain.
- Installation validates the exact client fingerprint, primary vtable RVA `0x1C33A10`, Add RVA `0x9F95D0` and Remove RVA `0x9FA050`. A mismatch keeps the full-slot fallback. The DLL is pinned before its callback is published, matching the existing process-lifetime camera/UI workflow.
- The notification records only the full serial-qualified handle in a bounded 256-entry queue. It does not retain a raw entity pointer or run discovery/schema work in the lifecycle callback. Producer locking is nonblocking; overflow retains recent births. Each camera callback prioritizes up to 64 recent notifications and verifies that the handle still resolves to the same serial.
- Pending/known projectile slots are revisited, while the resumable fallback covers every index `0..32767`, independent of population count. Existing bounded cold-class work, soft scan budget, owner checks, late-ownership handoff, terminal-effect release and rewind chronology remain.

This observer handles the validated virtual dispatch path. Static inspection found no direct E8/E9 references to the Add entry in the installed client, but that is not proof that every runtime creation path will notify through this slot. The complete sweep remains necessary. Notifications cannot reconstruct a prop that was created and removed between visible camera callbacks, nor safely attribute a projectile whose ownership was never observable.

## Background verification

- Installed CS2 remains build `26445667`: PE timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`. HLAE target remains `2.192.10` / AfxHookSource2 `0.41.9`.
- Required signatures, all 30 schema entries, pawn eye slot 173, entity-system primary vtable slots/RVAs and count-accessor semantics passed the offline binary checker. A deliberately wrong Add RVA was rejected.
- Fresh Chinese/English x64 static-runtime builds and native compatibility/schema/hook/camera validation passed under PowerShell 7.
- Deterministic anchor cases covered 30/60/144/500 FPS, 0.1/1/8x playback, repeated samples, pauses, frame gaps, changed velocities/bounce metadata and legacy follow-speed values.
- 1,800 production camera poses plus repeated paused poses preserved expected prop-relative distance within 0.002 units and centered direction within the dot-product tolerance. Changing initial-velocity metadata later did not rotate the latched rig.
- Sparse discovery passed with reported count 32 and a live prop at index 25,000. A queued one-frame birth at a new index 28,000 was captured while a deliberately slow background sweep was elsewhere. A delayed birth for the wrong serial at index 31,000 was rejected before the correct event was accepted.
- Lifecycle tests in the hidden standalone EXE checked guarded atomic slot replacement, original-chain-first ordering, returned-result preservation, untouched Remove slot, active-mode/system filtering, queue capacity/newest-first behavior and non-clobbering compare-exchange failure. These tests edited only the test process's fake vtable.
- Previous follow-chain, hand/eye and actual production ImGui/framebuffer regressions passed.
- Final simulated 32,768-slot warm-cache discovery benchmark, 500 callbacks: median **0.246 ms**, p95 **0.260 ms**, maximum **0.299 ms**. This uses a fake entity accessor and is not game frame time, cold-cache cost, callback latency under contention or an FPS guarantee. The 0.5 ms scan budget is soft and checked in chunks.
- Both DLL dependency audits, fix3/v40 marker checks, copied-artifact SHA-256 checks, whitespace and active-manual consistency checks passed.

## Artifacts

| DLL | Bytes | SHA-256 |
| --- | ---: | --- |
| Chinese `release/zh-CN/Lyan_CS2自拍杆.dll` | 1,140,224 | `B1E3755FCE285D6D52CF01DD0A0C5DDDB0AD22D16D69A91863754E0E0F2061D7` |
| English `release/en-US/Lyan's selfiestick.dll` | 1,137,664 | `9A8473DEEC16E01BBCCFE5D5FA873CB915B0A2238CF6FEF44C7052725423A83B` |

`release/zh-CN/Lyan_CS2自拍杆_prop-rig-fix3.dll` and `bin/selfiestick_hlae.dll` are identical copies of the Chinese DLL. Personal INI files were not overwritten.

Only background/offline and hidden-process tests were performed. There was no CS2 launch/injection, foreground control, live demo framing, ReShade coexistence or in-game FPS measurement. A new exact-build virtual lifecycle observer remains a material runtime change despite the offline and mock checks. Fully restart HLAE/CS2, load one replacement DLL, and enable Prop Selfie before the throw. Live acceptance of every short throw and absence of crashes/frame loss remain unproven until an in-demo test is authorized/performed.
