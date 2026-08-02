# Phase 1 Data Model: Core Minesweeper Gameplay & Display Settings

All types below live in `src/core/` (namespace `minesweeper::core`), have no platform/OS
dependencies, and are unit-testable on the host (Constitution I/III). JSON shapes are the
persisted form written by `game.json`/`settings.json` (Constitution V); each carries a
`schemaVersion` field, following the existing `Counter` precedent
(`src/core/counter.cpp`).

## CellState (enum)

`Unopened | Opened | Flagged` — FR-004: every `Cell` is in exactly one of these at all times.

## Cell (struct)

| Field | Type | Notes |
|---|---|---|
| `isMine` | `bool` | Set once, at mine-placement time (FR-005). Never changes after. |
| `state` | `CellState` | `Unopened` initially. |
| `adjacentMines` | `int` (0–8) | Valid/meaningful only when `isMine == false`; computed once at mine-placement time from the final mine layout (FR-006). Not persisted — recomputed from `isMine` on load (see GameSession JSON below) so a corrupted count can never disagree with the mine layout it's supposed to describe. |

## DifficultyPreset (enum)

`Beginner | Intermediate | Expert | Custom` — FR-001.

## DifficultyConfig (struct)

| Field | Type | Notes |
|---|---|---|
| `preset` | `DifficultyPreset` | |
| `width` | `int` | Beginner 9, Intermediate 16, Expert 30; Custom 5–16 (FR-002). |
| `height` | `int` | Beginner 9, Intermediate 16, Expert 16; Custom 5–16 (FR-002). |
| `mineCount` | `int` | Beginner 10, Intermediate 40, Expert 99; Custom 1..(width×height−9) (FR-002). |

**Validation rules** (`DifficultyConfig::isValidCustom()`, used by FR-003):
- `preset == Custom` ⇒ `5 <= width <= 16`, `5 <= height <= 16`, `1 <= mineCount <= width*height - 9`.
- Named presets are exact, fixed values — never run through the custom-bounds check (spec Edge
  Cases: "the three named presets... are not constrained by the 5–16 custom-size bounds").

**Factory functions**: `DifficultyConfig::beginner()`, `::intermediate()`, `::expert()`,
`DifficultyConfig::custom(width, height, mineCount)` — caller, i.e. `NewGameScreen`, is expected
to only call it with values already clamped by its stepper UI, so this is a defensive check, not
the primary UX gate. `custom()` itself never throws or clamps: it constructs the struct exactly
as given, even if out of bounds. Callers that can't already guarantee valid input (e.g.
`GameSession::fromJson` deserializing an untrusted file) MUST check `isValidCustom()`/`isValid()`
themselves before trusting the result.

## Board (class)

Owns `width`, `height`, `mineCount`, a `std::vector<Cell>` (row-major, size `width*height`), a
`bool minesPlaced_`, and a `Status status`.

### Status (enum)

`NotStarted | InProgress | Won | Lost` — mirrors `GameSession`'s persisted `status` field
(`Board` is the single source of truth; `GameSession::status()` delegates to it).

### Operations

| Method | Behavior |
|---|---|
| `openCell(x, y)` | No-op if `status != NotStarted && status != InProgress`, or cell is `Flagged` (FR-011), or already `Opened`. If `!minesPlaced_`: place mines excluding `(x,y)` (research.md #4), set `minesPlaced_ = true`, `status = InProgress`. If the opened cell `isMine`: `status = Lost` (FR-008). Else: reveal it, and if `adjacentMines == 0`, cascade-open all non-mined neighbors iteratively (research.md #5, FR-007). After any reveal, check win: if every non-mine cell is now `Opened`, `status = Won` (FR-017). |
| `toggleFlag(x, y)` | No-op if cell is `Opened`, or `status` is `Won`/`Lost` (FR-014, edge case: no board actions after game end). Otherwise flips `Unopened ⇄ Flagged`. Never changes `status`. |
| `chord(x, y)` | No-op unless cell is `Opened` with `adjacentMines > 0` and `status == InProgress`. Counts `Flagged` neighbors; if that count `!= adjacentMines`, no-op (FR-013). Otherwise opens every remaining `Unopened` neighbor via the same path as `openCell` (so a wrongly-flagged mine among them still triggers `Lost`, FR-008/FR-012 edge case), then re-checks win. |
| `flaggedCount()` | Count of cells in `Flagged` state. |
| `remainingMineCount()` | `mineCount - flaggedCount()` — may be negative (FR-015). |
| `cellAt(x, y)` | Read-only accessor the UI layer uses to decide tap routing (open vs. chord vs. flag) — see research.md #2. |

## GameSession (class)

Owns a `DifficultyConfig config`, a `Board board`, and `uint32_t elapsedSeconds` (accumulated
active-play time only — research.md #6).

| Method | Behavior |
|---|---|
| `openCell/toggleFlag/chord(x, y)` | Thin delegation to `board`. |
| `status()` | `board.status()`. |
| `addActiveSeconds(uint32_t)` | `elapsedSeconds += n`; no-op once `status()` is `Won`/`Lost` (elapsed time is frozen at game end, matching FR-018's "present... the elapsed time" as a fixed value). |
| `toJson()` / `static fromJson(text)` | See JSON shape below; `fromJson` throws `std::runtime_error` on any structural/range problem, exactly like `Counter::fromJson`. |

### game.json shape

```json
{
  "schemaVersion": 1,
  "difficulty": { "preset": "Beginner", "width": 9, "height": 9, "mineCount": 10 },
  "status": "InProgress",
  "elapsedSeconds": 42,
  "cells": [
    { "isMine": false, "state": "Opened" },
    { "isMine": true,  "state": "Flagged" }
  ]
}
```

- `cells` has exactly `width*height` entries, row-major, present whenever `status != NotStarted`
  (omitted/empty when `NotStarted`, since no mine layout exists yet — FR-005). `adjacentMines` is
  **not** stored; `fromJson` recomputes it per cell from the deserialized `isMine` layout.
- **Rejection rules** (FR-023, Constitution III "corrupted-input recovery"): missing/mismatched
  `schemaVersion`; `status` not one of the four valid strings; `difficulty` failing
  `DifficultyConfig` validation (preset's fixed values, or custom bounds); `cells.length !=
  width*height` when `status != NotStarted`; any `state` string not one of the three `CellState`
  values; mine count in `cells` not matching `difficulty.mineCount` when `status != NotStarted`.
  Any rejection throws; the caller (`AppImpl`, mirroring today's `Counter` handling) logs to
  stderr, discards the file via `persist::removeFile`, and starts at new-game/difficulty
  selection (FR-023) — **without** touching `settings.json`.

## ColorMode (enum)

`Color | BlackAndWhite` — FR-019. Default (when no `settings.json` exists yet): `BlackAndWhite`
(clarified default).

## Settings (class)

| Field | Type |
|---|---|
| `colorMode` | `ColorMode` |

`toJson()`/`fromJson()` mirror `Counter`'s pattern:

```json
{ "schemaVersion": 1, "colorMode": "BlackAndWhite" }
```

**Rejection rules**: missing/mismatched `schemaVersion`; `colorMode` not `"Color"` or
`"BlackAndWhite"`. On rejection, `AppImpl` discards the file and falls back to the default
(`BlackAndWhite`), independent of any `game.json` outcome (FR-023).

## State Transitions (Board/GameSession.status)

```text
NotStarted --openCell (first ever)--> InProgress
InProgress --openCell/chord hits a mine--> Lost
InProgress --last non-mine cell opened (direct or via cascade/chord)--> Won
Won, Lost  --(terminal; no operation changes status further)--
```

`toggleFlag` never transitions `status`. Once `Won`/`Lost`, `openCell`/`toggleFlag`/`chord` are
all no-ops (FR-018, edge cases).

## Relationships

```text
Settings           — standalone, persisted independently, survives across every GameSession.
DifficultyConfig ─┐
                   ├─ owned by → GameSession ─ owned by → App (one at a time; FR-024 governs
Board ────────────┘                                       replacing it with a new one)
```
