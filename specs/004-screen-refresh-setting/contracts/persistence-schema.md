# Contract: `settings.json` gains `screenRefreshInterval` (delta over `003`)

`src/core/settings.h/.cpp`'s persisted shape gains one field; `game.json` is entirely unchanged by
this feature. This document covers only the delta over `003-fix-timer-hide-option`'s
`contracts/persistence-schema.md`.

## `settings.json`

```json
{
  "schemaVersion": 1,
  "colorMode": "BlackAndWhite",
  "hideTimer": false,
  "screenRefreshInterval": "Every10"
}
```

- `screenRefreshInterval`: string enum, one of `"Every5"`, `"Every10"`, `"Every25"`, `"Never"`.
  Default `"Every10"`. Read via `j.value("screenRefreshInterval", "Every10")` — **absent is
  valid** and means `"Every10"`, following the exact precedent `hideTimer` established
  (`003-fix-timer-hide-option/research.md` #4): an already-existing `settings.json` written before
  this feature (which cannot have this key) continues to load successfully with its `colorMode`
  and `hideTimer` intact, instead of being discarded and recreated from scratch.
- Any other string value (present but not one of the four recognized values) is invalid —
  `fromJson` throws, and the caller discards the whole file and falls back to all-defaults, exactly
  as an invalid `colorMode` string already does.
- `schemaVersion` stays `1` — this is an additive, backward-compatible change, not a breaking one.

## Versioning

No version bump for this change, for the same reason `hideTimer` didn't need one: an additive
field with a spec-mandated default (FR-003), handled via `.value(key, default)` rather than a
`schemaVersion` bump.
