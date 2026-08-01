#!/bin/sh
# KFMon launch wrapper (docs/contracts/install-layout.md).
# stderr goes to crash.log (truncated each run) for field debugging.
cd /mnt/onboard/.adds/minesweeper || exit 1

# Touch-panel calibration overrides, if this model needs them (see docs/settings.md):
# export MINESWEEPER_TOUCH_SWAP_XY=1
# export MINESWEEPER_TOUCH_MIRROR_X=1
# export MINESWEEPER_TOUCH_MIRROR_Y=1
# export MINESWEEPER_TOUCH_DEBUG=1
# export MINESWEEPER_IDLE_EXIT_SEC=300   # auto-exit after N seconds idle, 0 disables

# Nickel keeps running in the background otherwise (we only take over touch
# input, not the screen): it periodically repaints its own UI (e.g. the
# status-bar clock) over ours. Freeze it for the duration -- the standard
# Kobo homebrew pattern. Nickel does NOT repaint on SIGCONT; the app
# snapshots the framebuffer at startup and restores it right before exiting,
# so the screen Nickel left behind is back in place by the time it resumes.
# NOTE: while frozen, Nickel can't react to the power button or its own
# inactivity timer, so the app auto-exits after 5 min idle (see above) to
# hand control back before anything forces a hard power-off.
#
# Since FW 4.28 Kobo also runs "sickel", a watchdog that powers the device
# off/reboots it when nickel stops responding -- which a frozen nickel does,
# no matter how actively the user is using the app. Freeze it first so it
# never sees nickel unresponsive; thaw it last, after nickel is live again.
# Give Nickel a beat to finish repainting after the NickelMenu popup closes;
# freezing it mid-repaint would make the app snapshot (and later restore) a
# screen with the menu still open.
sleep 1

SICKEL_PID=$(pidof sickel)
[ -n "$SICKEL_PID" ] && kill -STOP "$SICKEL_PID"
NICKEL_PID=$(pidof nickel)
[ -n "$NICKEL_PID" ] && kill -STOP "$NICKEL_PID"

./minesweeper 2> crash.log

[ -n "$NICKEL_PID" ] && kill -CONT "$NICKEL_PID"
[ -n "$SICKEL_PID" ] && kill -CONT "$SICKEL_PID"
exit 0
