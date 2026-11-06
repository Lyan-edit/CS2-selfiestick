# Background validation — 2026-11-01

Target runtime: installed CS2 build `26334290`, HLAE `2.192.10` / AfxHookSource2 `0.41.9`.

## Installed binary evidence

- App manifest LastUpdated: `2026-10-31 09:44:04 +08:00`.
- `client.dll` SHA-256: `D618BBF8E6AFB36BAA09B3A282035996A88300458C55D2AA5CB81F0495C0EC54`.
- PE timestamp `0x6AE4FB84`, image size `0x029A2000`, checksum `0x0256C2C4`.
- Entity list, indexed entity accessor, attachment accessor and SetUpView patterns have unique required matches. Split-screen accessor retains its two expected candidates.
- Attachment CALL targets, SetUpView 16-byte instruction boundary and RBX view origin/angle context pass.
- All 22 fields in `TryGetKnownClientSchemaFieldOffsetFallback` were matched against the class-qualified, PE-resident schema field arrays in this exact local binary. Their offsets have not changed. The checker reads file bytes without loading client.dll, calling TypeScope functions or injecting into CS2.

## UI and camera

- Style is exactly upstream `ImGuiStyle{}` plus `StyleColorsDark()`. Production UI no longer draws custom gradients, hero cards or accent rails.
- The hidden preview uses the production UI functions, not a separate mockup. Player, prop and expanded Runtime captures pass.
- Twelve successive slider-value frames followed by restoring the initial state produce a byte-identical framebuffer: no retained slider pixels.
- A semi-transparent rectangle has uniform pixels across its shared triangle edge. Texel-center sampling and top-left coverage remove the previous diagonal seams.
- Player-pose tests cover both horizontal world axes, mirrored left/right offsets, positive legacy Back trims and an anchor behind the agent. The camera stays at least 8 units in front of the upper-body target and the optical axis intersects that target. Invalid forward axes are rejected.
- Framing settings now persist the actual v5 constant instead of writing v3 and triggering a reset on every later startup.

## MulNX review

Reviewed upstream commit `c8294146a901e07df60abcd57f04abab9d622be1`:

- [AdvancedViewController.cpp](https://github.com/Co1Swet/MulNX_CS2/blob/c8294146a901e07df60abcd57f04abab9d622be1/source/MulNXExtensions/CS2/Feature/View/AdvancedViewController/AdvancedViewController.cpp) constructs a local coordinate system from three weapon/body points and calculates position separately from rotation.
- [AdvancedViewController.hpp](https://github.com/Co1Swet/MulNX_CS2/blob/c8294146a901e07df60abcd57f04abab9d622be1/source/MulNXExtensions/CS2/Feature/View/AdvancedViewController/AdvancedViewController.hpp) defaults to bone indices 0/1/2 and zero position/rotation offsets. It does not contain an agent-centered selfie preset.
- [ProjectileTracker.cpp](https://github.com/Co1Swet/MulNX_CS2/blob/c8294146a901e07df60abcd57f04abab9d622be1/source/MulNXExtensions/CS2/Feature/View/ProjectileTracker/ProjectileTracker.cpp) selects the observed player's projectile, places the camera 80 units behind normalized velocity and aims along velocity. This is a chase camera, without a thrower/launch-direction lookback mode in the reviewed implementation.

Only the general coordinate-frame/independent-aim approach was used as a conceptual reference. No MulNX source was copied. This project retains its independent projectile launch-lookback and current-origin centering logic.

## Limits

Release DLLs were rebuilt and passed the static-runtime/graphics-dependency audit:

- English SHA-256: `D0242AFC4ED2DE22A559609B1EABCA0C91E9048C99EE628F79F6959E7142E0DF`.
- Chinese SHA-256: `69620890D3FEA6A1FE67F6D81C7E6F0EE7003D54F2D4230FFB5D58058E218262`.
- Both contain build marker `schema-probe-v34-cs2-26334290`.

These are offline compatibility, math, build, dependency and hidden-window rendering checks. CS2 was not launched, injected, driven or restarted for this validation. In-demo composition, ReShade coexistence, frame times and crash-free operation still require an actual demo session; this report makes no zero-crash or zero-FPS-loss claim.
