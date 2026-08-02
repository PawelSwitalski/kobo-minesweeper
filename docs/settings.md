# Settings

Two layers of configuration exist: the in-app Settings screen (currently just the color
mode switch, persisted to `settings.json`) and the environment variables in the launcher
script `.adds/minesweeper/start.sh` on the device, described below.

## In-app Settings screen

Reachable from both the New Game and Board screens. Currently offers one control:

- **Color / Black-and-white**: switches `Theme::color`, which composes the player's choice
  with the device's actual color capability (`DisplayInfo::color`) — a monochrome device
  still renders in grayscale even if "Color" is selected. Every board state and number is
  distinguishable by shape/contrast alone regardless of this setting. Defaults to
  Black-and-white and persists across restarts in `settings.json`.

## Launcher settings (`.adds/minesweeper/start.sh`)

Edit the script on the device over USB and uncomment/set the variables you
need before the `./minesweeper` line.

### Idle auto-exit

```sh
export MINESWEEPER_IDLE_EXIT_SEC=300   # auto-exit after N seconds idle, 0 disables
```

While the app runs, Nickel (the Kobo home software) is paused so it can't
draw over the app or steal input — which also means the power button and
Nickel's own inactivity sleep don't work meanwhile. If you want to sleep the
device immediately, exit the app first. If you just set the device down,
the app exits on its own after 5 minutes without a tap (default), handing
control back to Nickel so its normal sleep timer resumes. Setting `0`
disables the auto-exit — not recommended, since a forgotten paused Nickel
keeps the device awake.

The launcher also pauses `sickel`, Kobo's watchdog daemon (firmware
4.28+), which would otherwise power the device off when it sees Nickel
unresponsive — even mid-session. Both are resumed when the app exits.

### Touch calibration

Kobo touch panels don't all report coordinates in the same orientation as
the screen. If taps land in the wrong place (e.g. the right edge behaves
like the bottom), set the matching variables:

```sh
export MINESWEEPER_TOUCH_SWAP_XY=1    # swap raw x/y first
export MINESWEEPER_TOUCH_MIRROR_X=1   # mirror x after swap
export MINESWEEPER_TOUCH_MIRROR_Y=1   # mirror y after swap
```

To work out which combination your model needs, also set
`MINESWEEPER_TOUCH_DEBUG=1`, tap around, then check `.adds/minesweeper/crash.log`
for `tap raw=(..) -> (..)` lines — raw coordinates that span the screen's
*height* on what should be the *x* axis are the usual sign that `SWAP_XY`
is needed.

**Known limitation:** calibration assumes the device is held in its normal
(non-inverted) portrait orientation — the app doesn't read the
accelerometer, so holding the device upside-down maps taps incorrectly.

## Files on the device

Everything lives in `.adds/minesweeper/` on the USB-visible storage:

| File | Purpose |
|---|---|
| `minesweeper` | The app binary |
| `start.sh` | Launch wrapper (pause/resume Nickel, env settings) |
| `assets/` | Fonts |
| `game.json` | Created by the app at runtime: the in-progress/ended game session |
| `settings.json` | Created by the app at runtime: the color-mode preference |
| `crash.log` | stderr of the last run — first thing to check if something goes wrong |
