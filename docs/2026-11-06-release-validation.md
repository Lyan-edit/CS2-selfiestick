# v0.4.5beta release validation — 2026-11-06

## Runtime

- CS2 app 730, installed build `26445667`.
- Manifest LastUpdated: `2026-11-05 20:10:37 +08:00`.
- HLAE `2.192.10`, AfxHookSource2 `0.41.9` (workspace runtime).
- Installed client.dll SHA-256: `9279421852772522121BE2F13409DD37436E8C809FF48898C8C3D41B09E77844`.
- Exact PE fingerprint: timestamp `0x6AEBC4DA`, image `0x029A3000`, checksum `0x0256CDA1`.

## Checks passed before publication

1. `check_cs2_binary_compatibility.py` against the installed client.dll and schemasystem.dll: all required signatures, attachment CALL targets, the SetUpView 16-byte boundary and RBX origin/angle context passed. All 22 class-qualified static schema fields match the build-gated table; no field or signature wildcard was relaxed.
2. Fresh native x64 Release builds of both languages using PowerShell 7 and `tools/Build-Native.ps1`, with the MSVC runtime linked statically.
3. Fresh `selfiestick_validation.exe`: compatibility gates, schema helpers, original/HLAE-patched SetUpView handling, player/prop camera math and smoothing regression passed.
4. Fresh standalone hidden UI preview: upstream ImGui style, player/prop/status rendering, slider frame cleanup and shared-edge coverage passed.
5. Dependency audit passed for both packaged DLLs: no dynamic MSVC/UCRT requirement, D3D shader compiler or D3D/DXGI/OpenGL/Vulkan renderer import.
6. Both DLLs contain `v0.4.5beta` and `schema-probe-v35-cs2-26445667`.
7. PowerShell script syntax and `git diff --check` passed.

## Release binary hashes

- English: `F6F6E760DF94E82FECC80169CC0942B4383DB4F87569F92755DE8F3F0E9BC592`.
- Chinese: `840B96C5C9F0079699F800778C9ABB58FD8B03EBB9A525984524811AC784A754`.

The release packager generates a separate `SHA256SUMS.txt` for the two DLLs, bilingual ZIP and complete source ZIP. Distribution settings come from the version-controlled `release/defaults/selfiestick.ini`, not local runtime preferences. The publisher uploads a draft, downloads all five attachments and compares their sizes/hashes before making the beta public.

## Limits

Only background, offline and hidden-window checks were performed. No CS2 launch, injection, demo control, ReShade coexistence test or in-game FPS measurement was performed for this release. The update is published as a GitHub pre-release; it does not claim zero crashes or zero FPS loss and does not replace the stable Latest release.
