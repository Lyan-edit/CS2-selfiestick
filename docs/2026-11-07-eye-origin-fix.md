# Eye-origin correction — 2026-11-07

Historical investigation: this correction is included in formal release v0.4.5. The local-build hashes below identify the earlier diagnostic build, not the final release DLLs.

The reported screenshot shows the camera inside/below map geometry after enabling Player Selfie. CS2 was not running when inspected, so no live scene reproduction was performed. The current Chinese release INI selects PlayerSelfie / framing v6; user R/B/U values were preserved.

## Confirmed defect

The production wrapper called pawn vtable slot 166, zero-initialized the output array and accepted any finite result. Offline disassembly of the installed build 26445667 primary C_CSPlayerPawn vtable shows:

- Primary vtable RVA: `0x1C8F288`.
- Slot 166 -> RVA `0x166840`, forwards to slot 167 via `jmp [rax+0x538]`.
- Slot 167 -> RVA `0x166830`, `xor eax,eax; ret`; no eye-output write.
- Slot 173 -> RVA `0x93DF30`, saves the output pointer from RDX, reads camera services at pawn offset `0x1328`, delegates the eye computation and returns the output pointer.
- Workspace HLAE 2.192.10 `AfxHookSource2/ClientEntitySystem.cpp` also uses slot 173 for GetRenderEyeOrigin.

Therefore a normally returning old-slot call could leave `(0,0,0)` in the output, bypass scene fallback and anchor the synthetic hand near world origin. The existing hand test had no executable render-eye method and exercised the scene fallback, so it did not cover this defect. This explains a concrete path to the screenshot's incorrect world placement; it is not proof of the exact final in-demo pose without a live capture/log.

## Correction

- Explicit fingerprint-gated render-eye slot 173.
- Initialize all output components to NaN so a no-op or partial write fails validation.
- Validate eye XY within 48 units of the selected pawn scene origin and height 16..96 units above it; retain valid crouched eye positions. Reject remote/zero eye results and use actor-relative scene fallback when trustworthy.
- Reject an uninitialized zero scene plus invalid eye rather than constructing a camera near `(0,0,0)`.
- Final pose writes reject non-finite position or angle components before touching CViewSetup.
- The offline compatibility checker now resolves the pawn primary RTTI/vtable and validates the eye function's output/camera-service path. An explicit old-slot-166 negative test fails as required.
- Existing INI values are unchanged. Runtime INI backups are ignored by Git/source packaging and have not been deleted.

## Regression coverage

Fresh production-code hidden tests exercise no output, partial output, zero output, finite far-away output, valid standing and crouched output, large actor world coordinates, missing weapon/hand transforms, a 180-degree turn, stale handles and non-finite final-write rejection. No calls to the old slot are allowed. Existing UI full-frame cleanup tests remain enabled.

The installed CS2 signature/schema/SetUpView/eye-vtable check passes against build 26445667. English/Chinese DLLs use build marker `v0.4.5beta eye-origin-fix` / `schema-probe-v37-eye-origin-cs2-26445667`.

Fresh bilingual builds, dependency audits, native regression, hidden production eye/hand/UI regression, the old-slot-166 negative check and whitespace validation passed. Local release DLLs and `bin/selfiestick_hlae.dll` were updated; runtime R/B/U values remain unchanged.

- Chinese SHA-256: `2E60E84E56B69EBC9A50B4B349FEEED8AE9CBC19E1E534015A841964BAF0DE1E`.
- English SHA-256: `1BEC2B6FC503F824BAE67897B73A2B1FFBDABBF374C3981F80A7727D36CB3587`.

This fix corrects coordinate placement. A world collision trace / automatic wall avoidance has not been introduced, and a deliberately offset camera can still intersect nearby map geometry. No game launch, injection, demo interaction or in-game FPS test was performed. GitHub release assets are unchanged.
