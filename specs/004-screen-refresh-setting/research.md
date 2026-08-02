# Phase 0 Research: Screen Refresh Frequency Setting

No items in Technical Context were left as `NEEDS CLARIFICATION` — this feature reuses the
existing codebase's language, dependencies, testing, and target platform unchanged. The research
below is grounded in direct inspection of `src/platform/renderer.h`, `src/platform/kobo/fbink_renderer.h/.cpp`,
`src/platform/sdl/sdl_renderer.h`, `src/core/settings.h/.cpp`, `src/ui/theme.h/.cpp`,
`src/ui/screens/settings_screen.h/.cpp`, and `src/main.cpp`.

## 1. Reuse `Renderer::setGhostingInterval` verbatim — it already exists, unused

**Decision**: Do not add any new refresh/ghosting mechanism. `Renderer::setGhostingInterval(int n)`
(`renderer.h:69`) and `FbinkRenderer`'s implementation (`fbink_renderer.cpp:126-128`,
`ghostingPartials_ = n > 0 ? n : INT_MAX;`, consumed by `flushPartial()` at
`fbink_renderer.cpp:112-119` to auto-promote the next partial refresh to a full flashing one after
`ghostingPartials_` partials) already implement exactly the behavior spec.md FR-004/FR-005
describe. This feature's entire job is to call it from Settings with a player-chosen value instead
of the hardcoded default (`fbink_renderer.h:30`, `ghostingPartials_ = 12;`).

**Rationale**: `renderer.h:66-69`'s own doc comment reads *"User-tunable ghosting policy (Settings
screen): n <= 0 disables auto-promotion. No-op on backends without a ghosting concept"* — this was
evidently designed in from `001` with exactly this Settings integration in mind, but the wiring was
never completed. Building a second, parallel mechanism instead would duplicate an already-correct,
already-tested-in-practice implementation (Constitution VI).

**Alternatives considered**: None seriously — the existing mechanism is a precise match for the
spec's four options (5/10/25/never map directly onto `n = 5/10/25/0`).

## 2. `Settings` gains a typed `ScreenRefreshInterval` enum, not a raw int

**Decision**: Add `enum class ScreenRefreshInterval { Every5, Every10, Every25, Never };` to
`src/core/settings.h`, and a field `ScreenRefreshInterval screenRefreshInterval =
ScreenRefreshInterval::Every10;` (FR-003's default) to `Settings`. Persist it in `settings.json` as
a string (`"Every5"`/`"Every10"`/`"Every25"`/`"Never"`), read via `j.value("screenRefreshInterval",
"Every10")` then parsed (so an absent key defaults exactly like a present `"Every10"` would),
matching `hideTimer`'s established backward-compatibility pattern
(`003-fix-timer-hide-option/research.md` #4) — `schemaVersion` stays `1`.

**Rationale**: `ColorMode` already establishes the "typed enum + string JSON encoding" idiom for a
small, fixed set of Settings choices in this codebase; a raw `int` field would let invalid values
(e.g. `7`, `-3`) round-trip silently instead of being caught by `fromJson`'s validation, and would
leak the "0 means never" `Renderer`-level convention into the persisted settings shape instead of
keeping it as an internal mapping detail. An enum keeps `settings.json` self-documenting and keeps
`fromJson` able to reject genuinely malformed values (an unrecognized string) exactly like
`colorMode`'s existing `colorModeFromString` does for its own invalid-value case.

**Alternatives considered**:
- *Store `screenRefreshInterval` as the raw `int` count (5/10/25) with `0` for "Never"*: rejected
  — loses the ability to distinguish a deliberately-chosen `0`/`Never` from a not-yet-migrated file
  or a typo'd value at the JSON boundary, and couples the persisted shape directly to the
  `Renderer`-internal sentinel convention instead of keeping that translation in one place
  (`applyScreenRefreshInterval`, research.md #3).
- *A `std::optional<int>`*: rejected — the spec's four options are fixed and closed, not an
  open-ended numeric range; an enum communicates that constraint directly, an `optional<int>` does
  not.

## 3. Wiring: a new `ui::applyScreenRefreshInterval`, called from the same two sites as `applyColorMode`

**Decision**: Add `void applyScreenRefreshInterval(Renderer& renderer, core::ScreenRefreshInterval
interval);` to `src/ui/theme.h/.cpp`, alongside the existing `applyColorMode`. Internally it maps
the enum to a count (`Every5→5`, `Every10→10`, `Every25→25`, `Never→0`) and calls
`renderer.setGhostingInterval(count)`. `AppImpl` (`main.cpp`) calls it once in the constructor,
right after loading `settings_` (mirroring the existing `applyColorMode(theme_, renderer_.info(),
settings_.colorMode);` call at `main.cpp:77`), and again inside `autosaveSettings()`, right after
its existing `applyColorMode(...)` call (`main.cpp:109`) — so a change made in Settings takes
effect on the very next partial-refresh decision, satisfying FR-006, with zero new call-site
plumbing beyond the two spots `applyColorMode` already established as "where Settings meets the
platform."

**Rationale**: `applyColorMode`'s existing two call sites (constructor for startup, `autosaveSettings()`
for immediate effect on change) are the exact shape this feature needs too — reusing the pattern
keeps `AppImpl` consistent rather than inventing a second "apply settings to platform" convention.
Putting the function in `theme.h/.cpp` (rather than a new file) keeps this feature at zero new
files, consistent with Constitution VI; the function's signature (`Renderer&`, not `Theme&`) is a
minor asymmetry with `applyColorMode` but the file already serves as "the module where Settings
values get pushed into platform-facing objects," which both functions are examples of.

**Alternatives considered**:
- *Have `SettingsScreen`'s `onTap` call `app_.renderer().setGhostingInterval(n)` directly, with the
  enum→int mapping inlined at each of the four button handlers*: rejected — duplicates the mapping
  four times in `settings_screen.cpp` instead of once, and skips applying the persisted value at
  startup (the constructor has no reason to touch `SettingsScreen` at all), which would leave the
  renderer at its hardcoded default (`12`) until the player opens Settings and taps something, even
  if they'd already chosen a different value in a previous session.
- *A new `App` interface method, e.g. `applyScreenRefreshInterval()`*: rejected — `applyColorMode`
  itself is not an `App` interface method either; it is a plain free function `AppImpl` happens to
  call, and there is no other caller (no screen needs to invoke this independently of a settings
  change) that would justify widening `App`'s interface (Constitution VI).

## 4. `Never` maps to `n = 0`; no new sentinel needed

**Decision**: `applyScreenRefreshInterval` passes `0` for `ScreenRefreshInterval::Never`, relying
on `Renderer::setGhostingInterval`'s already-documented contract (`renderer.h:66-67`, "n <= 0
disables auto-promotion") and `FbinkRenderer::setGhostingInterval`'s existing implementation
(`ghostingPartials_ = n > 0 ? n : INT_MAX;`, which then never trips `flushPartial`'s
`++partialCount_ >= ghostingPartials_` promotion check for any realistic session length).

**Rationale**: The `Renderer` interface already defines this exact semantic; inventing a second
"never" representation (e.g. a `bool` flag alongside the count) would duplicate a concept the
platform layer already expresses correctly, for no behavioral gain (Constitution VI).

**Alternatives considered**: None — the existing contract is a precise fit.

## 5. `SettingsScreen` UI: extend the existing two-way toggle idiom to four options

**Decision**: Add four `Button` fields (`refresh5Button_`, `refresh10Button_`, `refresh25Button_`,
`refreshNeverButton_`) laid out in one row below the existing Hide Timer row, each toggled
mutually-exclusively (only the button matching `app_.settings().screenRefreshInterval` shows
`toggled = true`), labeled "5", "10", "25", "Never" respectively, under a new "Screen Refresh"
section label. Tapping one sets `app_.settings().screenRefreshInterval` to the corresponding value,
calls `app_.autosaveSettings()`, redraws, and `flushFull()`s — identical to the existing
`colorButton_`/`blackWhiteButton_` handler shape.

**Rationale**: `colorButton_`/`blackWhiteButton_` already establish the "N mutually-exclusive
buttons, one `toggled` at a time, direct field assignment + autosave on tap" idiom for exactly this
category of setting (a small, fixed, single-choice-from-a-closed-set control) in this codebase;
reusing it for four options instead of two needs no new widget type, matching Constitution VI. A
`Label` for the "Screen Refresh" section heading needs its rect stored as a new field
(`screenRefreshLabelRect_`) — unlike the "Settings" title label (positioned by fixed constants
inline in `draw()`), this section label's vertical position depends on `layout()`'s running `y`
computation, the same reason `BoardScreen` stores `mineCountRect_`/`timerRect_` as fields rather
than recomputing their position inline in `draw()`.

**Alternatives considered**:
- *A single button that cycles through the four values on repeated taps*: rejected — hides the
  other three options from view at any given time, worse for discoverability/SC-001 (find and
  change the setting quickly) than four always-visible buttons.
- *A stepper (+/-) control, as `NewGameScreen`'s Custom section uses for width/height/mine count*:
  rejected — steppers in this codebase represent a numeric range a player dials in one step at a
  time; this setting is a closed set of four named options (one of which, "Never", isn't a number
  at all), which the mutually-exclusive-button idiom represents more directly.
