# Feature Specification: Screen Refresh Frequency Setting

**Feature Branch**: `004-screen-refresh-setting`

**Created**: 2026-08-02

**Status**: Draft

**Input**: User description: "in the settings add refresh screen settings (5, 10, 25, never)"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Control how often the screen fully refreshes (Priority: P1)

A player on an e-ink device notices the screen can accumulate visual ghosting (faint traces of
previous cell states) during a long play session, since most moves only redraw the changed area
instead of the whole screen. They open Settings and choose how often the display should
automatically perform a full, ghosting-clearing refresh: every 5, every 10, or every 25 screen
updates, or never automatically. A player who prefers the cleanest possible screen picks a low
number; a player who prefers fewer distracting full-screen flashes and faster response picks a
higher number or turns it off entirely.

**Why this priority**: This is the entire feature — a single, self-contained settings control. It
stands alone with no dependency on anything else.

**Independent Test**: From Settings, select each of the four available values in turn and confirm
each is saved. Play a game with each value selected and confirm a full clearing refresh visibly
occurs at the expected cadence (or never, for the "Never" option) during play on a device where
this is visible.

**Acceptance Scenarios**:

1. **Given** the player is in Settings, **When** they select "Every 5", "Every 10", "Every 25", or
   "Never" for the screen refresh setting, **Then** the choice is saved and takes effect
   immediately.
2. **Given** "Every 10" (or any "Every N") is selected, **When** the player accumulates N screen
   updates during play, **Then** the display performs one full clearing refresh, after which the
   count starts over for the next N updates.
3. **Given** "Never" is selected, **When** the player continues playing for an extended session,
   **Then** the display does not automatically insert extra full refreshes to clear ghosting —
   normal full refreshes that happen for other reasons (finishing a game, switching screens, the
   device waking up) still occur exactly as they otherwise would.
4. **Given** a value has been selected, **When** the app is closed and reopened, **Then** the same
   value is still selected in Settings and still governs refresh behavior.
5. **Given** the player is running the desktop/simulator version of the app (which has no visible
   ghosting), **When** they select any value, **Then** the choice is still saved but has no
   observable effect on that version — the setting itself is always available regardless of
   device.

---

### Edge Cases

- Changing the value mid-game takes effect immediately for subsequent screen updates; no new game
  or restart is required.
- "Never" only disables the *automatic, ghosting-driven* full refresh. It does not disable full
  refreshes that already happen for other reasons (e.g., when a game ends, when navigating between
  screens, or when the device wakes from sleep) — those continue unaffected by this setting.
- Before a player ever changes this setting, the app behaves using the default value (see FR-003).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The Settings screen MUST provide a "Screen Refresh" control offering exactly four
  selectable values: Every 5, Every 10, Every 25, and Never.
- **FR-002**: The selected value MUST persist across app restarts.
- **FR-003**: Before a player changes it, the setting MUST default to "Every 10".
- **FR-004**: When set to "Every N", the system MUST automatically perform one full, ghosting-
  clearing screen refresh after every N screen updates during play, then resume counting from zero
  for the next N.
- **FR-005**: When set to "Never", the system MUST NOT automatically insert a full refresh based on
  accumulated screen updates. Full refreshes that occur for other reasons (finishing a game,
  navigating between screens, waking from sleep) are unaffected and continue to occur normally.
- **FR-006**: Changing the value MUST take effect immediately, without requiring a new game or an
  app restart.
- **FR-007**: On a device or platform where screen ghosting is not a meaningful concern (e.g., the
  desktop simulator), the setting MUST still be selectable and persisted, even though it has no
  observable effect there — consistent with how the existing Color/Black-and-white setting is
  always available regardless of the device's color support.

### Key Entities

- **Settings**: The player's saved preferences, already holding color mode and the hide-timer
  choice. Gains one new preference: the screen refresh frequency (Every 5 / Every 10 / Every 25 /
  Never).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A player can find and change the Screen Refresh setting in under 15 seconds from
  opening Settings.
- **SC-002**: With any "Every N" value selected, a full clearing refresh visibly occurs at least
  once within every N consecutive screen updates during gameplay, on a device where ghosting is
  visible.
- **SC-003**: With "Never" selected, no automatic ghosting-driven full refresh occurs during an
  extended play session, while refreshes triggered by other events (game end, screen navigation)
  still occur exactly as they would with any other value selected.
- **SC-004**: The selected value survives 100% of app restarts.

## Assumptions

- The four offered values (5, 10, 25, Never) represent a count of screen updates between automatic
  full refreshes, not a time interval — matching the "how many small updates before one big clean
  redraw" framing implied by the requested options.
- Default value is "Every 10", a reasonable middle ground between a very clean screen (low counts,
  more frequent full-screen flashing) and minimal interruption (high counts or Never, more
  potential ghosting) absent any stated player preference.
- This is a single global preference, not per-difficulty or per-game, consistent with how the
  existing Color and Hide Timer settings are scoped.
- The setting is shown and persisted on every platform the app runs on, even though its effect is
  only observable on hardware where partial screen updates can visibly ghost.
