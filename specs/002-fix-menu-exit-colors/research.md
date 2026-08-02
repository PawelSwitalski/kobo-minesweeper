# Phase 0 Research: Menu Layout, Exit Controls, and Mine-Count Colors Fixes

No items in Technical Context were left as `NEEDS CLARIFICATION` — this feature reuses the
existing `001` codebase's language, dependencies, testing, and target platform unchanged. The
research below resolves the concrete design decisions the spec's clarified requirements force,
each grounded in what the current code already does (per direct inspection of
`src/ui/screens/new_game_screen.cpp`, `src/ui/screens/board_screen.cpp`, `src/ui/app.h`,
`src/main.cpp`, `src/platform/renderer.h`, and `src/platform/softcanvas.cpp`).

## 1. Exit controls call the existing, currently-unused `App::requestExit()`

**Decision**: `NewGameScreen` and `BoardScreen` each get a new `Button` labeled "Exit" whose
`onTap` handler calls `app_.requestExit()` directly — no dialog, no new App method needed for
FR-001/FR-002/FR-014.

**Rationale**: `App::requestExit()` and its `AppImpl` implementation (`main.cpp:119`,
`exitRequested_ = true`) already exist and already correctly break the main loop
(`while (!app.exitRequested() && ...)`, `main.cpp:214`) and unconditionally persist on the way out
(`app.autosaveSession(); app.autosaveSettings();`, `main.cpp:274-275`). Today its only caller is
the idle-timeout watchdog (`main.cpp:258-261`) — no screen has ever called it. Wiring a plain
button tap to this exact call gives FR-003 (progress preserved) and FR-014 (no confirmation) for
free, with zero new persistence code.

**Alternatives considered**: Adding a new `App::exitImmediately()` alias — rejected, `requestExit()`
already does precisely this; a second method with the same behavior would be a needless
abstraction (Constitution VI).

## 2. `App::returnToMainMenu()` — a new method, not a bare `pop()`

**Decision**: Add `virtual void returnToMainMenu() = 0;` to `App`. `AppImpl` implements it as:
reset `session_` to a fresh default-constructed `core::GameSession()` (which is `NotStarted`,
Beginner config — the same state `main.cpp` already treats as "show the menu" at startup),
`autosaveSession()` to persist that reset immediately, then clear the entire screen stack and
push a fresh `NewGameScreen`. `BoardScreen`'s new "Return to Menu" banner button calls this.

**Rationale**: `main.cpp`'s startup logic pushes `BoardScreen` directly (skipping `NewGameScreen`
entirely) whenever `hasInProgressGame() || status == Won || status == Lost`
(`main.cpp:182-188`) — so after a fresh launch that resumes a finished game, `BoardScreen` is the
*only* entry on the screen stack, with no `NewGameScreen` underneath it to `pop()` back to. A
plain `app_.pop()` in that case would empty the stack entirely, which the main loop's
`while (... && app.top())` condition (`main.cpp:214`) treats as "exit" — silently conflating
"return to menu" with "quit," which violates FR-004's requirement that they be distinct controls
with distinct outcomes. A dedicated `returnToMainMenu()` sidesteps stack depth entirely: it always
lands on a fresh `NewGameScreen`, whether `BoardScreen` was pushed as the sole root screen (resumed
at launch) or on top of an existing `NewGameScreen` (started this session) — and clearing the
stack (rather than pushing on top) avoids the alternative's unbounded stack growth across repeated
play/win/return cycles in one sitting.

Resetting to a fresh `GameSession()` (rather than merely leaving the finished session in place and
relying on some other flag) directly satisfies FR-006: `main.cpp`'s startup resume condition checks
`status`, so a `NotStarted` session makes the next launch show `NewGameScreen`, not the old finished
board — with no new persisted field required.

**Alternatives considered**:
- *Plain `app_.pop()` from the banner button*: rejected — breaks when `BoardScreen` is the stack
  root, as shown above.
- *Push a new `NewGameScreen` on top without clearing the stack*: rejected — leaves the previous
  (finished) `BoardScreen` (and any earlier screens) buried in the stack forever, growing without
  bound across repeated games in one session, for no benefit (nothing needs to navigate back
  *into* a finished game).
- *A boolean "needsReset" flag consulted only at startup*: rejected — more state to keep in sync
  than just resetting `session_` immediately, for the same eventual effect.

## 3. Outcome banner buttons: extend the existing banner, no new widget

**Decision**: `BoardScreen::drawOutcomeBanner()` grows to include two `Button`s ("Return to Menu",
"Exit") below the existing title/time text, sized within the same bordered box (the box height
grows to fit them). `BoardScreen::onTap()` — which today unconditionally swallows all taps once
`status()` is `Won`/`Lost` (`board_screen.cpp:211-213`, "board actions are inert once the game has
ended") — gains a check for these two buttons *before* that early return, since the two new
buttons are exactly the affordances FR-004/FR-005 require on that screen; the existing
"board is inert" behavior is otherwise unchanged (the grid itself still ignores taps).

**Rationale**: Reuses the existing `Button` widget and the banner's existing draw call exactly as
`Dialog` already reuses `Button` for its own buttons (`widgets.cpp:102-103`) — no new widget type.
Keeping the buttons inside the existing bordered banner (rather than, say, a separate modal
`Dialog`) avoids a second competing "outcome UI" concept on the same screen.

**Alternatives considered**: Reusing `ui::Dialog::confirm(...)` for the outcome instead of the
existing custom banner — rejected; `Dialog` is a generic two-button confirm/cancel shape, while the
outcome banner already has its own title/elapsed-time layout this feature only needs to extend, not
replace (Constitution VI: prefer the smaller change).

## 4. New Game menu layout: group, then double the gap, then Custom

**Decision**: `NewGameScreen::layout()` adds the new Exit button immediately after Settings
(same group, same per-item spacing `t.gap`), then advances `y` by `2 * t.gap` (instead of the
current `t.gap * 2`... which is already exactly this value in the pre-existing code at
`new_game_screen.cpp:37` — see note below) before laying out the Custom header/steppers. The
Custom section's internal layout (header, three +/- steppers, reason line, Start Custom Game
button) is otherwise unchanged.

**Note on the existing code**: `new_game_screen.cpp:37` already writes
`y += t.touchTargetPx + t.gap * 2;` after the Settings button — i.e., the gap *before* Custom is
already `2 * t.gap` today, exactly twice the `t.gap` used between the preset buttons themselves.
The user's complaint is therefore about the *visual grouping*, not really the raw gap arithmetic:
today nothing distinguishes "the gap before Custom" from "the gap between any two preset buttons"
except that one extra `t.gap` — visually subtle on an e-ink screen. This feature keeps the
resolved 2× multiplier (per clarification) but makes the grouping legible: the Exit button joins
the preset group (so the group reads as five buttons, not three), and the "Custom" section header
text remains the clear label marking where the new block starts, directly below the now-more-
noticeable gap.

**Rationale**: Reuses the exact stepper/section code already in place (`new_game_screen.cpp:39-67`)
untouched; only the button list above it and the gap constant change, per FR-007/FR-008/FR-009.

**Alternatives considered**: A visual divider line between the groups — rejected; not requested by
the spec (which asks only for spacing, per the clarified "2x normal spacing" answer), and would be
an extra visual element beyond what was asked (Constitution VI).

## 5. Mine-count digit colors: extend `Color`, resolve in the one shared `SoftCanvas`

**Decision**: Extend `enum class Color : uint8_t` in `src/platform/renderer.h` from
`{ None, Red }` to `{ None, Red, Blue, Green, Navy, Crimson, Cyan }`. `board_screen.cpp`'s
`drawCell()` replaces its single `if (app_.theme().color) accent = Color::Red;` with a per-count
lookup:

| Count | Accent (Color mode) | Notes |
|---|---|---|
| 1 | `Color::Blue` | new |
| 2 | `Color::Green` | new |
| 3 | `Color::Red` | reuses the existing value/RGB — visually unchanged from today for this one digit |
| 4 | `Color::Navy` | new; "deep blue" |
| 5 | `Color::Crimson` | new; "cherry red" |
| 6 | `Color::Cyan` | new |
| 7 | `Color::None` (glyph shade `Gray::Black`, unchanged default) | "black" is already the default digit shade — no accent needed |
| 8 | `Color::None`, glyph shade set to `Gray::Mid` | "gray" is expressed via the existing grayscale shade axis, not a new hue |

`SoftCanvas::resolveColor()` (`softcanvas.cpp:22-28`) gains one RGB triple per new `Color` value
(only used when `colorDisplay` is true, exactly like the existing `Color::Red` branch); the
`shade` fallback path (used whenever `colorDisplay` is false, i.e. Black & White mode or a
monochrome device) is completely untouched, so FR-011 holds automatically.

**Rationale**: `resolveColor()` and `SoftCanvas` are the one rasterizer both the SDL and FBInk
backends draw through (`platform/canvas_renderer.h`, `softcanvas.cpp`) — extending it here means
both backends get pixel-identical colors with a single edit, matching the existing
`Color::Red` precedent exactly (Constitution I). Digits 7 and 8 deliberately reuse the existing
`Gray` shade mechanism instead of adding two more `Color` values: black and (mid-)gray are already
expressible as points on the existing grayscale axis used everywhere else in this app (mine glyphs
are already drawn `Gray::White` on a `Gray::Black` background, for instance), so representing them
as `Color::None` + a shade keeps the accent enum reserved for genuine hues only, exactly mirroring
how `Color::Red` was already documented as "accent only, never sole meaning" — grayscale digits 7/8
need no such accent because grayscale *is* their assigned color.

**Alternatives considered**:
- *Add all 8 digits as new `Color` enum values (including Black/Gray)*: rejected — would duplicate
  the existing `Gray` enum's job for two of the eight values, adding an enum value that means the
  same thing a `Gray` shade already means (Constitution VI).
- *A `std::array<Color, 8>` lookup table constant instead of an inline switch in `drawCell`*:
  considered equivalent in effect; either is acceptable at task-generation time, this doc does not
  mandate one over the other since it's a purely local implementation-shape choice with no
  architectural consequence.

**RGB choices** (for `resolveColor`, colorDisplay only — chosen to stay visually distinct from
each other and from the existing `Red` value `(0xB4, 0x20, 0x20)` and `Gray::Black`/`Gray::Mid`):

| Color | RGB (hex) | Rationale |
|---|---|---|
| `Blue` | `#1030C0` | Clear, saturated blue distinct from `Navy`. |
| `Green` | `#127A12` | Mid-dark green, legible on white without looking like `Cyan`. |
| `Navy` | `#0A1868` | Visibly darker/more desaturated than `Blue` — "deep blue." |
| `Crimson` | `#8B0A2A` | Darker, more maroon than the existing plain `Red` — "cherry red," stays distinguishable from `Red` and `Navy` side by side. |
| `Cyan` | `#0A7A82` | Teal-leaning cyan, legible on white (pure `#00FFFF` washes out against a white cell background). |

These are a starting point for implementation, not a pixel-perfect mandate; task execution may
adjust exact hex values for on-device legibility as long as all eight digits stay mutually
distinguishable per SC-006 and the digit glyph itself (not color) remains the primary signal per
FR-012.
