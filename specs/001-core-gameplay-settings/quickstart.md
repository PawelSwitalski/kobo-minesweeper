# Quickstart: Validating Core Minesweeper Gameplay & Display Settings

## Prerequisites

- CMake ≥ 3.20, a C++17 toolchain (Windows: MSVC via Visual Studio Build Tools; the SDL2 backend
  fetches a prebuilt dev package automatically on MSVC per `CMakeLists.txt`).
- No device/toolchain needed for any of this — everything below runs on the host.

## 1. Host unit tests (Constitution III gate)

```bash
cmake -B build/host -DMINESWEEPER_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure
```

**Expected**: all suites pass, including (once implemented per tasks.md):
- `test_difficulty.cpp` — preset values match FR-001; custom bounds accept 5–16/1..(w×h−9) and
  reject outside them (FR-002/FR-003).
- `test_board.cpp` — open/cascade/flag/chord/win/loss transitions (User Stories 1–3); a
  many-seed loop asserting the first-opened cell is never a mine across all three presets and
  boundary custom configs (SC-002).
- `test_game_session.cpp` / `test_settings.cpp` — JSON round-trip is lossless; malformed/invalid
  JSON is rejected via a thrown exception (mirrors `test_counter.cpp`'s existing cases).

## 2. Desktop simulator — manual scenario walkthrough

```bash
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300
```

Run through each acceptance scenario from `spec.md` directly in the simulator window (mouse click
= tap; click-and-hold ≥ 500ms = long-press, per `contracts/platform-touch-input.md`):

1. **US1 (win/loss)**: Start Beginner. Confirm the first cell you open is never a mine (SC-002).
   Open cells until you either clear the board (win banner + difficulty + elapsed time shown,
   FR-017/FR-018) or hit a mine (loss banner, board actions now inert, FR-008/FR-018).
2. **US2 (flagging)**: Long-press an unopened cell → it flags; long-press again → unflags.
   Confirm a flagged cell doesn't open on a plain tap. Toggle Flag Mode on; confirm a plain tap
   now flags instead of opening, and long-press still flags too (FR-009/FR-010). Watch the
   remaining-mine counter go negative if you over-flag (FR-015).
3. **US3 (chording)**: Flag exactly the mines around an opened numbered cell, tap it again →
   remaining safe neighbors open at once. Deliberately mis-flag one, chord → loss (FR-012/FR-013).
4. **US4 (color mode)**: Open Settings, switch to Black-and-white; confirm every cell
   state/number is still readable by shape/contrast alone (SC-003). Close and relaunch the app;
   confirm the mode is still Black-and-white (SC-005).
5. **US5 (custom board)**: From New Game, use the +/- steppers to pick a custom width/height/mine
   count inside 5–16/1..(w×h−9); confirm it starts. Try to push a stepper past its bound; confirm
   it clamps AND shows a reason message stating the valid range (FR-002/FR-003) — not just a
   silent refusal to change the value.
6. **Resume across restart**: Mid-game, close the app (Ctrl+C or window close) and relaunch with
   the same `--data-dir`; confirm the board is exactly as left — same opened/flagged cells, same
   elapsed time (SC-004, FR-022).
7. **Abandon confirmation**: Mid-game, go to New Game and pick a preset; confirm the "Abandon
   current game?" dialog appears; Cancel leaves the game untouched, Confirm starts the new one
   (FR-024).
8. **Elapsed-time pause**: Mid-game, switch to Settings (or just leave the app idle past a
   `--data-dir` relaunch) and confirm the timer does not advance while off the Board screen or
   while the app is closed, only while actively playing (clarified FR-016 semantics).
9. **Hardware-monochrome run (FR-021)**: Relaunch with `--gray` (simulates a monochrome device):
   confirm the color-mode control in Settings is still visible and toggleable even though the
   device itself can't render color, and that the board remains fully legible exactly as in
   scenario 4.

## 3. Corrupted-save recovery (Constitution V / FR-023)

```bash
# with the sim not running:
echo "not json" > <data-dir>/game.json
build/sim/Release/minesweeper --width 1264 --height 1680 --dpi 300 --data-dir <data-dir>
```

**Expected**: app starts cleanly at New Game selection (no crash), `settings.json` (if present
and valid) is unaffected — confirm the color mode you set in step 2 is still applied.
