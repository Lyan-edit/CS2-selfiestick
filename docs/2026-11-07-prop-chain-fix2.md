# Prop follow chain fix 2 — 2026-11-07

Historical implementation, superseded by [prop-rig-fix3](2026-11-08-prop-rig-fix3.md). The follow-up removes independent camera integration and corrects the population-count/index-bound confusion in discovery.

Local build: `v0.4.5 prop-chain-fix2`.
Trace marker: `schema-probe-v39-prop-chain-cs2-26445667`.
Supersedes the local `short-prop-fix` attempt. The previously published GitHub v0.4.5 artifacts have not been changed by this local fix.

## Reproduced defects

The new production-code regression failed on the previous implementation at stage 11 (exit 21): an owner arriving 200 ms after an observed projectile birth could not replace an older, still-live automatic lock. The 100 ms freshness check had incorrectly doubled as the handoff criterion.

The lock validator also checked class, handle and owner but no detonation/effect state. A smoke, decoy or exploded projectile could retain a live entity and stale nonzero velocity, keeping the previous lock indefinitely. Post-flight holds additionally retained the old selection and pose after removal. Capturing a newer shot could later allow the older still-flying shot to be acquired again.

These are demonstrated code paths. No current in-demo trace of the user's particular failing throw was collected; the stale workspace trace was not treated as evidence of this build's live behavior.

## Changes

- Use verified common grenade explosion tick/flag, smoke effect tick/flag and decoy shot tick to reject ended projectiles even when the same entity remains present. Apply this check to discovery, manual-lock validation and pose construction.
- Remove production post-flight hold state and its 0.45/1.2-second pose/selection retention. Removal, effect start or owner invalidation clears handle, owner, kind, manual flag, smoothing, origin history and cached camera axes on the next camera callback. A new throw can be selected within that same callback.
- Order automatic handoff by `m_flSpawnTime`, with persistent first-eligible sequence as a fallback/tie breaker. The initial-velocity grace remains a motion-discovery aid, not an expiry on ownership or handoff eligibility.
- Keep an acquisition watermark to avoid returning to older superseded throws. Allow the same latest live throw to retry after a failed first pose or explicit clear. Pawn simulation-time rewind resets slot history and chronology so replaying the demo can capture earlier throws again.
- Revisit up to 128 known/pending projectile indices on every callback before the resumable background sweep. Pending owner, velocity or designer-name fields and recycled projectile slots no longer wait for the sweep to return to that index.
- Supplement missing velocity with observed scene-origin movement. Concrete type can fall back from generic grenade RTTI to `CEntityIdentity::m_designerName`, following the accessor used in local HLAE 2.192.10 source. A shared generic vtable never caches one entity's designer kind for other entities.
- Failed pose construction clears only the matching current selection; a stale failure cannot clear a newer UI selection. Clear/set operations reset camera caches inside the state mutation.
- Keep explicit manual locks while the selected projectile has not ended. A manual lock ends at explosion/effect start or invalid entity/ownership, allowing subsequent automatic acquisition.

## Verified schema additions

All additions were read from PE-resident class-qualified tables in the installed client, then added to the exact-build-gated offset table and offline checker:

| Class | Field | Offset |
| --- | --- | --- |
| C_BaseCSGrenadeProjectile | m_flSpawnTime | 0x12E8 |
| C_BaseCSGrenadeProjectile | m_nExplodeEffectTickBegin | 0x12D8 |
| C_BaseCSGrenadeProjectile | m_bExplodeEffectBegan | 0x12FC |
| C_SmokeGrenadeProjectile | m_nSmokeEffectTickBegin | 0x1360 |
| C_SmokeGrenadeProjectile | m_bDidSmokeEffect | 0x1364 |
| C_DecoyProjectile | m_nDecoyShotTick | 0x1348 |
| CEntityIdentity | m_designerName | 0x20 |
| C_BaseEntity | m_flSimulationTime | 0x3B8 |

## Background checks

- Installed CS2 build `26445667`, PE timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`; HLAE target `2.192.10`, AfxHookSource2 `0.41.9`.
- All required signatures, all 30 static schema entries and pawn eye slot 173 passed the installed-binary checker.
- Fresh x64 static-runtime Chinese/English DLL builds and native compatibility/schema/hook/camera regression passed under PowerShell 7.
- Hidden production-code suite passed the original hand/eye/ImGui checks and follow-chain cases: one-frame low/reused slot capture; late owner after 100 ms; immediate end and cache reset; lingering flash/HE/fire explosion flags, both smoke end fields and decoy activation; manual release; no older reacquisition; priority capture while slow background lookup is far away; shared base RTTI and late/changing designer names; movement without either velocity field; spawn ordering, pause and demo rewind; retry after clear and stale failure guard; target/mode isolation; scan checkpoint/resume.
- Final offline warm-cache benchmark: 32,768 populated fake world slots, 500 callbacks, median **0.260 ms**, p95 **0.297 ms**, maximum **0.444 ms**. This is a simulated entity accessor benchmark, not game frame time or an FPS measurement. The 0.5 ms soft scan budget is checked in chunks and includes priority work before the background sweep, so it is not a real-time latency guarantee.
- Both DLL dependency audits passed; packaged binaries match build-output hashes and contain the fix2/v39 marker. Whitespace and active-documentation checks passed.

## Artifacts and limits

- Chinese DLL: `release/zh-CN/Lyan_CS2自拍杆.dll`, 1,148,928 bytes, SHA-256 `249A14CFB1066DCF9E724048D9859FB7232CD140F5CC2D199B34A430550E4C80`.
- A distinctly named identical Chinese copy is provided as `release/zh-CN/Lyan_CS2自拍杆_prop-chain-fix2.dll`.
- English DLL: `release/en-US/Lyan's selfiestick.dll`, 1,146,880 bytes, SHA-256 `C1C8563EEDD46FB5E9FDDFF841D49D4F50F9C319AD2D34ACAEE0F62433849CDF`.
- `bin/selfiestick_hlae.dll` matches the Chinese DLL. Personal INI files were retained.

No game launch, injection, foreground control, demo playback, ReShade coexistence test or in-game FPS test was performed. Fully restart HLAE/CS2 before loading one replacement DLL. Enable Prop Selfie before the throw. An entity born and destroyed entirely between observable camera callbacks still needs slower demo playback; cold/budget-limited discovery can span callbacks, so the tests cannot establish capture of every real throw or zero crashes/frame drops.
