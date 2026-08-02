# Quickstart: Validating the Screen Refresh Frequency Setting

## Prerequisites

- Same as prior features: CMake ≥ 3.20, a C++17 toolchain. Steps 1–2 below run on the host via the
  SDL2 desktop simulator; step 3 requires an actual Kobo device (or careful visual inspection of
  the FBInk build's logic), since the SDL backend never exhibits visible ghosting.

## 1. Host unit tests (new + regression)

```bash
cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure
```

**Expected**: all existing suites still pass unchanged, plus `test_settings.cpp` (extended) —
`screenRefreshInterval` defaults to `Every10`; round-trips losslessly for all four values; a
`settings.json` written *without* a `screenRefreshInterval` key (simulating an already-existing
install) still parses successfully with `screenRefreshInterval == Every10` and the correct
`colorMode`/`hideTimer` (contracts/persistence-schema.md).

## 2. Desktop simulator — UI and persistence (no visible ghosting effect)

```bash
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300
```

1. **US1 — Setting is visible and selectable** (FR-001): Open Settings. Confirm a "Screen Refresh"
   section is present below "Hide Timer," with four buttons: "5", "10", "25", "Never" — and "10" is
   shown as selected by default on a fresh install.
2. **US1 — Selection persists** (FR-002, SC-004): Select "25". Close and relaunch the app with the
   same `--data-dir`. Open Settings; confirm "25" is still selected.
3. **US1 — No visible effect on the simulator** (FR-007): Confirm selecting any value doesn't
   visibly change simulator behavior (no flashing pattern difference) — this is expected, since
   `SdlRenderer` never overrides `setGhostingInterval` (research.md #1's "no-op" branch of
   `Renderer`'s base contract).
4. **Regression**: Confirm Color/Black-and-white and Hide Timer still work exactly as before — this
   feature adds a new row below them without disturbing their layout or behavior.

## 3. Kobo device — observing the actual refresh cadence (FR-004, FR-005, SC-002, SC-003)

This is the only way to observe the feature's actual effect, since ghosting and full-refresh
flashing are e-ink-specific phenomena the desktop simulator cannot reproduce.

1. Select "Every 5" in Settings. Start a game and open cells one at a time, counting taps. Confirm
   a visible full-screen flash (the "clean" refresh) occurs on (or very near) every 5th board
   update.
2. Repeat with "Every 10" and "Every 25", confirming the flash cadence scales accordingly.
3. Select "Never". Play an extended session (well past 25 board updates) without switching screens
   or finishing the game. Confirm no automatic ghosting-driven full flash occurs — while ghosting
   may become visibly more noticeable over time, that's the expected tradeoff this setting exists
   to let the player choose (FR-005).
4. With "Never" still selected, finish the game (win or lose) and confirm the win/loss outcome
   banner still triggers its own full refresh as usual (`BoardScreen::afterMutation`'s existing
   `flushFull()` on game end) — proving "Never" only suppresses the *ghosting-driven* auto-promotion,
   not every full refresh in the app (FR-005, Edge Cases).
5. With any value selected, navigate from the board to Settings and back; confirm the normal
   screen-transition full refresh still happens both ways, unaffected by this setting.

## 4. Regression: existing flows still work

Quickly re-run a couple of earlier quickstart scenarios (chording, Exit controls, the hide-timer
toggle, per-digit colors) to confirm this feature's changes — confined to `Settings`,
`ui::applyScreenRefreshInterval`, `SettingsScreen`, and two new call sites in `main.cpp` — didn't
disturb anything else.
