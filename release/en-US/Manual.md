# CS2 Selfiestick Manual

## Package Contents

This English release folder contains:

- `Lyan's selfiestick.dll`
- `Manual.md`
- `selfiestick_bind.cfg`

Only the DLL is required at runtime. The manual and cfg are reference files.

## Supported Versions

This package is marked for:

- HLAE: `2.190.2`
- Counter-Strike 2: Steam app `730`, buildid `23669931`

Use it for HLAE demo/cinematic workflows only.

## Load The DLL

1. Copy the `en-US` folder to a stable location.
2. Start HLAE `2.190.2`, launch CS2, and open a demo in spectator mode.
3. Run this in the HLAE console:

```cfg
mirv_loadlibrary "<absolute path to Lyan's selfiestick.dll>"
```

Example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

If loading succeeds, the console prints a selfiestick ready message. Press `Ins` to open the ImGui panel.

Recommended setup speed: use the CS2 demo UI / HLAE playback controls, or the game console command below when available, to tune Prop Selfie shots at `0.1x` speed:

```cfg
demo_timescale 0.1
```

## Hotkeys

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
- `Forward`: forward-facing camera mode
- `Follow`: use the current spectator target
- `Lock Current`: lock the current target agent
- `Clear`: clear target lock
- `Player R / B / U`: Right / Back / Up local camera trim, editable by numeric input or by the labeled `RIGHT`, `BACK`, and `UP` sliders
- `Player Pitch / Yaw / Roll`: local angle trim, editable by numeric input or by the labeled `PITCH`, `YAW`, and `ROLL` sliders

Player angle trim is applied after the normal Selfie Left, Selfie Right, or Forward view is calculated.

## Prop Selfie

Select `Prop Selfie` in Camera Mode to follow a projectile thrown by the current target agent.

Recommended workflow: set demo playback to `0.1x`, lock or follow the target agent, wait for the projectile candidate to appear, use auto-lock for a single projectile or `LOCK` for one candidate, then tune `X / Y / Z` and `Pitch / Yaw / Roll`. After framing is stable, raise demo speed as needed.

Supported projectiles:

- smoke
- molotov / incendiary
- HE grenade
- flashbang
- decoy

Behavior:

- The target agent is still controlled by `Follow` / `Lock Current`.
- The prop scanner only accepts projectiles thrown or owned by the current target.
- If there is exactly one valid flying projectile and no prop lock, it auto-locks.
- If multiple valid projectiles exist, choose one manually from Recent Props with `LOCK`.
- `CLEAR PROP` clears the current prop lock.
- The camera tracks flying projectile entities only. When the projectile ends, override stops and the Runtime panel shows the reason.

Prop controls:

- `Prop X / Y / Z`: local prop camera offset
- `X`, `Y`, `Z` sliders: medium-sensitivity position sliders from `-64` to `64`
- `Pitch / Yaw / Roll`: angle trim, editable by numeric input or by labeled sliders
- `Pitch`, `Yaw`, `Roll` sliders: medium-sensitivity sliders from `-45` to `45` degrees for quick camera adjustment
- `Recent Props`: candidate list with validity, handle, age, type, and lock state

## Failure States

Read the Runtime panel when the camera is blocked. Common examples:

- no observer target
- target unresolved
- prop not locked
- prop ended
- schema or SetUpView compatibility failure

The DLL writes `selfiestick_trace.log` next to the loaded DLL. If you replace the DLL, fully restart HLAE and CS2 before testing again.
