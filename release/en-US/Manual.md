# Selfiestick DLL Manual

## 1. Package contents

This English release folder contains only:

- `Lyan's selfiestick.dll`
- `Manual.md`
- `selfiestick_bind.cfg`

From this project side, **the only runtime file you actually need is `Lyan's selfiestick.dll`**.  
`Manual.md` and `selfiestick_bind.cfg` are reference files only.

## 2. Required environment

You must already have:

- a working `HLAE`
- a working `CS2`

That means:

- from this package, you only need `Lyan's selfiestick.dll`
- from your own environment, you still need `HLAE + CS2`

## 3. Load the DLL

1. Copy the `en-US` folder to any stable location.
2. Start `HLAE`, then enter `CS2`.
3. Run this in the HLAE console:

```cfg
mirv_loadlibrary "<absolute path to Lyan's selfiestick.dll>"
```

Example:

```cfg
mirv_loadlibrary "D:\tools\selfiestick\en-US\Lyan's selfiestick.dll"
```

If loading succeeds, the console prints:

```text
[selfiestick] ready - press Ins for panel, F8 toggle, F9 lock, F10 follow
```

## 4. Runtime hotkeys

- `Ins`: show / hide the control window by default
- `F8`: enable / disable the selfiestick
- `F9`: lock the current spectator target
- `F10`: return to follow mode

Notes:

- The control window is now a standalone movable, resizable window.
- You can change the panel toggle key inside the window. The new key applies immediately and is saved for the next DLL load.
- `F8 / F9 / F10` only work while the panel is hidden.
- When the panel is visible, the panel captures input.
- Gun-mounted views now damp muzzle shake while firing, so the selfie camera no longer jitters with each shot.
- While following an `AWP`, scope zoom no longer changes the selfiestick camera position or FOV.

## 5. Panel controls

The panel keeps one compact page with:

- enable / disable
- `R / B / U` local trim
- `SELFIE LEFT / SELFIE RIGHT / FORWARD`
- panel hotkey remap button
- `FOLLOW / LOCK CURRENT / CLEAR`
- live runtime status

Trim semantics:

- `R` = Right
- `B` = Back
- `U` = Up

Direction semantics:

- `SELFIE LEFT`: selfie-facing composition from the left side
- `SELFIE RIGHT`: mirrored selfie-facing composition from the right side
- `FORWARD`: current forward-facing mode

## 6. Typical usage flow

1. Enter spectator view.
2. Press the current panel key (`Ins` by default) to open the control window.
3. Enable the selfiestick.
4. Adjust `R / B / U` as needed.
5. Switch between `SELFIE LEFT`, `SELFIE RIGHT`, and `FORWARD`.
6. Use `FOLLOW / LOCK CURRENT / CLEAR` to manage the target.

## 7. Failure states

If the status area shows a failure reason, the DLL is reporting the exact blocking condition instead of hiding it behind fallback behavior.

Common examples:

- `no observer target`
- `attachment missing`
- `target resolution failed`
- `camera direction degenerate`

Read the `Reason` field in the panel for the exact runtime truth.
