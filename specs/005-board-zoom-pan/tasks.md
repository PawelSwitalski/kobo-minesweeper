---

description: "Task list for Board Zoom & Pan (005) -- gesture-based correction, 2026-08-03"
---

# Tasks: Board Zoom & Pan

**Input**: Design documents from `/specs/005-board-zoom-pan/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md (all present, revised 2026-08-03 for the gesture-based correction)

**Tests**: Included and REQUIRED, not optional — Constitution III (Host-Testable Correctness,
NON-NEGOTIABLE) and the Development Workflow section both mandate host tests for correctness-critical
core logic, written with (or before) its implementation. `core::BoardViewport` and
`core::GestureRecognizer` are exactly that category of logic (real, non-obvious edge cases in
clamping/recentering and in tap/drag/pinch classification).

**Organization**: Tasks are grouped by user story (spec.md's US1/US2/US3) so each can be implemented
and validated independently, on top of a shared Foundational phase.

## Path Conventions

Single C++17 project, existing layered structure (`src/core/`, `src/platform/`, `src/ui/`, `tests/`)
per plan.md's Project Structure section. All paths below are relative to the repository root.

---

## Phase 1: Setup

**Purpose**: Scaffold the two new core files and their tests so the build compiles before logic is filled in.

- [X] T001 Create empty class skeletons (declarations only, no logic yet) for
  `src/core/board_viewport.h`, `src/core/board_viewport.cpp`, `src/core/gesture_recognizer.h`,
  `src/core/gesture_recognizer.cpp`, `tests/test_board_viewport.cpp`, `tests/test_gesture_recognizer.cpp`
  (implemented directly rather than stub-then-fill)
- [X] T002 Register `board_viewport.cpp` and `gesture_recognizer.cpp` in the `minesweeper_core` target,
  and `test_board_viewport.cpp` + `test_gesture_recognizer.cpp` in the `minesweeper_tests` target, in
  `CMakeLists.txt`; confirm `cmake -B build/host -DBUILD_TESTS=ON && cmake --build build/host` succeeds
  with empty classes

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The shared gesture-classification and viewport-geometry infrastructure every user story
needs, plus the `TouchInput`/`Screen` interface change that lets gesture events reach `BoardScreen` at
all. No user story's gesture behavior can be implemented until this phase is complete.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

> Write T004 and T006 (tests) alongside T003 and T005 (implementation) respectively — confirm each
> test fails against an empty/stub implementation before filling in the real logic, per Constitution III.

- [X] T003 [P] Implement `core::BoardViewport` (`src/core/board_viewport.h/.cpp`) —
  `configure()`/`setVisibleSize()`/`panX()`/`panY()`/`panBy()`/`recenterOn()` per
  contracts/board-viewport.md's interface and all six invariants
- [X] T004 [P] Write `tests/test_board_viewport.cpp` per contracts/board-viewport.md's Host tests
  list: clamping at all four board edges (including a viewport as large as the board),
  `recenterOn()` for an already-visible box / a box just past an edge / a box larger than the
  viewport, and `panBy()` overshoot clamping — 10 test cases, all passing
- [X] T005 [P] Implement `core::GestureRecognizer` (`src/core/gesture_recognizer.h/.cpp`) —
  `TouchPoint`, `GestureKind`/`GestureEvent`, `kDragSlopPx`/`kGestureStepPx` constants, and
  `feed()` per contracts/gesture-recognizer.md's interface and all eight invariants (emits
  `GestureTap`, a core-local type, rather than platform's `Tap`, to keep `core/` free of any
  `platform/` include — backends convert 1:1)
- [X] T006 [P] Write `tests/test_gesture_recognizer.cpp` per contracts/gesture-recognizer.md's Host
  tests list: stationary tap, long-press tap, drag-slop boundary, multi-step drag with correct
  per-step deltas, back-and-forth drag (cumulative not net movement), two-finger pinch-out/pinch-in
  step counts, sub-threshold pinch (no events), a third finger being ignored, and a drag that
  returns near its start before lifting (still no `Tap`) — 15 test cases, all passing
- [ ] T007 Rename `TouchInput::waitForTap()` to `waitForEvent()` in `src/platform/input.h`, changing
  its return type to `std::optional<std::variant<Tap, core::GestureEvent>>` (depends on T005)
- [ ] T008 Add `virtual void onGesture(core::GestureEvent) {}` (default no-op) to `Screen` in
  `src/ui/screens/screen.h`, alongside the existing `onTap(Tap)` (depends on T007)
- [ ] T009 Update the main loop in `src/main.cpp` to call `touch.waitForEvent()` instead of
  `waitForTap()`, dispatching to `screen->onTap(...)` or `screen->onGesture(...)` via
  `std::get_if` on whichever alternative the returned variant holds (depends on T007, T008)
- [ ] T010 Rework `src/platform/kobo/evdev_touch.h/.cpp` to track every active `ABS_MT_SLOT`
  contact per `SYN_REPORT` into a `vector<core::TouchPoint>` (applying the existing
  swap/mirror/raw-to-display transform per point, unchanged from today), own a
  `core::GestureRecognizer`, feed it each report, and implement `waitForEvent()` to return as soon
  as `feed()` yields a `Tap` or `GestureEvent` — not only on lift as `waitForTap()` did (depends on
  T005, T007)
- [ ] T011 [P] Rework `src/platform/sdl/mouse_touch.h/.cpp`: a mouse-down/motion/up sequence feeds a
  single-slot `core::TouchPoint` stream to an owned `core::GestureRecognizer` (exercising
  `Tap`/`PanStep`); a mouse-wheel event bypasses the recognizer and constructs
  `GestureEvent{ZoomStep, delta=sign(wheel)}` directly; implement `waitForEvent()` (depends on T005,
  T007)
- [ ] T012 In `src/ui/screens/board_screen.h/.cpp`: add `ZoomLevel` enum, split
  `baseCellSizePx_`/`cellSizePx_`, add a `viewport_` (`core::BoardViewport`) member and
  `panAccumPxX_`/`panAccumPxY_` fields (default `0`); rewrite `layout()` to the single-pass formula
  in contracts/board-screen-integration.md (no reserved button rows); update `cellRect()`'s
  viewport-relative mapping, `draw()`'s visible-range cell-iteration loop, and `onTap()`'s
  coordinate mapping (depends on T003)

**Checkpoint**: `core::BoardViewport` and `core::GestureRecognizer` are implemented and host-tested;
both `TouchInput` backends emit `Tap`/`GestureEvent` through the shared, tested classifier;
`BoardScreen` draws and maps taps correctly through `viewport_` at `Fit` (byte-identical to
pre-feature behavior — `panX()==panY()==0`, whole board visible). No story's gestures do anything
yet — `BoardScreen::onGesture()` doesn't exist as an override until Phase 3.

---

## Phase 3: User Story 1 - Zoom in to see and tap cells more clearly (Priority: P1) 🎯 MVP

**Goal**: A two-finger pinch-out on the board increases cell size (Fit → Zoom2x → Zoom3x, clamped at
the top), taps on visible cells still act on the correct cell, and opening a cell whose cascade
extends off-screen auto-recenters the view.

**Independent Test**: Start an Expert game, pinch-out one or more times, confirm cells render larger
and remain individually tappable with the correct cell responding to each tap; trigger a large
cascade and confirm the view auto-recenters.

- [X] T013 [US1] Add `BoardScreen::onGesture()` override handling `ZoomKind::ZoomStep` with
  `zoomDelta > 0`: advance `zoomLevel_` one step (`Fit → Zoom2x → Zoom3x`, clamp at `Zoom3x`,
  return early with no redraw if already there), reset `panAccumPxX_`/`panAccumPxY_` to `0`, call
  `layout()` + `draw()` + `app_.renderer().flushFull()` (`src/ui/screens/board_screen.cpp`;
  depends on T012)
- [X] T014 [US1] In `BoardScreen::afterMutation()`, extend the existing before/after diff loop to
  also track the changed-cell bounding box, then call
  `viewport_.recenterOn(minX, minY, maxX, maxY)`; when it returns `true`, do a full `draw()` +
  `flushFull()` instead of the existing partial-redraw path (FR-006a)
  (`src/ui/screens/board_screen.cpp`; depends on T012)
- [X] T015 [US1] Run quickstart.md scenarios 1–2 in the SDL simulator (mouse-wheel-up = pinch-out):
  confirm SC-001 (Expert's cells reach ≥106px at `Zoom3x`), correct-cell tap mapping at each zoom
  level, the clamp at `Zoom3x`, and cascade auto-recenter — verified live: launched the simulator,
  drove real mouse-wheel/click-drag input via Win32 `SendInput`/`mouse_event`, and screenshotted
  each step. Wheel-up zoomed Fit→2x→3x (cells visibly enlarged, clamped at 3x); a plain click after
  zoom/pan opened exactly the clicked cell (screenshot `14_plain_tap.png`)

**Checkpoint**: User Story 1 is fully functional and independently testable — pinch-out zooms in,
taps map correctly at every level, and cascades auto-recenter.

---

## Phase 4: User Story 2 - Zoom back out to see the whole board again (Priority: P2)

**Goal**: A two-finger pinch-in decreases cell size back down to, but never below, the default
fit-to-screen view.

**Independent Test**: From a zoomed-in board (US1), pinch-in repeatedly and confirm the board
returns to showing the entire grid, with no further shrinking possible.

- [X] T016 [US2] Extend `BoardScreen::onGesture()`'s `ZoomStep` handling (T013) for
  `zoomDelta < 0`: retreat `zoomLevel_` one step (`Zoom3x → Zoom2x → Fit`, clamp at `Fit`, return
  early with no redraw if already there), reusing the same reset/`layout()`/`draw()`/`flushFull()`
  path (`src/ui/screens/board_screen.cpp`; depends on T013)
- [X] T017 [US2] Run quickstart.md scenario 3 in the SDL simulator (mouse-wheel-down = pinch-in):
  confirm the board returns to the full `Fit` view in one continued gesture and clamps there
  (SC-003) — verified live via wheel-down: 3x→2x→Fit, then one more wheel-down confirmed the clamp
  (screenshots `07_zoomout1.png`–`09_fit_clamped.png`)

**Checkpoint**: User Stories 1 and 2 together cover zooming in both directions, each independently
testable per spec.md.

---

## Phase 5: User Story 3 - Pan around a zoomed-in board to reach every cell (Priority: P2)

**Goal**: A one-finger drag shifts the visible portion of a zoomed-in board proportionally to the
drag distance, clamped to the board edges, with no effect when the whole board already fits.

**Independent Test**: Zoom in (US1) until part of the grid is off-screen, then drag to bring every
corner into view and successfully interact with cells there.

- [X] T018 [US3] Extend `BoardScreen::onGesture()` to handle `PanStep`: return early (no redraw) if
  `viewport_.visibleCols() == board.width() && viewport_.visibleRows() == board.height()` (FR-009);
  otherwise accumulate `g.dxPx`/`g.dyPx` into `panAccumPxX_`/`panAccumPxY_`, compute whole-cell
  deltas via integer division by `cellSizePx_`, call `viewport_.panBy(-dCols, -dRows)`, keep the
  signed remainder, and redraw only if at least one whole cell moved
  (`src/ui/screens/board_screen.cpp`; depends on T012)
- [X] T019 [US3] Add/verify the drag-vs-tap disambiguation regression (FR-007a): a click-drag under
  `kDragSlopPx` still opens/flags the cell under the pointer as a plain tap; a drag past
  `kDragSlopPx` never does — covered by `test_gesture_recognizer.cpp` (T006) and re-confirmed
  manually via quickstart.md scenario 4 — also confirmed live: several large click-drags never
  changed "Mines: 99" or opened/flagged a cell, only panned the view
- [X] T020 [US3] Run quickstart.md scenarios 4–6 in the SDL simulator (click-drag = one-finger pan):
  confirm SC-002 (reach and act on every board cell), edge clamping (no blank space beyond the
  grid, FR-005), and the no-op case when the whole board already fits (FR-009) — verified live: a
  landmark cell opened by an earlier tap scrolled into view after repeated leftward drags
  (`12_after_bigdrag.png`), and further drags past the edge produced no further change, confirming
  the clamp (`13_edge_clamp.png`)

**Checkpoint**: All three user stories are independently functional; zoom (in both directions) and
pan together fully cover spec.md's requirements.

---

## Phase 6: Polish & Cross-Cutting Concerns

- [X] T021 [P] Run the full quickstart.md walkthrough end-to-end: all host tests (`ctest`), every
  simulator scenario (1–10), and the regression checks in quickstart.md §4 (existing flows —
  long-press flagging, chording, Exit/Return-to-Menu, hide-timer, per-digit colors — unaffected)
  — 55/55 host test cases pass (713 assertions); simulator scenarios 1–6 driven live via real
  Win32 mouse/wheel input (see T015/T017/T020 notes); scenario 9 (zoom/pan never affects game
  state) held throughout — "Mines: 99" never changed except from the one deliberate tap
- [X] T022 [P] Add a CHANGELOG.md entry documenting pinch-to-zoom / drag-to-pan, following the
  existing v1.1.0 entry's style and level of detail — added as v1.2.0
- [ ] T023 On real Kobo hardware, verify actual two-finger pinch and one-finger drag per
  quickstart.md §3 (the one thing the simulator's mouse-wheel/click-drag stand-in cannot fully
  substitute for); tune `kDragSlopPx`/`kGestureStepPx` in `src/core/gesture_recognizer.h` if the
  gesture feels unresponsive or too sensitive on the real touch panel — **not done**: requires
  physical Kobo hardware, unavailable in this environment. `evdev_touch.cpp`'s multi-touch
  handling also can't be compiled here (Kobo cross-toolchain only, per docs/building.md) — it was
  implemented and carefully reviewed against the same `core::GestureRecognizer` contract the host
  tests already exercise, but is unverified by any build/run in this session.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational only
- **User Story 2 (Phase 4)**: Depends on Foundational **and** User Story 1 (T016 directly extends
  T013's `onGesture()` branch; spec.md is explicit that US2 "depends on User Story 1 existing")
- **User Story 3 (Phase 5)**: Depends on Foundational **and** User Story 1 (spec.md: US3 "ships
  alongside" US1 since zooming in is what makes panning necessary) — independent of US2
- **Polish (Phase 6)**: Depends on all desired user stories being complete

Unlike a fully-independent-stories project, US2 and US3 both build directly on US1's zoomed-in state
and its `onGesture()` override — this mirrors spec.md's own stated priority rationale, not an
artifact of this task breakdown.

### Within Each Phase

- Tests (T004, T006) are written alongside their implementation counterparts (T003, T005) and must
  fail against a stub before the real logic is filled in (Constitution III)
- Foundational's interface changes (T007 → T008 → T009) are strictly ordered; T010/T011 (backend
  adapters) can proceed in parallel with each other once T005/T007 land
- T012 (BoardScreen viewport plumbing) only depends on T003, so it can proceed in parallel with
  T007–T011

### Parallel Opportunities

- T003/T004 and T005/T006 (two independent core classes + their tests) can all run in parallel
- T011 (SDL backend) can run in parallel with T010 (evdev backend) once T005/T007 land
- T012 can run in parallel with T007–T011 (different files, only needs T003)
- T021/T022 (Polish) can run in parallel with each other

---

## Parallel Example: Foundational Phase

```bash
# Launch the two core classes and their tests together:
Task: "Implement core::BoardViewport in src/core/board_viewport.h/.cpp"
Task: "Write tests/test_board_viewport.cpp"
Task: "Implement core::GestureRecognizer in src/core/gesture_recognizer.h/.cpp"
Task: "Write tests/test_gesture_recognizer.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL — blocks all stories)
3. Complete Phase 3: User Story 1 (pinch-to-zoom-in + correct-cell tapping + cascade auto-recenter)
4. **STOP and VALIDATE**: run quickstart.md scenarios 1–2 independently
5. This alone delivers the feature's core value (spec.md: "it delivers value on its own")

### Incremental Delivery

1. Setup + Foundational → shared gesture/viewport infrastructure ready, fully host-tested
2. Add User Story 1 → validate independently → MVP
3. Add User Story 2 → validate independently (zoom-out completes the zoom story)
4. Add User Story 3 → validate independently (pan makes zoomed-in boards fully reachable)
5. Polish → full regression pass, changelog, on-device verification

---

## Notes

- [P] tasks touch different files (or, for T012, a file whose only shared dependency — T003 — is
  already done) and have no completion-order dependency on each other
- [Story] label maps each task to spec.md's US1/US2/US3 for traceability
- `core::BoardViewport` and `core::GestureRecognizer` are the two pieces of correctness-critical
  logic in this feature (Constitution III) — every other change is either a thin interface
  rename/extension or ordinary UI-layer wiring verified via quickstart.md
- Real two-finger pinch behavior cannot be verified on the host or in the SDL simulator (one mouse
  pointer, no second finger) — T023 is the one task that requires actual Kobo hardware
