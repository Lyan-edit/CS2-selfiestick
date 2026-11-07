# Local hand-anchor fix — 2026-11-06

Historical investigation: the hand-anchor correction described below is included in formal release v0.4.5. The local/unpublished wording and hashes below record the initial test build.

This is an unpublished local fix based on v0.4.5beta. GitHub attachments are unchanged. Runtime compatibility remains CS2 build 26445667, HLAE 2.192.10 / AfxHookSource2 0.41.9.

## Investigation

The camera update uses one mutually exclusive Player/Prop branch. Retaining a projectile lock while Player mode is selected does not itself execute the projectile camera. No simultaneous-mode activation was reproduced in one DLL instance.

The old gun path required a valid active weapon, muzzle and both hand attachments every callback. A missing transform returned without overriding the original CS2/HLAE view; intermittent success can therefore alternate the selfie and upstream views. The active weapon was re-read every frame, so a changed weapon handle alone was not sufficient to prove the reported cause.

Two defects were reproduced using production functions in a standalone process, with fake entities in private test memory:

- An expired handle 163882 resolved to replacement handle 196650 because only the entity index was checked. Viewmodel restoration then wrote to the replacement.
- The old gun right-axis continuity logic flipped world-up from +1 to -1 on a 180-degree turn. The new Player path no longer calls this gun-basis logic.

Other observations: an empty observer target could select the spectator pawn; F9 is intentionally ignored while the panel is visible (use the panel's Lock Current button in that case). A pawn handle remains a lifetime handle, not a permanent player identity across respawn or demo seeks.

## Changes

- All player-held items use the pawn's right-hand attachment `weapon_hand_r`, not a gun/knife entity anchor.
- Camera yaw/right/up come from the pawn's yaw and world-up. The look-at point is the pawn eye minus six vertical units, independent of weapon or hand animation.
- A missing hand transform uses the last valid pawn-local offset for up to 0.25 seconds, then a deterministic body-relative hand position. Moving/turning the pawn moves this fallback; an unrelated CViewSetup pose is never used as the agent eye/angles.
- Weapon metadata is optional and only used for FOV/viewmodel handling. Gun/knife/unarmed transitions keep the same hand-based framing profile.
- Entity resolution requires the complete current handle, including serial, to match. Viewmodel restoration skips a destroyed/reused weapon and never writes through a saved raw pointer.
- Empty-target fallback rejects spectator pawns and nonzero observer modes, avoiding a false observer lock.
- Framing v6 preserves the old INI in `.pre-hand-anchor.bak`, resets old muzzle-relative player trim once, and keeps prop settings.

## Verification

The production-code hidden test covers stale/exact handles, no write to a reused entity, an absent/rebuilding weapon, a missing hand transform, pawn movement, unrelated upstream camera values, near-vertical pitch, a 180-degree yaw turn and equal gun/knife framing. Native observer-fallback regression also rejects spectator/observer-mode self-targets. The existing hidden panel and camera compatibility regressions remain in use.

Final native regression, production hand/hidden-panel regression, installed CS2 signature/schema validation, bilingual builds, dependency audits and whitespace checks passed. Local DLL marker: `v0.4.5beta hand-anchor-fix` / `schema-probe-v36-hand-anchor-cs2-26445667`.

- Chinese SHA-256: `C9EE649BFA363F38567076DE464CD1C2C5F1E419D2E0F1AC57353A61B8469CD5`.
- English SHA-256: `6AAA3269BF6BD6E401B110D6E07C8DA7FEC424B904B854705F50EB81A0A1CB76`.

No CS2 process was running during investigation. No demo playback, live injection, ReShade coexistence test or game-frame measurement was performed. The reported in-demo oscillation still needs session validation; this fix addresses the confirmed unsafe lifetime logic and removes weapon lifetime from Player camera geometry. If the pawn itself is destroyed/recreated, the old lock now fails safely and must be selected again rather than silently following a reused slot.
