# Phase 0 Research: Core Minesweeper Gameplay & Display Settings

No items in Technical Context were left as `NEEDS CLARIFICATION` — the existing codebase already
fixes language, dependencies, testing, and target platform. The research below instead resolves
the concrete design decisions the spec's touch-driven, e-ink-constrained requirements force,
each grounded in patterns already present in this template.

## 1. Long-press detection for flagging (FR-009)

**Decision**: Extend `platform::Tap` with a `bool longPress` field, and add a shared
`constexpr int kLongPressMs = 500;` threshold in `platform/input.h` (portable, no OS calls — just
a constant both backends reference). Each `TouchInput` backend times its own touch-down→up
interval and sets `longPress = (duration_ms >= kLongPressMs)` before returning the collapsed
`Tap`, exactly as it already collapses down/up into one `Tap` today.

**Rationale**: `TouchInput::waitForTap` currently collapses an entire down→up cycle into a single
`Tap{x,y}` with no timing information — there is no way for the UI layer to distinguish a tap
from a long-press today. Timing must be sensed where the down/up events actually arrive
(`EvdevTouch` already parses `BTN_TOUCH`/`ABS_MT_TRACKING_ID` down/up; `MouseTouch` already
parses `SDL_MOUSEBUTTONDOWN`/`UP`), so this stays a `platform/`-only change — Principle I is
preserved because the *policy* (is 500ms "long"?) is a plain constant, not an OS call, and the UI
layer still only ever sees a `Tap` value, never raw events.

**Alternatives considered**:
- *UI-layer timing via two taps (down-tap, up-tap) instead of one collapsed `Tap`*: rejected —
  would force every existing/future screen to implement its own down/up state machine, a much
  bigger interface change than adding one bool to `Tap`.
- *Separate `waitForLongPress()` method on `TouchInput`*: rejected — can't know in advance which
  gesture is coming; the backend has to time the whole down/up cycle regardless, so reporting it
  on the same `Tap` is simpler and cheaper than a second blocking call.

## 2. Flag Mode control (FR-010) and its interaction with long-press

**Decision**: `BoardScreen` owns a plain ephemeral `bool flagModeOn_ = false;` member (not
persisted, per the spec's Assumptions) with a small `Button` in the HUD to toggle it. Long-press
always flags/unflags an unopened cell regardless of `flagModeOn_` (per clarification). A plain
tap on an unopened cell flags/unflags when `flagModeOn_` is true, opens when false. A tap on an
already-opened numbered cell always attempts a chord, regardless of `flagModeOn_`.

**Rationale**: Matches the resolved clarifications directly; keeping the flag as a screen-local
UI variable (not in `core::GameSession`) keeps `core` free of input-mode concepts it has no
business knowing about (Principle I/VI).

**Alternatives considered**: Persisting Flag Mode state — rejected per the spec's own Assumptions
section (explicitly out of scope for persistence).

## 3. Persisted file layout: two files, not one

**Decision**: `game.json` (in-progress/ended `GameSession`) and `settings.json` (`Settings`),
each an independent atomic file via the existing `persist::store`, following the same one-file-
per-concern shape as the template's `counter.json`.

**Rationale**: FR-023 requires that a corrupt in-progress-game save must not affect the persisted
color mode. Two independent files make that isolation automatic (a JSON parse failure in one
file can never touch the other) rather than something application code has to carefully
preserve inside a combined blob.

**Alternatives considered**: One combined `state.json` — rejected; would require partial-corruption
handling (recover settings out of an otherwise-invalid combined document) that two files avoid
for free.

## 4. Mine placement algorithm and first-click safety (FR-005)

**Decision**: Mines are placed lazily on the first `openCell()` call: build the list of all cell
indices except the first-opened one, draw `mineCount` of them without replacement (partial
Fisher–Yates / `std::sample` over `std::mt19937` seeded from `std::random_device`), mark them as
mines, then compute each non-mine cell's `adjacentMines` count once. Before the first open,
`Board` holds only `width`/`height`/`mineCount` — no mine layout exists yet.

**Rationale**: Directly implements FR-005 ("mines placed only after the first cell-open action...
the specific cell first opened is never a mine") and SC-002 (0% of games end on the first cell).
Only the exact opened cell is excluded from the draw — per the spec's own text, guaranteeing the
whole first-click neighborhood is mine-free was explicitly called a stretch goal and left out of
scope, so the simpler single-cell exclusion is the correct, spec-matching implementation.

**Alternatives considered**: Placing all mines at board-construction time and re-rolling if the
first click hits one — rejected as more complex (needs a retry loop, and for high mine-density
custom boards could theoretically retry many times) for no behavioral difference from the
lazy-placement approach.

## 5. Cascade flood-fill algorithm (FR-007)

**Decision**: Iterative BFS/DFS using an explicit `std::vector<int>` work-stack of cell indices
(not recursive function calls).

**Rationale**: Up to 480 cells (30×16 Expert) is trivial either way, but an explicit stack avoids
any recursion-depth consideration entirely and is no more code than a recursive version —
straightforward, defensively simple (Constitution VI).

**Alternatives considered**: Recursive flood fill — rejected only for the (very cheap) safety
margin of not depending on call-stack depth on constrained hardware; not a performance-driven
choice given the small board sizes involved.

## 6. Elapsed-time pause semantics (FR-016, clarified: pauses when backgrounded)

**Decision**: No new mechanism needed — reuse `Screen::onTick(uint32_t activeSeconds)` and
`Screen::countsPlayTime()`, which the template already defines for exactly this purpose (see
`src/ui/screens/screen.h`'s existing comment: "Only time spent on screens that return true counts
toward any active time tracking... device sleep and menu time excluded"). `BoardScreen::
countsPlayTime()` returns `true` only while `session().status() == InProgress`; `NewGameScreen`
and `SettingsScreen` return `false` (the default). The app-shell loop in `main.cpp` already
computes `activeSeconds` since the last tick using a steady clock and only calls `onTick` on the
current top screen — so time spent on another screen, or with the process not running at all
(closed app), is never counted, satisfying the clarification with zero new plumbing.
`GameSession::addActiveSeconds(uint32_t)` simply accumulates into the persisted `elapsedSeconds`
field; the on-screen timer label redraws (partial flush) only when the displayed minute value
changes, satisfying FR-016's "no more than once per minute" cap.

**Rationale**: The architecture already anticipated this exact requirement; implementing it any
other way would duplicate logic that already exists and is already exercised by the idle-exit
timer in `main.cpp`.

**Alternatives considered**: A wall-clock timestamp pair (`startedAt`/`now`) recomputed on every
draw — rejected; would count backgrounded/closed time unless extra pause bookkeeping were added,
which is exactly what `onTick`/`countsPlayTime` already provide for free.

## 7. Color-mode composition: player preference × device hardware (FR-021)

**Decision**: `Theme::color` (already existing, currently set from `DisplayInfo::color` alone in
`makeTheme`) becomes the AND of two things: the device's actual color capability
(`DisplayInfo::color`, a hardware fact) and the player's `Settings::colorMode` preference. Add a
small `void applyColorMode(Theme&, const DisplayInfo&, ColorMode)` helper in `ui/theme.*` that
`AppImpl` calls once at startup (after loading `settings.json`) and again immediately whenever
the player changes the setting in `SettingsScreen`.

**Rationale**: Matches the clarified/assumed behavior directly: the color-mode control is always
shown and always toggleable (FR-021) regardless of hardware, but a monochrome device obviously
still renders in grayscale even if the player selects "Color" — hardware capability is a floor,
the setting is a ceiling.

**Alternatives considered**: Hiding/disabling the setting on monochrome hardware — explicitly
rejected in the spec's clarification session (chosen answer was "always shown").

## 8. Grid cell sizing vs. the 9mm touch-target floor

**Decision**: The game grid's per-cell hit area is *not* held to `Theme::touchTargetPx` (the 9mm
floor used for buttons elsewhere). Cell size is derived by fitting `width × height` cells into
the available board area on the display, same as every existing Minesweeper implementation on
any device. Real UI controls (New Game buttons, Settings buttons, the Flag Mode toggle, HUD)
continue to use `Theme::touchTargetPx` as normal.

**Rationale**: Minesweeper's whole design is many small cells; an Expert board (30 wide) on the
reference 1264px-wide display already yields ~42px (~3.5mm) cells even at full width, well under
9mm — the same is true of Expert boards on physical Windows/mobile Minesweeper. This is inherent
to the classic preset the spec explicitly asks for, not a defect introduced here. Constitution
II's "touch targets scale from DPI" requirement is about not hardcoding pixel sizes (which this
design also satisfies — cell size is computed from `DisplayInfo`/board dimensions, never a
literal constant), not about a fixed minimum for a dense information grid.

**Alternatives considered**: Panning/zooming a fixed-cell-size board — explicitly out of scope
(no such requirement in the spec, and it would add real complexity for no requested value);
capping board pixel dimensions and requiring scroll — rejected for the same reason.

## 9. Custom board size/mine-count entry UI (US5 / FR-002)

**Decision**: `NewGameScreen`'s custom section uses `+`/`-` stepper `Button`s per field (width,
height, mine count), each clamped to the FR-002 bounds (5–16 / 5–16 / 1..(w×h−9), the upper mine
bound re-clamped live as width/height change), with the current values shown via `Label`.

**Rationale**: The template's widget vocabulary (`Button`/`Label`/`Dialog`) has no text-entry or
on-screen-keyboard widget, and building one would be a large, unnecessary addition (Constitution
VI) for three small bounded integers. Steppers reuse `Button` exactly as-is.

**Alternatives considered**: An on-screen numeric keypad — rejected as unnecessary complexity for
three small-range integers; a single "cycle through a fixed list of sizes" button — rejected as
less flexible than the spec's continuous 5–16 range implies.

## 10. Abandon-in-progress-game confirmation (FR-024)

**Decision**: Reuse `ui::Dialog::confirm(...)` as-is (already exists exactly for this shape:
title, message, cancel label, confirm label) when the player picks a new difficulty/custom
config from `NewGameScreen` while `app_.session().status() == InProgress`. Confirming calls
`App::startNewGame(config)`; canceling closes the dialog and leaves the in-progress session
untouched.

**Rationale**: `Dialog::confirm` is an exact match for what FR-024 needs; no new widget required.

## 11. Removing the placeholder Counter/About demo

**Decision**: Delete `src/core/counter.*`, `src/ui/screens/counter_screen.*`,
`src/ui/screens/about_screen.*`, and `tests/test_counter.cpp`; remove their references from
`CMakeLists.txt`.

**Rationale**: `SETUP.md` explicitly instructs deleting the Counter demo "once you have real
state to replace it with," and `AboutScreen` existed only to exercise `push`/`pop` navigation —
`NewGameScreen`/`BoardScreen`/`SettingsScreen` now exercise that same navigation with real
screens, so keeping the placeholder around would just be dead demo code (Constitution VI).
