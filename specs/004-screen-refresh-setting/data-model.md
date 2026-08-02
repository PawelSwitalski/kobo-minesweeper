# Phase 1 Data Model: Screen Refresh Frequency Setting

This feature makes **one additive, backward-compatible change** to `settings.json`
(`schemaVersion` stays `1`) and introduces **one new non-persisted free function**. `game.json` is
completely unchanged.

## `ScreenRefreshInterval` (enum, `src/core/settings.h`) — NEW

```cpp
enum class ScreenRefreshInterval { Every5, Every10, Every25, Never };
```

| Value | Meaning | Maps to (`Renderer::setGhostingInterval` count) |
|---|---|---|
| `Every5` | Full clearing refresh after every 5 screen updates | `5` |
| `Every10` | Full clearing refresh after every 10 screen updates (**default**) | `10` |
| `Every25` | Full clearing refresh after every 25 screen updates | `25` |
| `Never` | No automatic ghosting-driven full refresh | `0` (renderer's existing "disabled" sentinel) |

## `Settings` (`src/core/settings.h/.cpp`)

| Field | Type | Default | Introduced by |
|---|---|---|---|
| `colorMode` | `ColorMode` (`Color` \| `BlackAndWhite`) | `BlackAndWhite` | `001` |
| `hideTimer` | `bool` | `false` | `003` |
| `screenRefreshInterval` | `ScreenRefreshInterval` | `Every10` | `004` (this feature) |

**Validation rule**: `fromJson` reads `screenRefreshInterval` via
`j.value("screenRefreshInterval", "Every10")` — absent in an already-existing `settings.json`
(written before this feature) is treated as `"Every10"`, matching FR-003's default, then parsed
through the same string→enum conversion used for a present value. An unrecognized string (neither
absent nor one of the four valid values) is treated as invalid input and rejected, exactly like an
invalid `colorMode` string already is — the caller discards the whole file and falls back to
all-defaults (`persistence-schema.md`'s existing `001` contract, unchanged).

**Persisted shape** (delta over `003`'s `contracts/persistence-schema.md`):

```json
{
  "schemaVersion": 1,
  "colorMode": "BlackAndWhite",
  "hideTimer": false,
  "screenRefreshInterval": "Every10"
}
```

## `ui::applyScreenRefreshInterval` (`src/ui/theme.h/.cpp`) — NEW, not persisted

```cpp
void applyScreenRefreshInterval(Renderer& renderer, core::ScreenRefreshInterval interval);
```

- **Behavior**: maps `interval` to a count per the table above and calls
  `renderer.setGhostingInterval(count)`.
- **Not persisted**: a pure "push this Settings value into the platform object" function, the same
  category as `applyColorMode` (which pushes `colorMode` into `Theme::color`), just targeting
  `Renderer` instead of `Theme`.
- **Call sites**: `AppImpl`'s constructor (once, at startup, right after loading `settings_`) and
  `AppImpl::autosaveSettings()` (on every settings change) — see
  `contracts/screen-refresh-application.md`.

## `SettingsScreen` (`src/ui/screens/settings_screen.h/.cpp`)

New non-persisted members (screen-local UI state, same category as the existing `colorButton_`):

| Field | Type | Purpose |
|---|---|---|
| `screenRefreshLabelRect_` | `Rect` | Position for the "Screen Refresh" section heading, computed in `layout()` (the same reason `BoardScreen` stores `mineCountRect_`/`timerRect_` as fields rather than recomputing inline in `draw()`). |
| `refresh5Button_` | `Button` | Mutually exclusive with the other three; `toggled` when `screenRefreshInterval == Every5`. |
| `refresh10Button_` | `Button` | `toggled` when `screenRefreshInterval == Every10`. |
| `refresh25Button_` | `Button` | `toggled` when `screenRefreshInterval == Every25`. |
| `refreshNeverButton_` | `Button` | `toggled` when `screenRefreshInterval == Never`. |

## Relationships (delta over `001`/`002`/`003`)

```text
App
├─ settings(): Settings          — gains screenRefreshInterval; read by SettingsScreen's four new
│                                   buttons (draw/toggle) and by AppImpl (startup + autosave)
└─ renderer(): Renderer          — unchanged interface; setGhostingInterval(int) already existed
                                    (001) and is now actually called, via the new
                                    ui::applyScreenRefreshInterval, from AppImpl's constructor and
                                    autosaveSettings() — mirroring applyColorMode's existing
                                    call-site pattern exactly
```
