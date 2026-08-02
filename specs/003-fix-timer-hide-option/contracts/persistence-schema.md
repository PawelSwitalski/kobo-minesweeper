# Contract: `settings.json` gains `hideTimer` (delta over `001`)

`src/core/settings.h/.cpp`'s persisted shape gains one field; `game.json`
(`001-core-gameplay-settings/contracts/persistence-schema.md`) is entirely unchanged by this
feature. This document covers only the delta.

## `settings.json`

```json
{ "schemaVersion": 1, "colorMode": "BlackAndWhite", "hideTimer": false }
```

- `hideTimer`: `bool`, default `false`. Read via `j.value("hideTimer", false)` — **absent is valid**
  and means `false`, unlike every other field in this schema (which use `.at()`/throw-on-missing).
  This is deliberate: it lets an already-existing `settings.json` written before this feature (which
  cannot have this key) continue to load successfully with its `colorMode` intact, instead of being
  discarded and recreated from scratch (research.md #4).
- `schemaVersion` stays `1` — this is an additive, backward-compatible change, not a breaking one.
- All other validation is unchanged: `colorMode` ∈ `{Color, BlackAndWhite}`, malformed JSON /
  wrong `schemaVersion` / invalid `colorMode` value still cause `fromJson` to throw and the caller
  to discard the file and fall back to all-defaults, exactly as `001`'s contract already specifies.

## Versioning

No version bump for this change (see research.md #4 for why an additive field with a spec-mandated
default is handled via `.value(..., default)` rather than a `schemaVersion` bump). A future field
with no reasonable default would still warrant bumping to `2` per `001`'s existing versioning note.
