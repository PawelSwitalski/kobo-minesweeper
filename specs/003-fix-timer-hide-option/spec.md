# Feature Specification: Fix Game Timer & Add Hide-Timer Setting

**Feature Branch**: `003-fix-timer-hide-option`

**Created**: 2026-08-02

**Status**: Draft

**Input**: User description: "I found some problem with the clock, timer. It's not working well. Also in the settings should be option to hide the watch"

## Clarifications

### Session 2026-08-02

- Q: How precisely must the fixed timer's final recorded time match real elapsed play time? → A: Within 5 seconds of real elapsed time.
- Q: When the hide-timer setting is on, should the final elapsed time still appear on the win/loss outcome screen? → A: Yes — the timer is hidden only during live play; the final time still appears once the game ends.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Timer accurately tracks play time (Priority: P1)

While a player is actively solving a puzzle, the on-screen timer must count up in step with real elapsed time, whether the player is tapping cells rapidly, pausing to think, or a mix of both. Today the displayed time can fall noticeably behind the real time that has passed during active play, so players lose trust in the timer as a fair measure of their solving speed.

**Why this priority**: This is a correctness defect in a core piece of feedback the player relies on for every single game. It undermines the basic promise of a timed puzzle and must be fixed before any related settings work is meaningful.

**Independent Test**: Start a new game, play through it using a realistic mix of rapid taps and idle pauses while independently tracking real elapsed time (e.g., with a stopwatch), then compare the final displayed time on the win/loss screen against the independently tracked time. Can be fully tested without any settings changes and delivers a trustworthy timer on its own.

**Acceptance Scenarios**:

1. **Given** a new, in-progress game, **When** the player taps cells frequently and continuously for an extended period, **Then** the displayed elapsed time still advances in step with real time and does not fall behind.
2. **Given** a new, in-progress game, **When** the player alternates between bursts of rapid tapping and periods of no interaction, **Then** the total displayed elapsed time at game end is within 5 seconds of the real time the game was in progress.
3. **Given** an in-progress game, **When** the player pauses and returns to the main menu or opens Settings, **Then** the timer does not continue accumulating while outside active play, consistent with existing pause behavior.
4. **Given** a won or lost game, **When** the outcome screen is shown, **Then** the final recorded time reflects the actual duration of play as verified in Scenario 2.

---

### User Story 2 - Hide the timer during play (Priority: P2)

A player who finds the visible, counting timer distracting or stressful wants to turn it off. They go to Settings, enable a "hide timer" option, and the timer disappears from the board while they play; other board information stays exactly as before.

**Why this priority**: This is a standalone quality-of-life preference. It's valuable but depends on nothing from Story 1 to be usable, and delivers value to a subset of players who care about this specific distraction.

**Independent Test**: From Settings, enable the hide-timer option, start or resume a game, and confirm the timer is no longer shown on the board while all other board elements remain visible and functional. Then disable the option and confirm the timer reappears.

**Acceptance Scenarios**:

1. **Given** the player is in Settings, **When** they turn on the "hide timer" option, **Then** the setting is saved and takes effect immediately.
2. **Given** the hide-timer option is on, **When** the player is on the board screen playing a game, **Then** no elapsed-time display is shown on the board.
3. **Given** the hide-timer option is on, **When** the player finishes a game (win or loss), **Then** the final elapsed time for that game is shown on the outcome screen, since reviewing the result afterward does not create the ongoing distraction the setting exists to avoid.
4. **Given** the hide-timer option is off (default), **When** the player is on the board screen playing a game, **Then** the timer is shown exactly as it is today.
5. **Given** the hide-timer option is on, **When** the player turns it back off from Settings, **Then** the timer reappears on the board the next time it is shown.

---

### Edge Cases

- What happens if the player toggles the hide-timer setting while a game is already in progress and then returns to the board? The board must reflect the new setting immediately (timer shown or hidden) without needing to start a new game.
- What happens to time tracking used for pausing/resuming logic when the timer is hidden? Elapsed time must continue to be tracked and persisted internally exactly as when visible — only the on-screen display is affected, so save/resume and the final result time remain accurate.
- What happens if the device sleeps or the app is closed and reopened while the timer is hidden? The hidden state must persist across restarts, matching the persistence behavior of the existing color-mode setting.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST accumulate elapsed play time so that it reflects real wall-clock time the game was actively being played, regardless of how frequently or infrequently the player interacts with the board during that time.
- **FR-002**: The system MUST continue to exclude from elapsed play time any period during which the game is not actively in progress (e.g., paused at the main menu, in Settings, or while the device is asleep), consistent with existing pause behavior.
- **FR-003**: The displayed timer on the board MUST update to reflect newly elapsed time at least once per minute of active play, consistent with existing update behavior.
- **FR-004**: The final elapsed time shown on the win/loss outcome screen MUST be within 5 seconds of the total real time the game was actively in progress.
- **FR-005**: The Settings screen MUST offer a "hide timer" option that the player can turn on or off.
- **FR-006**: The hide-timer option MUST default to off (timer visible), preserving current behavior for players who don't change it.
- **FR-007**: When the hide-timer option is on, the board screen MUST NOT display the live elapsed-time indicator, while all other board information and controls remain visible and functional.
- **FR-008**: When the hide-timer option is on, the win/loss outcome screen MUST still display the final elapsed time for the completed game.
- **FR-009**: Changing the hide-timer option MUST take effect immediately on the board screen without requiring a new game to be started.
- **FR-010**: The hide-timer option MUST persist across app restarts, the same way the existing color-mode setting persists.
- **FR-011**: Hiding the timer MUST NOT affect how elapsed time is tracked, saved, or restored internally — only its on-screen visibility during active play changes.

### Key Entities

- **Game Session**: Represents an in-progress or completed puzzle, including its accumulated elapsed play time. The correctness fix changes how reliably this accumulated time reflects real elapsed time; it does not change what is stored.
- **Settings**: The player's saved preferences, already holding the color-mode choice. Gains one new preference: whether the timer is hidden during play.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: For a play session mixing rapid interaction and idle pauses, the timer displayed at game end is within 5 seconds of the independently measured real elapsed play time.
- **SC-002**: 100% of games played with heavy, continuous board interaction show a final elapsed time within 5 seconds of the real time spent, eliminating the under-counting defect.
- **SC-003**: A player can locate and enable the hide-timer setting, and confirm the timer is gone from the board, in under 15 seconds from opening Settings.
- **SC-004**: The hide-timer preference is still in effect 100% of the time after closing and reopening the app.

## Assumptions

- "The watch" in the user's request refers to the live elapsed-time indicator shown during play on the board screen (referred to here as "the timer"), not a separate clock-of-day display.
- Hiding the timer is a display-only preference; the underlying elapsed-time tracking, persistence, and final result reporting are unaffected, so no other feature (e.g., save/resume) is impacted.
- The hide-timer setting is a single global on/off preference (not per-difficulty or per-game), consistent with how the existing color-mode setting is scoped.
