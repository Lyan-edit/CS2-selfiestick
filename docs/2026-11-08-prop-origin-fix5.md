# Engine-resolved prop origin sampling — 2026-11-08

Local build `v0.4.5 prop-origin-fix5`; marker `schema-probe-v42-prop-origin-cs2-26445667`.
The previously published GitHub v0.4.5 artifacts are unchanged.

## Report and evidence

The user clarified that the prop/background shake without the camera cutting back to the observer. No CS2 process was running during this investigation, and the available trace files were older builds, so no current live input trace was available. This update addresses a concrete source-sampling defect consistent with that symptom; it is not evidence that every runtime source of shake has been eliminated.

Previous builds directly copied `CGameSceneNode::m_vecAbsOrigin` at scene+0xC8. The installed client's common absolute-origin accessor at RVA `0x219BC0` instead:

1. Reads the entity's scene node at +0x330.
2. Checks the node's dirty-transform byte at +0x110.
3. Calls the scene-transform resolver at RVA `0x9D2280` when required.
4. Returns the vector at node+0xC8.

Primary projectile vtables inspected offline all use render-eye slot 173 at RVA `0x15FBF0`, whose first origin call is this same absolute-origin accessor before it adds a view offset. The local HLAE 2.192.10 reference uses that virtual render-eye path. This camera needs the entity's origin rather than an eye offset, so it calls the verified common absolute-origin accessor directly.

A new deterministic production-read regression alternated a stale cached origin by ±15 units while marking the scene transform dirty. Before wiring the accessor into the camera, it failed at frame 0 with **15.000 units error and zero resolver calls** (exit 24). After the change, all 480 samples matched the resolved positions. Earlier smooth/constant-distance tests had not modeled a dirty scene cache and therefore did not cover this defect.

## Change

Both prop discovery's movement check and camera origin sampling use the resolved accessor. It is bound only for the supported exact client fingerprint and must lie in executable client memory. A known production client cannot silently fall back to reading an unresolved cache if that binding is missing. The mock-only fallback remains for standalone fake entities.

The guarded call rejects null/non-finite results and coordinates that differ between two immediate reads. It performs no independent position integration, arbitrary vector search, polling-rate reduction or prediction. The existing pose-recovery and effect-hold paths handle a failed sample. The new origin path adds no hook and leaves short-lived birth capture, serial checks, automatic handoff, pinned effects and per-shot FOV behavior intact.

## Diagnostic export

Runtime now contains `导出抖动诊断 / EXPORT MOTION DIAGNOSTICS`. Successful prop override callbacks record a bounded ring of at most 1,024 samples in memory. The producer uses nonblocking insertion and does not write files. Export explicitly snapshots that ring and writes CSV on a worker to `selfiestick_prop_motion.csv` beside the loaded DLL. Only one export worker can run at once.

The CSV includes build/marker comments, monotonic sample sequence/time, selected/current-focus/displayed handles, target, shot phase, pin state, origin-read sequence, observed dirty-resolve count, cached/resolved coordinates, camera position/angles and FOV. It stores no player names or entity pointers. Origin-read sequence distinguishes a retained view from a fresh source sample; coordinates are the most recent matching source sample, not necessarily a newly read source on every recovery/effect-hold callback. Phase values are 0 waiting, 1 tracking, 2 recovering and 3 effect hold.

The cumulative dirty counter includes known projectile slots visited by discovery, not solely the selected shot. These callback samples are not a per-present GPU/FPS trace. If the problem remains, an export immediately after it occurs supplies the current build and input/output evidence absent from this investigation.

## Background checks

- Installed CS2 build `26445667`, PE timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`; target HLAE `2.192.10` / AfxHookSource2 `0.41.9`.
- All required signatures and 30 schema entries, pawn eye slot 173, entity lifecycle vtable/count semantics, absolute-origin dirty branch and transform-resolver call target passed the offline checker. A deliberately wrong origin-accessor RVA was rejected.
- PowerShell 7 Chinese/English x64 static-runtime DLL builds, native compatibility regression and both dependency audits passed.
- New 480-sample dirty-cache test passed, along with null/non-finite return rejection and refusal of an unbound production resolver.
- All prior short-lived capture, sparse indices, 1,800 fixed-distance poses, varied frame/playback clocks, owner/scene recovery, FOV, pinned/unpinned effects, destruction/rewind, lifecycle chaining, hand/eye and ImGui framebuffer/smear checks passed.
- Diagnostic overflow/export tests produced exactly 1,024 chronological records from 1,100 inserted fake samples; an independent CSV parser verified retained endpoints and all 26 fields. Test CSV data came from the standalone EXE, not a live demo.
- Final simulated 32,768-slot warm-cache sweep benchmark, 500 callbacks: median **0.213 ms**, p95 **0.274 ms**, maximum **0.457 ms**. This does not measure the real engine transform resolver's runtime cost, GPU rendering or in-game FPS.
- Hidden panel inspection confirmed the export button is visible immediately under the expanded Runtime header. Version markers, copied-file hashes and whitespace checks passed. Personal INI files were retained.

## Artifacts

| DLL | Bytes | SHA-256 |
| --- | ---: | --- |
| `release/zh-CN/Lyan_CS2自拍杆.dll` | 1,160,704 | `5ED6B1199C6609A1DDC08FCD3152ADD75CD7BD3C019E74AC7915AC31735B77B8` |
| `release/en-US/Lyan's selfiestick.dll` | 1,158,656 | `34AA660070029889E580DC7938DE4B5E4CD53CF4C365800B0F01ED3C5CE47A30` |

`release/zh-CN/Lyan_CS2自拍杆_prop-origin-fix5.dll` and `bin/selfiestick_hlae.dll` match the Chinese DLL. Fully restart HLAE/CS2 and load one replacement DLL.

No game launch/injection, foreground input, live demo motion test, ReShade coexistence or in-game FPS measurement was performed. The verified engine getter legitimately recalculates dirty scene transforms, as the game's own render-eye path does. Live confirmation is still required to establish whether this resolves the reported shake completely; residual input interpolation, rendering or external effects cannot be ruled out by the mock tests.
