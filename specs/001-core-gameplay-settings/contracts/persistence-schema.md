# Contract: Persisted JSON schemas

Both files are written with `persist::saveFileAtomic` (unchanged) and read with
`persist::loadFile` + a throwing `fromJson` (unchanged pattern from `Counter`). Full field-level
detail lives in `data-model.md`; this document is the stable interface other tools/tests can
depend on.

## `game.json`

```json
{
  "schemaVersion": 1,
  "difficulty": { "preset": "Beginner", "width": 9, "height": 9, "mineCount": 10 },
  "status": "InProgress",
  "elapsedSeconds": 42,
  "cells": [ { "isMine": false, "state": "Opened" } ]
}
```

- `preset` ∈ `{Beginner, Intermediate, Expert, Custom}`; `status` ∈
  `{NotStarted, InProgress, Won, Lost}`; `state` (per cell) ∈ `{Unopened, Opened, Flagged}`.
- `cells.length == width*height` whenever `status != "NotStarted"`; omitted/empty otherwise.
- `adjacentMines` is deliberately **not** part of this schema — always recomputed from `isMine`
  on load (see data-model.md §GameSession).
- Any structural or range violation → `fromJson` throws → caller discards the file and starts a
  fresh `NotStarted` session (FR-023), leaving `settings.json` untouched.

## `settings.json`

```json
{ "schemaVersion": 1, "colorMode": "BlackAndWhite" }
```

- `colorMode` ∈ `{Color, BlackAndWhite}`.
- Any violation → `fromJson` throws → caller discards the file and falls back to the default
  (`BlackAndWhite`), independent of `game.json`'s state (FR-023).

## Versioning

Both schemas start at `schemaVersion: 1`. A future breaking change bumps the version and adds a
migration or an explicit rejection — out of scope for this feature (no existing installs to
migrate from, since these files don't exist yet).
