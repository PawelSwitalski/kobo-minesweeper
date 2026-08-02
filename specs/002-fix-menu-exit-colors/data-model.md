# Phase 1 Data Model: Menu Layout, Exit Controls, and Mine-Count Colors Fixes

This feature introduces **no new persisted entities and no changes to any existing JSON schema**.
`game.json` and `settings.json` (`data-model.md` of `001-core-gameplay-settings`) are unchanged —
`schemaVersion` stays `1` for both. What follows are the non-persisted contract/enum extensions
this feature does add.

## `Color` (enum, `src/platform/renderer.h`)

Extended from `{ None, Red }` to:

| Value | Meaning | Introduced by |
|---|---|---|
| `None` | No accent; render in the plain `Gray` shade already given (unchanged). | `001` |
| `Red` | Reused as-is for mine-count `3` (research.md #5). | `001` |
| `Blue` | Mine-count `1`. | `002` (this feature) |
| `Green` | Mine-count `2`. | `002` |
| `Navy` | Mine-count `4` ("deep blue"). | `002` |
| `Crimson` | Mine-count `5` ("cherry red"). | `002` |
| `Cyan` | Mine-count `6`. | `002` |

Mine-counts `7` and `8` do **not** get new `Color` values — they stay `Color::None` and instead
set `TextStyle::shade` directly (`Gray::Black` for `7`, unchanged from today's default; `Gray::Mid`
for `8`, new). See research.md #5 for the full digit→rendering table and rationale.

**Validation rule**: this mapping applies only when `app_.theme().color` is true (Color mode *and*
a color-capable device — `ui::applyColorMode`, unchanged from `001`). When false, every digit
renders with `accent = Color::None` and `shade = Gray::Black` exactly as `001` already does — no
per-digit shade change in Black & White mode (FR-011).

## `App` interface (`src/ui/app.h`)

One new pure-virtual method, added alongside the existing navigation methods:

```cpp
virtual void returnToMainMenu() = 0;
```

**Contract** (see `contracts/app-interface.md` for the full picture including unchanged methods):
- Resets the current `core::GameSession` to a fresh default (`NotStarted`, Beginner config).
- Persists that reset via the existing `autosaveSession()` path (same `game.json` shape, just now
  written with `status: "NotStarted"` and an empty `cells` array, exactly like any other
  `NotStarted` session already serializes per `001`'s `GameSession::toJson()`).
- Replaces the entire screen stack with a single fresh `NewGameScreen` (does not merely `pop()`;
  see research.md #2 for why stack depth can't be assumed).
- Callers: `BoardScreen`'s new "Return to Menu" outcome-banner button only. No other screen calls
  this (menu-to-menu navigation doesn't need a reset; `NewGameScreen` is already the menu).

## `BoardScreen` (class, `src/ui/screens/board_screen.h/.cpp`)

New non-persisted members (screen-local UI state, same category as the existing `flagModeOn_`):

| Field | Type | Purpose |
|---|---|---|
| `exitButton_` | `Button` | HUD row, alongside existing `flagModeButton_`/`settingsButton_`. |
| `returnToMenuButton_` | `Button` | Laid out only while the outcome banner is shown; calls `app_.returnToMainMenu()`. |
| `outcomeExitButton_` | `Button` | Laid out only while the outcome banner is shown; calls `app_.requestExit()`. |

No new `core::` types; `Board`/`GameSession`/`DifficultyConfig`/`Settings` are all unchanged.

## `NewGameScreen` (class, `src/ui/screens/new_game_screen.h/.cpp`)

New non-persisted member:

| Field | Type | Purpose |
|---|---|---|
| `exitButton_` | `Button` | Grouped with `beginnerButton_`/`intermediateButton_`/`expertButton_`/`settingsButton_`, above the gap preceding the Custom section. |

## Relationships (delta over `001`)

```text
App
├─ session(): GameSession        — unchanged type; returnToMainMenu() replaces its value with a
│                                   fresh default instead of a chosen-difficulty one (contrast with
│                                   startNewGame(cfg), which always takes an explicit cfg)
├─ push/pop(): Screen stack      — unchanged; returnToMainMenu() is a new third navigation
│                                   primitive alongside them (clear + push, not pop)
└─ requestExit()                 — unchanged; now actually called, from three new Button taps
                                    instead of only the idle-timeout watchdog
```
