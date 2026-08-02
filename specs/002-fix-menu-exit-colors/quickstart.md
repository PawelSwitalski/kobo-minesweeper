# Quickstart: Validating Menu Layout, Exit Controls, and Mine-Count Colors Fixes

## Prerequisites

- Same as `001-core-gameplay-settings`: CMake ≥ 3.20, a C++17 toolchain. No device/toolchain
  needed — everything below runs on the host via the SDL2 desktop simulator.
- No new tests are added by this feature (see plan.md's Technical Context / Constitution Check —
  no new core logic exists to unit-test); running the existing host suite is still a useful
  regression check that nothing in `src/core/` was accidentally touched.

## 1. Host unit tests (regression check)

```bash
cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure
```

**Expected**: all existing suites still pass unchanged (`test_board.cpp`, `test_difficulty.cpp`,
`test_game_session.cpp`, `test_settings.cpp`, `test_persist.cpp`, `test_smoke.cpp`) — this feature
adds no new test files and touches no file under `src/core/`.

## 2. Desktop simulator — manual scenario walkthrough

```bash
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300
```

Run through each acceptance scenario from `spec.md` directly in the simulator window:

1. **US1 — Exit from the main menu** (FR-001, FR-014, SC-001): On launch (fresh `--data-dir`),
   confirm an "Exit" button is visible on the New Game screen. Tap it; confirm the app closes
   immediately with no confirmation dialog.
2. **US1 — Exit mid-game** (FR-002, FR-003, FR-014, SC-002, SC-004): Start Beginner, open a few
   cells (don't finish), tap the board screen's "Exit" button; confirm the app closes immediately.
   Relaunch with the same `--data-dir`; confirm the board resumes exactly as left (same
   opened/flagged cells, same elapsed time) — no progress lost.
3. **US1 — Settings has no Exit, but Exit stays reachable** (FR-015): From the board screen open
   Settings; confirm Settings shows only "Back" (no Exit button there). Tap Back; confirm you're
   back on a screen with a working Exit button.
4. **US2 — Recover after a win** (FR-004, FR-005, FR-006, SC-003): Play a game to completion
   (win). On the win banner, confirm both "Return to Menu" and "Exit" buttons are visible. Tap
   "Return to Menu"; confirm the New Game screen appears. Close and relaunch the app (same
   `--data-dir`); confirm it opens on the New Game screen, not back on the finished board.
5. **US2 — Recover after a loss** (FR-004, FR-005, SC-003): Start a new game and open a mine to
   lose. Confirm the loss banner also shows both "Return to Menu" and "Exit," and that "Exit"
   closes the app immediately from there too.
6. **US3 — New Game menu grouping** (FR-007, FR-008, FR-009, SC-005): On the New Game screen,
   confirm Beginner/Intermediate/Expert/Settings/Exit read as one grouped block, followed by a
   visibly larger gap, followed by the "Custom" section (header + width/height/mine steppers +
   "Start Custom Game"). Confirm all preset buttons, the Exit button, and every Custom
   adjuster/start button still work exactly as before.
7. **US4 — Per-digit colors** (FR-010, FR-011, FR-012, SC-006): In Settings, switch to Color mode.
   Start a board large enough to expose several different numbers (Expert is an easy way to see
   most of 1–8 quickly). Confirm: `1` is blue, `2` is green, `3` is red, `4` is a deeper/darker
   blue than `1`, `5` is a dark cherry-red distinguishable from plain `3`, `6` is cyan, `7` is
   black, `8` is a visible mid-gray. Switch back to Black & White mode; confirm every digit reverts
   to plain black text, identical to `001`'s pre-existing behavior.
8. **Hardware-monochrome run** (FR-011): Relaunch with `--gray` (simulates a monochrome device).
   Confirm Color mode, if selected in Settings, has no visible effect — digits stay plain black,
   exactly as in Black & White mode — while Exit/Return-to-Menu controls and the menu layout are
   unaffected by this flag (they're not color-dependent features).

## 3. Regression: existing `001` flows still work

Quickly re-run a couple of `001`'s own quickstart scenarios (long-press flagging, chording, the
abandon-in-progress-game confirmation when picking a new difficulty from the New Game screen) to
confirm the button/layout changes in this feature didn't disturb them — none of this feature's
changes touch `core::Board`/`GameSession` gameplay logic, so these should behave identically to
before.
