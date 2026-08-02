# Contract: `ui::applyScreenRefreshInterval` and its call sites

Governs the new `src/ui/theme.h/.cpp` free function that pushes `Settings::screenRefreshInterval`
into the `Renderer`'s existing (but previously unused) ghosting-promotion policy (research.md #1,
#3; data-model.md).

## `ui::applyScreenRefreshInterval`

```cpp
namespace minesweeper::ui {

// Maps a player's chosen ScreenRefreshInterval onto Renderer::setGhostingInterval's existing
// count-based contract (renderer.h). Never→0 relies on that contract's own "n <= 0 disables
// auto-promotion" semantics -- no new sentinel is introduced.
void applyScreenRefreshInterval(Renderer& renderer, core::ScreenRefreshInterval interval);

}  // namespace minesweeper::ui
```

| `interval` | `renderer.setGhostingInterval(...)` argument |
|---|---|
| `Every5` | `5` |
| `Every10` | `10` |
| `Every25` | `25` |
| `Never` | `0` |

**Invariants**:
1. **Pure translation, no state of its own.** The function holds no state between calls; every
   call fully determines the `Renderer`'s ghosting-promotion count from the `interval` argument
   alone. Calling it twice with the same `interval` is idempotent.
2. **Renders the platform-level contract unreachable in an inconsistent state.** Because `Never`
   maps to `0`, and `Renderer::setGhostingInterval`'s own documented contract already treats `n <=
   0` as "disable auto-promotion" (`renderer.h:66-67`), there is no way to select a `Settings`
   value that leaves the renderer in an undefined or partially-applied state.
3. **No-op on backends without a ghosting concept.** `SdlRenderer` never overrides
   `setGhostingInterval`, so it inherits the `Renderer` base class's empty default implementation —
   calling `applyScreenRefreshInterval` on the desktop simulator is always a harmless no-op,
   satisfying FR-007 with zero simulator-specific code.

## Call sites (`src/main.cpp`, `AppImpl`)

```cpp
AppImpl(...) {
    // ... load settings_ ...
    minesweeper::ui::applyColorMode(theme_, renderer_.info(), settings_.colorMode);
    minesweeper::ui::applyScreenRefreshInterval(renderer_, settings_.screenRefreshInterval);  // NEW
    // ... load session_ ...
}

void autosaveSettings() override {
    minesweeper::persist::saveFileAtomic(paths_.settings, settings_.toJson());
    minesweeper::ui::applyColorMode(theme_, renderer_.info(), settings_.colorMode);
    minesweeper::ui::applyScreenRefreshInterval(renderer_, settings_.screenRefreshInterval);  // NEW
}
```

- **Startup**: the constructor call ensures a previously-saved choice governs refresh behavior from
  the very first partial refresh of the session, not only after the player revisits Settings
  (research.md #3's rejected alternative would have missed this).
- **On change**: `autosaveSettings()`'s call ensures FR-006 (changes take effect immediately) — the
  very next `flushPartial()` call after a settings change uses the new threshold.
- No other call site exists. `SettingsScreen`'s four button handlers only mutate
  `app_.settings().screenRefreshInterval` and call `app_.autosaveSettings()`, exactly like every
  other Settings control already does — they never call `applyScreenRefreshInterval` or
  `renderer().setGhostingInterval` directly.
