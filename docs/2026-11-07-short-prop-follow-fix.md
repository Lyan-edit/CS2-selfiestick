# Short projectile follow fix — 2026-11-07

Historical first attempt, superseded by [prop-chain-fix2](2026-11-07-prop-chain-fix2.md). Its 100 ms handoff gate and entity-existence-only end check were insufficient in follow-up testing; the current build removes post-flight lock/pose holds.

Local post-release build: `v0.4.5 short-prop-fix`.
Diagnostic marker: `schema-probe-v38-short-prop-cs2-26445667`.
The published v0.4.5 Release remains a separate, earlier artifact.

## Confirmed code paths

- Discovery prioritized only the highest 128 entity slots, throttled by wall-clock intervals. Projectile slots can be reused below that tail. The descending 192-slot fallback could reach a short-lived projectile too late.
- An existing lock reduced discovery frequency to 250 ms and suppressed the hot-tail path. An earlier projectile's post-flight hold therefore delayed the next short throw.
- Current velocity was mandatory even when a freshly created projectile already had initial velocity but had not received its current velocity/owner fields.
- The camera callback copied the prop owner/type before discovery and refreshed only an initially missing handle. A same-frame selection change could use the previous throw's metadata.
- Full-pass validation depended on the moving-candidate list, so a live manually locked projectile could be released when it stopped moving.

## Implementation

`prop_discovery.inl` replaces the tail/full-pass split with one resumable all-slot sweep on each active Prop Selfie callback. Unknown and known class kinds are cached by vtable, so ordinary world entities need only a guarded entity lookup, vtable read and cache lookup. The soft scan budget is 0.5 ms, checked every 64 slots; cold RTTI work is additionally limited to eight new classes per callback. A checkpoint can exceed the target budget, and OS scheduling can add latency. Neither limit is a real-time deadline or an FPS guarantee.

Each projectile slot records its full handle/serial, prior observation, first acceptance and a 100 ms fresh-birth window. Only an observed birth/reused slot or a newly grown slot can use initial velocity during this window. Stationary smoke/decoys already present on activation cannot capture the camera through stale initial velocity. Owner validation still requires the selected pawn or controller and accepts current thrower, original thrower or owner-entity fields.

Automatic acquisition can hand over to a newly observed throw while the previous automatic shot is live or holding its last frame. A live explicit manual lock remains pinned, including after landing. An ended lock can yield to another flying projectile. Direct lock validation uses exact handle serial and owner; full-list movement filtering cannot invalidate an otherwise live manual lock. Selection changes reset prediction, launch-axis and hold state. Mode/target/UI-lock changes are checked again before committing discovery results.

The callback reads the selected handle, owner and projectile type together after discovery and uses them to build the same-frame camera pose. Existing inverse-launch-yaw framing, raw-origin centering, smoothing, 0.45-second short-prop hold and 1.2-second smoke hold remain.

## Background validation

- Rebuilt x64 static-runtime Chinese and English DLLs using PowerShell 7.
- Installed CS2 remains build `26445667`; client PE timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`. Binary checker passed all required signatures, all 22 schema fields and pawn render-eye slot 173. HLAE target remains `2.192.10` / AfxHookSource2 `0.41.9`.
- Native compatibility/schema/hook/camera validation passed.
- Hidden production-code tests passed: low-index one-frame flash; zero first-frame velocity; same-frame HE owner/type and pose; hold handoff; explicit live/manual/stationary locks; five projectile types; same-pointer/index/vtable serial reuse; late owner via controller/original thrower; owner-entity fallback; expired initial-velocity grace; stationary baseline smoke/decoy rejection; selected-pawn/controller replacement; disabled/player-mode isolation; slow-lookup budget checkpoint and resume.
- In the final offline warm-cache benchmark with 32,768 populated fake world slots, 500 callbacks measured median **0.226 ms**, p95 **0.234 ms**, maximum **0.315 ms**. This measures the discovery function against a simulated entity accessor, not CS2 frame time or real-game entity density/cache behavior.
- Existing hand/eye tests and actual production ImGui rendering/framebuffer smear tests passed.
- Both DLLs passed dependency audit and version-marker verification. Copied package DLLs were verified against build-output SHA-256.
- Whitespace checks passed. No game launch, injection, foreground UI control or demo playback was performed.

## Local artifacts

- `release/zh-CN/Lyan_CS2自拍杆.dll`: `2B3FBA12DE7A7312AD9B49E1B767A6E2FFDB6C1389A8BB6C4DE3DFBBA7F5C9D1` (1,144,832 bytes).
- `release/en-US/Lyan's selfiestick.dll`: `3FCFBDA72C4DEBFB4A7E4D0BAE02B4962BC522137DDAC4057A3E223111E04860` (1,142,272 bytes).
- `bin/selfiestick_hlae.dll` matches the Chinese package DLL.

Personal INI files are retained. Restart HLAE/CS2 before loading the replacement DLL. Enable Prop Selfie before the throw. A projectile born and destroyed entirely between camera callbacks has no visible flight frame to follow; slow playback or seek back before the throw in that case. Under a cold cache or budget overrun, a full sweep can span multiple callbacks, so this fix cannot promise capture of every sub-frame entity.
