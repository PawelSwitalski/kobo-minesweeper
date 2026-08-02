# Quickstart: Validating the Timer Fix & Hide-Timer Setting

## Prerequisites

- Same as `001`/`002`: CMake ≥ 3.20, a C++17 toolchain. No device/toolchain needed — everything
  below runs on the host via the SDL2 desktop simulator plus the host unit test suite.
- A stopwatch or phone timer (or your OS clock) for the manual timing checks in step 2.

## 1. Host unit tests (new + regression)

```bash
cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure
```

**Expected**: all existing suites still pass unchanged (`test_board.cpp`, `test_difficulty.cpp`,
`test_game_session.cpp`, `test_persist.cpp`, `test_smoke.cpp`), plus:

- `test_active_time_tracker.cpp` (new) — including a case that feeds `ActiveTimeTracker::tick()` a
  synthetic sequence of frequent, closely-spaced timestamps (simulating rapid tapping) and asserts
  the summed returned seconds matches the total real interval covered — directly reproducing and
  proving the fix for the bug described in research.md #1.
- `test_settings.cpp` (extended) — `hideTimer` defaults to `false`, round-trips through
  `toJson`/`fromJson`, and a `settings.json` written *without* a `hideTimer` key (simulating an
  already-existing install) still parses successfully with `hideTimer == false` and the correct
  `colorMode` (contracts/persistence-schema.md).

## 2. Desktop simulator — manual scenario walkthrough

```bash
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300
```

Run through each acceptance scenario from `spec.md` directly in the simulator window:

1. **US1 — Timer keeps up during tap-heavy play** (FR-001, FR-003, SC-001, SC-002): Start a new
   Beginner game. Using a stopwatch, play for about 2 minutes with frequent, continuous tapping
   (open cells rapidly, flag and unflag repeatedly) — avoid long pauses. Lose or win the game (or
   open the Settings screen and back out to force a redraw) and compare the displayed elapsed time
   against your stopwatch: it should be within 5 seconds of the real elapsed time. Before this fix,
   the displayed time would be substantially lower.
2. **US1 — Timer still excludes paused time** (FR-002): Start a new game, let a small amount of
   time pass, then open Settings (or return to the main menu without finishing) and wait ~30 real
   seconds there before returning to the board. Confirm the displayed time did not advance while
   away from the board.
3. **US1 — Final time on the outcome screen is accurate** (FR-004): Combine both of the above —
   play with a realistic mix of rapid taps and idle pauses, finish the game (win or lose), and
   confirm the outcome banner's elapsed time is within 5 seconds of your independently tracked real
   elapsed *active* time (excluding any paused/menu time per Scenario 2).
4. **US2 — Hide the timer from Settings** (FR-005, FR-006, SC-003): Open Settings; confirm a "Hide
   Timer" toggle is present and off by default. Tap it on; confirm it visually shows as pressed/on.
   Go back to the board; confirm the timer HUD slot is now blank (no elapsed-time text), while the
   mine counter, HUD buttons, and grid are all still visible and functional.
5. **US2 — Outcome screen still shows the time when hidden** (FR-008): With "Hide Timer" still on,
   finish the current game (win or lose). Confirm the win/loss banner still displays the final
   elapsed time, even though the live HUD timer was hidden throughout play.
6. **US2 — Toggling takes effect immediately, no new game needed** (FR-009): Start a fresh game
   with "Hide Timer" off (timer visible on the board). Without finishing the game, open Settings,
   turn "Hide Timer" on, and go back to the board. Confirm the timer disappears immediately on the
   same in-progress game. Repeat in reverse (turn it back off) and confirm the timer reappears.
7. **US2 — Setting persists across restart** (FR-010, SC-004): With "Hide Timer" on, close the app
   and relaunch it with the same `--data-dir`. Open Settings; confirm "Hide Timer" is still shown
   as on, and the board (if a game is in progress) still has the timer hidden.
8. **US2 — Hiding doesn't affect save/resume** (FR-011): With "Hide Timer" on, play partway through
   a game, close the app, and relaunch with the same `--data-dir`. Confirm the game resumes exactly
   where it was left (same opened/flagged cells) — the fact that the timer was hidden should have no
   bearing on what was saved or restored.

## 3. Regression: existing `001`/`002` flows still work

Quickly re-run a couple of earlier quickstart scenarios (long-press flagging, chording, the Exit
buttons, the win/loss "Return to Menu" flow, per-digit colors in Color mode) to confirm the loop
restructuring and the new setting didn't disturb them — none of this feature's changes touch
`core::Board`/`GameSession` gameplay logic or the `Color`/digit-rendering path, so these should
behave identically to before.
