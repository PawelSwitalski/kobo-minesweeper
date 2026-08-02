# Implementation Plan: Screen Refresh Frequency Setting

**Branch**: `004-screen-refresh-setting` | **Date**: 2026-08-02 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/004-screen-refresh-setting/spec.md`

## Summary

Exposes an existing-but-never-wired-up mechanism as a player-facing setting: `Renderer::setGhostingInterval(int n)` (`src/platform/renderer.h:69`) and its `FbinkRenderer` implementation
(`src/platform/kobo/fbink_renderer.cpp`) already implement "auto-promote to a full flashing
refresh after N partial refreshes" — the header comment literally says *"User-tunable ghosting
policy (Settings screen)"* — but nothing has ever called it from Settings. This feature adds
`core::Settings::screenRefreshInterval` (a new `ScreenRefreshInterval` enum: `Every5` / `Every10`
(default) / `Every25` / `Never`), a four-button mutually-exclusive control on `SettingsScreen`
(extending the existing `colorButton_`/`blackWhiteButton_` two-way-choice idiom to four options),
and a new `ui::applyScreenRefreshInterval(Renderer&, ScreenRefreshInterval)` free function —
mirroring `applyColorMode`'s existing call-site pattern exactly — called once at startup (`AppImpl`
constructor) and again on every settings change (`autosaveSettings()`) so the choice takes effect
immediately. `Never` maps to `n = 0`, matching the renderer's existing `n <= 0 disables
auto-promotion` contract with no new sentinel needed. No new files; every change is an edit to a
file that already exists.

## Technical Context

**Language/Version**: C++17 (existing codebase; per constitution)

**Primary Dependencies**: None added. Reuses the existing `Renderer::setGhostingInterval`
interface method (already implemented by `FbinkRenderer`; no-op default for `SdlRenderer`/the
desktop simulator, per `renderer.h:69`'s own doc comment), the existing `Button` widget, and the
existing `Settings`/`applyColorMode` persistence-and-apply pattern.

**Storage**: `settings.json` gains one additive, backward-compatible field, `screenRefreshInterval`
(string enum: `"Every5"`/`"Every10"`/`"Every25"`/`"Never"`, default `"Every10"` when absent).
`schemaVersion` stays `1`, following the exact precedent `hideTimer` set in
`003-fix-timer-hide-option` (research.md #4 of that feature).

**Testing**: doctest, host-run via `-DBUILD_TESTS=ON` + `ctest` (Constitution III). `Settings`'
JSON round-trip and backward-compatible-default behavior for `screenRefreshInterval` are extended
into `tests/test_settings.cpp`, exactly like `colorMode`/`hideTimer` already are. The
`ScreenRefreshInterval → int` mapping used by `ui::applyScreenRefreshInterval` is a trivial, static
4-way lookup with no branching logic worth a doctest beyond what `quickstart.md`'s manual
on-device check already verifies — same precedent `002-fix-menu-exit-colors` established for its
digit→color mapping (that feature's Constitution Check, row III), since `src/ui/` is not linked
into `minesweeper_tests` today (`CMakeLists.txt`).

**Target Platform**: Kobo e-ink devices (FBInk backend, where the effect is observable) and the
SDL2 desktop simulator (where the setting is still selectable/persisted per FR-007, but has no
visible effect, since `SdlRenderer` never overrides `setGhostingInterval` and inherits the
`Renderer` base class's no-op default).

**Project Type**: Single project, existing layered structure. Zero new files; every task edits a
file that already exists from `001`/`002`/`003`.

**Performance Goals**: No change to e-ink refresh discipline beyond what this feature exists to
tune (Constitution II) — it exposes the *existing* ghosting-promotion knob, it doesn't add a new
redraw trigger. Selecting a lower "Every N" trades more frequent full-screen flashes (slower,
cleaner) for less ghosting; "Never" trades zero ghosting-driven flashes for potential visible
ghosting during long partial-refresh-only stretches. This tradeoff is exactly what the setting
exists to let the player choose.

**Constraints**: `SettingsScreen`'s layout grows by one more row (a "Screen Refresh" section label
plus four buttons) below the existing Hide Timer row and above Back — the screen must still fit
within the display height at the smallest supported touch-target size; no scrolling is introduced
(consistent with `SettingsScreen`'s existing fixed, non-scrolling layout).

**Scale/Scope**: Same single-local-player scope as prior features; no new persisted entities beyond
one new `Settings` field, no new concurrency, no new device API surface (reuses
`Renderer::setGhostingInterval`, which already exists).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design (see below).*

| Principle | Status | Notes |
|---|---|---|
| I. Portable Core, Thin Platform Layer | **PASS** | `Settings::screenRefreshInterval` is a plain enum field in `src/core/settings.h` — no OS calls. `ui::applyScreenRefreshInterval` lives in `src/ui/theme.cpp` alongside `applyColorMode`, the established "derive platform config from Settings" module; it calls the *existing* `Renderer::setGhostingInterval` interface method, already implemented identically by both backends (FBInk actually promotes; SDL no-ops) — no new divergent per-backend code. |
| II. E-ink-First, Grayscale-First UX | **PASS** | This feature *is* the ghosting-policy tuning knob the constitution's own rationale describes ("periodically to clear ghosting") — it doesn't add a new per-tick redraw source, it lets the player tune an existing one. The four-button control is static UI, redrawn only on Settings screen transitions/taps, same as every other Settings control. |
| III. Host-Testable Correctness (NON-NEGOTIABLE) | **PASS** | `Settings::screenRefreshInterval`'s JSON round-trip, default, and backward-compatible-missing-key behavior are host-tested in `tests/test_settings.cpp`, identically to `colorMode`/`hideTimer`. The `ScreenRefreshInterval → int` mapping (`ui::applyScreenRefreshInterval`) is UI/platform-wiring code with no state-transition or generative logic — a static 4-way switch — verified via `quickstart.md`'s on-device scenario, matching the accepted precedent `002`'s digit→color mapping already established for exactly this category of trivial, non-`core::` lookup. |
| IV. Firmware-Agnostic Device Integration | **PASS** | No new device API surface at all — `Renderer::setGhostingInterval`/`FbinkRenderer`'s `fbink_refresh`-based promotion logic already exists and is unchanged by this feature; this feature only adds the caller. |
| V. Never Lose the User's Progress | **PASS** | `screenRefreshInterval` persists via the existing `autosaveSettings()` → `saveFileAtomic(paths_.settings, ...)` path, additive and backward-compatible exactly like `hideTimer` (research.md #1). |
| VI. Simplicity and Minimal Dependencies | **PASS** | No new dependencies, no new widget type (extends the existing two-way `Button` toggle-group idiom to four options), no new persistence file, no new Renderer method — every piece this feature needs (the interface method, the backend implementation, the settings-apply pattern, the button widget) already exists in the codebase and is simply wired together. |

No violations — Complexity Tracking table is empty (see below).

**Post-Phase-1 re-check**: research.md and data-model.md were reviewed against the same six rows
after design. The decision to reuse `Renderer::setGhostingInterval` verbatim rather than adding any
new mechanism (research.md #1) and the enum-with-string-JSON encoding choice (research.md #2) are
the two decisions with any real design weight; neither introduces a platform dependency, a new
redraw trigger, or a schema version bump. Gate remains **PASS**, unchanged from the pre-research
check above.

## Project Structure

### Documentation (this feature)

```text
specs/004-screen-refresh-setting/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md         # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/            # Phase 1 output (/speckit-plan command)
│   ├── persistence-schema.md
│   └── screen-refresh-application.md
└── tasks.md              # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
src/
├── core/
│   ├── settings.h/.cpp              # Settings gains `ScreenRefreshInterval screenRefreshInterval
│   │                                 # = ScreenRefreshInterval::Every10;` + JSON round-trip (edit)
│
├── ui/
│   ├── theme.h/.cpp                  # new applyScreenRefreshInterval(Renderer&,
│   │                                 # core::ScreenRefreshInterval) free function, alongside the
│   │                                 # existing applyColorMode (edit)
│   ├── screens/
│   │   ├── settings_screen.h/.cpp    # 4 new mutually-exclusive Button fields (5/10/25/Never) +
│   │                                 # a section-label Rect; layout/draw/onTap extended (edit)
│
├── main.cpp                          # AppImpl constructor and autosaveSettings() each call the
│                                     # new applyScreenRefreshInterval(...), mirroring the existing
│                                     # applyColorMode(...) call sites (edit)

tests/
├── test_settings.cpp                 # extended: screenRefreshInterval default, round-trip for
│                                     # all 4 values, old-file-without-the-key compatibility (edit)

CMakeLists.txt                        # UNCHANGED — no files added or removed
```

**Structure Decision**: Single project, existing four-layer template structure retained unchanged.
This feature adds **zero new files** — every change is an edit to a file that already exists.

## Complexity Tracking

*No Constitution Check violations — table intentionally empty.*
