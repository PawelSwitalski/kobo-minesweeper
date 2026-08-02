---

description: "Task list for Fix Game Timer & Add Hide-Timer Setting"
---

# Tasks: Fix Game Timer & Add Hide-Timer Setting

**Input**: Design documents from `/specs/003-fix-timer-hide-option/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md (all present)

**Tests**: This feature adds new `core::` logic (`ActiveTimeTracker`) and extends an existing
`core::` type (`Settings`), both of which Constitution Principle III (NON-NEGOTIABLE) requires
host-run doctest coverage for — plan.md's Technical Context and Constitution Check call this out
explicitly, so test tasks ARE included for User Story 1's new type and User Story 2's `Settings`
extension. The remaining changes (the `main.cpp` loop restructuring, `SettingsScreen`'s new button,
`BoardScreen`'s display guards) live in `main.cpp`/`src/ui/`, neither of which is linked into
`minesweeper_tests` (`CMakeLists.txt`) — same as `001`/`002`, these are verified via their
corresponding manual scenario in `quickstart.md` instead.

**Organization**: Tasks are grouped by user story (spec.md priorities P1, P2). There is no Setup or
Foundational phase in the usual sense — see the note below — so numbering starts directly at the
first user story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on another incomplete task in the
  same batch)
- **[Story]**: Which user story this task belongs to (US1–US2, per spec.md)
- File paths are relative to the repository root

## Path Conventions

Single project, existing layout: `src/core/`, `src/ui/`, `src/main.cpp`, `tests/` — see plan.md's
Project Structure. One new file pair is added (`src/core/active_time_tracker.h/.cpp`) plus its test
(`tests/test_active_time_tracker.cpp`); every other task edits a file that already exists.

---

## Phase 1: Setup — not applicable

No project initialization or new dependencies are needed; `CMakeLists.txt` only needs two new
source-file registrations (within User Story 1, see below).

## Phase 2: Foundational — not applicable

There is no cross-story shared prerequisite: User Story 1 (`ActiveTimeTracker`) and User Story 2
(`Settings::hideTimer`) touch entirely disjoint files and neither depends on the other's code to
function or to be tested. The one file either story could in principle collide on, `CMakeLists.txt`,
is only touched by User Story 1 (registering the two new files); User Story 2 adds no new files, so
it never needs a `CMakeLists.txt` change at all.

---

## Phase 3: User Story 1 - Timer accurately tracks play time (Priority: P1) 🎯 MVP

**Goal**: The displayed and persisted elapsed play time keeps up with real wall-clock time during
active play, including tap-heavy stretches, and still correctly excludes paused/menu/sleep time.

**Independent Test**: quickstart.md §1 (host tests) plus §2 scenarios 1–3 — play with a mix of
rapid tapping and idle pauses; the final elapsed time (shown on the win/loss outcome screen) is
within 5 seconds of independently tracked real active-play time, and time spent paused at the menu
or in Settings is correctly excluded.

### Implementation for User Story 1

- [X] T001 [US1] Implement `core::ActiveTimeTracker` — constructor taking a starting
      `std::chrono::steady_clock::time_point`, and a `tick(now, counts)` method that returns whole
      seconds elapsed since the previous call (0 if `counts` is false) and unconditionally advances
      its internal reference point to `now` — in `src/core/active_time_tracker.h` and
      `src/core/active_time_tracker.cpp`, per contracts/active-time-tracker.md's invariants
- [X] T002 [P] [US1] Register `src/core/active_time_tracker.cpp` as a new source file in the
      `minesweeper_core` library target in `CMakeLists.txt` (depends on T001)
- [X] T003 [P] [US1] Add `tests/test_active_time_tracker.cpp` with doctest cases covering: (a) a
      tap-heavy synthetic timestamp sequence (frequent, closely-spaced `tick()` calls) where the sum
      of returned seconds matches the total real interval covered — directly reproducing and proving
      the fix for the bug in research.md #1; (b) `counts=false` calls (simulating paused/menu/sleep
      time) return 0 but still advance the internal reference point so the next counted interval
      isn't inflated; (c) sub-second remainders are truncated per call without compounding drift
      across calls — per contracts/active-time-tracker.md and quickstart.md §1 (depends on T001)
- [X] T004 [US1] Register `tests/test_active_time_tracker.cpp` in the `minesweeper_tests` executable
      sources in `CMakeLists.txt` (depends on T003; same file as T002 — sequence after it)
- [X] T005 [P] [US1] Restructure the event loop in `src/main.cpp`: construct a
      `core::ActiveTimeTracker` from the existing `lastTickSteady` starting point, compute
      `activeSeconds` via `tick(nowSteady, !slept && screen->countsPlayTime())` and call
      `screen->onTick(activeSeconds)` unconditionally on every iteration (immediately after the
      existing sleep/redraw repaint check, before branching on `tap`) instead of only on iterations
      without a tap; remove the now-redundant bare `lastTickSteady` local and its unconditional
      `lastTickSteady = nowSteady;` line; leave the `if (tap) {...} else {...idle-exit watchdog...}`
      branching otherwise unchanged — per contracts/active-time-tracker.md's `main.cpp` call-site
      delta (depends on T001)

**Checkpoint**: `ctest` passes including the new `test_active_time_tracker.cpp` suite, and
quickstart.md §2 scenarios 1–3 pass in the SDL simulator — the timer no longer falls behind during
tap-heavy play, and paused time is still correctly excluded.

---

## Phase 4: User Story 2 - Hide the timer during play (Priority: P2)

**Goal**: A "Hide Timer" setting, off by default, removes the live elapsed-time HUD from the board
while a game is in progress, without affecting the final time shown on the outcome screen or how
elapsed time is tracked/persisted internally.

**Independent Test**: quickstart.md §2 scenarios 4–8 — enabling the setting hides the live board
timer immediately (no new game needed) while the outcome screen still shows the final time, the
setting persists across restart, and save/resume is unaffected by whether the timer is hidden.

### Implementation for User Story 2

- [X] T006 [US2] Add `bool hideTimer = false;` to `core::Settings`; update `toJson()` to always
      write the field and `fromJson()` to read it via `j.value("hideTimer", false)` (absent means
      `false`, so an already-existing `settings.json` written before this feature still parses
      successfully) in `src/core/settings.h` and `src/core/settings.cpp`, per
      contracts/persistence-schema.md and research.md #4
- [X] T007 [P] [US2] Extend `tests/test_settings.cpp` with cases: `Settings` defaults to
      `hideTimer == false`; `toJson`/`fromJson` round-trips `hideTimer` losslessly for both `true`
      and `false`; a hand-built `settings.json` string with a valid `schemaVersion`/`colorMode` but
      **no** `hideTimer` key still parses successfully with `hideTimer == false` and the correct
      `colorMode` (simulating an install that predates this feature) (depends on T006)
- [X] T008 [P] [US2] Add a `hideTimerButton_` `Button` field to `SettingsScreen`, laid out as its
      own row below the existing Color/Black-and-white buttons and above the Back button; its
      `toggled` state mirrors `app_.settings().hideTimer`; on tap, flip the setting, call
      `app_.autosaveSettings()`, redraw, and `flushFull()` — mirroring the single-toggle pattern
      `BoardScreen::flagModeButton_` already uses (simpler than the two-button mutually-exclusive
      color-mode pattern, since this is a plain boolean) — in `src/ui/screens/settings_screen.h`
      and `src/ui/screens/settings_screen.cpp` (depends on T006)
- [X] T009 [P] [US2] In `BoardScreen::drawTimer()`, keep the unconditional
      `r.fillRect(timerRect_, Gray::White)` but only construct and draw the elapsed-time `Label`
      when `!app_.settings().hideTimer`; in `BoardScreen::onTick()`, only perform the
      minute-boundary `drawTimer()` + `flushPartial(timerRect_)` call when
      `!app_.settings().hideTimer` (the underlying `addActiveSeconds(activeSeconds)` and
      `autosaveSession()` calls stay unconditional, per FR-011) — in
      `src/ui/screens/board_screen.cpp`; do **not** touch `drawOutcomeBanner()`, which must keep
      showing the final elapsed time regardless of `hideTimer` per FR-008/research.md #3
      (depends on T006)

**Checkpoint**: `ctest` passes including the extended `test_settings.cpp` cases, and
quickstart.md §2 scenarios 4–8 pass — the setting toggles the live HUD timer immediately, the
outcome screen is unaffected, and the preference persists across restart without disturbing
save/resume.

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Full end-to-end validation now that both fixes are in place together.

- [X] T010 Run the full `quickstart.md` validation guide end-to-end: the host `ctest` run (§1,
      including both new/extended test suites), all 8 simulator scenarios (§2), and the `001`/`002`
      regression spot-check (§3) — fix any discrepancy found (depends on T001–T009)
      **Partial**: §1 verified directly — `cmake --build build/host` (both `minesweeper_ui` and
      `minesweeper_tests`) and `ctest --test-dir build/host` pass (1/1 suites), including the new
      `test_active_time_tracker.cpp` (5 cases) and the extended `test_settings.cpp` `hideTimer`
      cases. This run caught a real bug in the first `ActiveTimeTracker` implementation — it reset
      its reference point fully to `now` on every counted call, which truncated every sub-second
      interval to zero and lost it permanently (exactly the class of bug this feature fixes); fixed
      by carrying the sub-second remainder forward instead of discarding it (see
      contracts/active-time-tracker.md, updated). The full SDL simulator build (`build/sim`,
      `MINESWEEPER_BACKEND=sdl`) also compiles and links clean, and the resulting `minesweeper.exe`
      launches and stays running (smoke-tested via a backgrounded run, confirmed still alive after
      2s). §2's 8 scenarios require real touch/mouse interaction with the SDL window, which isn't
      available in this CLI environment — same limitation `001`/`002`'s own `tasks.md` recorded.
      **These should be run manually before release**; see quickstart.md §2 for the exact steps.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup / Foundational (Phase 1–2)**: Not applicable — see notes above.
- **User Story 1 (Phase 3)**: No dependency on User Story 2. Can start immediately.
- **User Story 2 (Phase 4)**: No dependency on User Story 1 — touches entirely different files
  (`core/settings.*`, `ui/screens/settings_screen.*`, `ui/screens/board_screen.cpp`'s
  `drawTimer()`/`onTick()`) than User Story 1 (`core/active_time_tracker.*`, `main.cpp`,
  `CMakeLists.txt`). Can start immediately, in parallel with User Story 1.
- **Polish (Phase 5)**: Depends on both Phases 3 and 4.

### Within Each Story

- US1: T001 (the class itself) blocks T002, T003, and T005, each of which then has no further
  dependency on one another (different files: `CMakeLists.txt`, `tests/test_active_time_tracker.cpp`,
  `src/main.cpp` respectively) — all three can proceed in parallel once T001 lands. T004 depends on
  T003 (test file must exist) and shares `CMakeLists.txt` with T002 (sequence after it).
- US2: T006 (the `Settings` field) blocks T007, T008, and T009, each of which then has no further
  dependency on one another (different files: `tests/test_settings.cpp`,
  `src/ui/screens/settings_screen.*`, `src/ui/screens/board_screen.cpp` respectively) — all three
  can proceed in parallel once T006 lands.

### Parallel Opportunities

- T001 (US1) and T006 (US2) — different stories, different files, no shared dependency — can start
  immediately and in parallel with each other.
- Once T001 lands: T002, T003, T005 (US1) can all proceed in parallel.
- Once T006 lands: T007, T008, T009 (US2) can all proceed in parallel.

---

## Parallel Example: User Story 1

```bash
# After T001 (core::ActiveTimeTracker) lands:
Task: "Register active_time_tracker.cpp in CMakeLists.txt"
Task: "Write tests/test_active_time_tracker.cpp"
Task: "Restructure the event loop in src/main.cpp"
```

## Parallel Example: User Story 2

```bash
# After T006 (Settings::hideTimer) lands:
Task: "Extend tests/test_settings.cpp for hideTimer"
Task: "Add hideTimerButton_ to SettingsScreen"
Task: "Guard BoardScreen::drawTimer()/onTick() behind hideTimer"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 3: User Story 1 — a timer that reliably tracks real play time is the correctness fix the
   user actually reported; it stands alone and delivers value with zero UI changes.
2. **STOP and VALIDATE**: `ctest` (new suite) + quickstart.md §2 scenarios 1–3.
3. This alone is a shippable bug fix even before User Story 2 lands.

### Incremental Delivery

1. + User Story 1 → the timer is trustworthy again. Demo-able, ships the reported bug fix.
2. + User Story 2 → players who find the ticking timer distracting can turn it off. Demo-able.
3. Phase 5 → full validation pass.

### Notes

- [P] tasks touch different files and have no unmet dependency on each other.
- Commit after each task or logical group; stop at either checkpoint to validate a story
  independently in the SDL simulator before moving on.
- Avoid: bumping `settings.json`'s `schemaVersion` — `hideTimer` is additive with a spec-mandated
  default, handled via `j.value("hideTimer", false)`, not a breaking schema change (research.md #4).
- Avoid: touching `BoardScreen::drawOutcomeBanner()` — it must keep showing the final elapsed time
  regardless of `hideTimer` (FR-008), and already does so without any change.
