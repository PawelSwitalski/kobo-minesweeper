# Feature Specification: Menu Layout, Exit Controls, and Mine-Count Colors Fixes

**Feature Branch**: `002-fix-menu-exit-colors`

**Created**: 2026-08-02

**Status**: Draft

**Input**: User description: "I found some mistakes
1) There should be better menu, In the "New Game" menu the "Custom" is look like not in this place. Let's the "Custom" place much more down around (20 dx).
2) I can not exit the game. The exit buttons should be in the main menu and in the game.
3) When I won the game or loos I can not return or exit.
4) In the color mode the numer that tell about a mines around should has different colors. Each '1' is blue, each '2' is green, each '3' is red, each '4' is deep blue, each '5' is cherry red, each '6' is cyan, each '7' is black, each '8' is gray"

## Clarifications

### Session 2026-08-02

- Q: Should tapping Exit during an active, unfinished game show a confirmation step before closing? → A: No confirmation, always — Exit closes the application immediately everywhere (main menu, in-game, game-over), with no dialog.
- Q: Should the Settings screen also get its own Exit control, or is Back-only acceptable there? → A: Back only on Settings — Settings keeps just its existing Back button; Exit remains reachable by going Back first (2 taps total from Settings).
- Q: How should the enlarged gap before the "Custom" section be sized? → A: ~2x normal spacing — the gap between the preset group and the Custom section MUST be twice the spacing used between the preset items themselves.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Exit the application from anywhere (Priority: P1)

A player wants to close the app. Today there is no in-app way to do this at all — the only options are killing the window externally (simulator) or letting the device idle-timeout, neither of which is a real "exit" a player can invoke on demand. The player needs a visible Exit control both on the main menu and while a game is in progress.

**Why this priority**: Not being able to quit on purpose is a fundamental usability failure that affects every session, regardless of what else works.

**Independent Test**: From a fresh app launch (main menu), tap Exit and confirm the app closes. Separately, start a game, tap Exit from within the game, and confirm the app closes.

**Acceptance Scenarios**:

1. **Given** the player is on the main menu (New Game screen), **When** they tap the Exit control, **Then** the application closes.
2. **Given** the player is in an active, unfinished game, **When** they tap the Exit control, **Then** the application closes and the in-progress game state is preserved for the next launch (no progress is lost).
3. **Given** the player is on the Settings screen, **When** they look for a way to quit, **Then** they find only the existing Back control there (Settings does not get its own Exit control); tapping Back returns them to a screen (main menu or board) where Exit is available, so the player is never stranded without a way to quit — at most 2 taps away.

---

### User Story 2 - Recover after winning or losing a game (Priority: P1)

A player who just won or lost a game is stuck: the game-over banner is plain text with no buttons, and the board no longer responds to taps. The player needs a way to either return to the main menu (to start a new game) or exit the application directly from this screen.

**Why this priority**: This traps the player at the end of every single game — the single most common moment in the app — making the app feel broken immediately after a natural stopping point.

**Independent Test**: Play a game to a win, then to a loss (separately), and from each outcome screen confirm both "return to menu" and "exit" controls are present and functional.

**Acceptance Scenarios**:

1. **Given** the player has just won a game, **When** the win banner is shown, **Then** controls to return to the main menu and to exit the application are both visible and usable.
2. **Given** the player has just lost a game, **When** the loss banner is shown, **Then** controls to return to the main menu and to exit the application are both visible and usable.
3. **Given** the player taps "return to menu" from either outcome, **When** the main menu appears, **Then** the finished game is no longer resumed on next launch (starting a new game or reopening the app begins fresh, not back at the same finished board).

---

### User Story 3 - Clearer New Game menu layout (Priority: P3)

A player opening the New Game menu currently sees "Custom" positioned as if it were a fourth item in the same group as Beginner/Intermediate/Expert, when it's actually a different kind of control (a header plus width/height/mine adjusters and its own start button). The player wants the fixed presets (Beginner/Intermediate/Expert, plus Settings) visually grouped together, and the Custom section moved further down with a clearly larger gap, so it reads as its own distinct block rather than a misplaced peer of the presets.

**Why this priority**: This is a layout clarity issue, not a blocker — the Custom controls still work today, they just look out of place.

**Independent Test**: Open the New Game menu and visually confirm the Beginner/Intermediate/Expert/Settings group is clustered together, followed by a gap twice the normal item spacing, followed by the distinctly-separated Custom section.

**Acceptance Scenarios**:

1. **Given** the player opens the New Game menu, **When** they view the layout, **Then** Beginner, Intermediate, Expert, and Settings appear grouped together above a gap that is twice the spacing used between those preset items.
2. **Given** the enlarged gap, **When** the player looks below it, **Then** the Custom section (header, adjusters, and its start button) appears as one visually separate block, not adjacent to the presets as if it were another preset choice.
3. **Given** the new layout, **When** the player interacts with any preset button or any Custom adjuster, **Then** all existing behavior (starting a preset game, adjusting width/height/mines, starting a custom game) continues to work unchanged.

---

### User Story 4 - Distinct colors per mine-count number in Color mode (Priority: P4)

A player using Color mode on a color-capable device wants each mine-count digit (1-8) shown on revealed cells to have its own distinct color, matching the classic convention, instead of every digit sharing one accent color.

**Why this priority**: This is a visual/cosmetic refinement to an already-functional feature (the digits are already legible and correct without it); it doesn't block or break anything for players in Black & White mode or on monochrome devices.

**Independent Test**: In Color mode on a color-capable device, reveal cells exposing each of the digits 1 through 8 and confirm each digit renders in its assigned color.

**Acceptance Scenarios**:

1. **Given** Color mode is enabled and the device supports color, **When** a revealed cell shows the digit 1, **Then** it renders in blue; digit 2 in green; digit 3 in red; digit 4 in deep blue; digit 5 in cherry red; digit 6 in cyan; digit 7 in black; digit 8 in gray.
2. **Given** Black & White mode is enabled, or the device does not support color, **When** any mine-count digit is shown, **Then** it continues to render using the existing grayscale/shade scheme, unaffected by the new color mapping.
3. **Given** Color mode with the new per-digit colors, **When** a player without color vision compares two digits, **Then** the digit's shape/glyph alone (not color) still unambiguously conveys the count, since color remains an accent rather than the sole signal.

---

### Edge Cases

- What happens if the player taps Exit while adjusting the Custom width/height/mine steppers but hasn't started a game? The application closes; only the unsaved in-progress configuration is discarded (no started game exists yet to lose).
- What happens if the player taps Exit mid-reveal (e.g., during a flood-fill of empty cells)? The already-persisted game state must not be corrupted; the exit must not occur in a way that leaves a partially-written save.
- What happens when the player returns to the main menu after a win/loss and then re-launches the app later — does it resume the finished board? No: once the player has returned to the menu, the finished game is cleared from resume state (see US2 acceptance scenario 3).
- How does the enlarged gap and new Exit/return controls interact with existing HUD elements (Flag Mode, Settings) on the board screen and outcome banner? They must coexist without overlapping or crowding existing touch targets.
- Do the new Exit controls require a confirmation step? No — confirmed: Exit always closes the application immediately with no confirmation dialog, on the main menu, in-game, and on the game-over screen alike, since game state is already persisted continuously (existing behavior), so exiting never discards recoverable progress.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST display a visible, tappable Exit control on the main menu (New Game screen) that closes the application when activated.
- **FR-002**: System MUST display a visible, tappable Exit control while a game is in progress (board screen) that closes the application when activated.
- **FR-003**: System MUST preserve the in-progress game state when the player exits via either Exit control, so no progress is lost (consistent with existing persistence behavior).
- **FR-004**: System MUST display, on the game-over presentation for both a win and a loss, a control that returns the player to the main menu.
- **FR-005**: System MUST display, on the game-over presentation for both a win and a loss, a control that exits the application.
- **FR-006**: System MUST clear the finished game from "resume on next launch" state once the player returns to the main menu from a game-over screen.
- **FR-007**: The New Game menu MUST visually group Beginner, Intermediate, Expert, and Settings together as the primary preset options.
- **FR-008**: The New Game menu MUST position the Custom section (header, width/height/mine adjusters, and its start button) below the preset group, separated by a gap that is twice the spacing used between the preset items themselves, so it reads as a distinct block rather than an additional preset choice.
- **FR-009**: The repositioned New Game menu MUST preserve all existing behavior of the preset buttons and Custom adjusters/start button unchanged.
- **FR-010**: In Color mode on a color-capable device, System MUST render each mine-count digit (1 through 8) shown on revealed cells in a distinct assigned color: 1=blue, 2=green, 3=red, 4=deep blue, 5=cherry red, 6=cyan, 7=black, 8=gray.
- **FR-011**: In Black & White mode, or on a device without color support, System MUST continue rendering mine-count digits using the existing grayscale/shade scheme, unaffected by FR-010.
- **FR-012**: System MUST continue to convey each mine count through its digit glyph regardless of color, so the count remains unambiguous without relying on color perception.
- **FR-013**: All new Exit and return-to-menu controls MUST be reachable via touch input at the same touch-target sizing standard used by existing controls in the app.
- **FR-014**: Every Exit control (main menu, in-game, game-over) MUST close the application immediately upon activation, with no confirmation step.
- **FR-015**: The Settings screen MUST NOT gain its own Exit control; it keeps only its existing Back control, and reaching Exit from Settings MUST require navigating Back first.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of players can quit the application from the main menu in 1 tap, without relying on any action outside the app.
- **SC-002**: 100% of players can quit the application while a game is in progress in 1 tap, without relying on any action outside the app.
- **SC-003**: 100% of players can, immediately after any game ends (win or loss), either return to the main menu or exit the application in 1 tap, with 0% needing to rely on any action outside the app to proceed.
- **SC-004**: 0% of in-progress games are lost when a player exits via any Exit control (verified by relaunching and confirming the game resumes exactly where it left off).
- **SC-005**: In a visual review of the New Game menu, all reviewers correctly identify the Custom section as a separate block from the Beginner/Intermediate/Expert/Settings group, rather than mistaking it for an additional preset option, on first look. The measured gap above the Custom section is twice the spacing measured between the preset items.
- **SC-006**: In Color mode on a color-capable device, all 8 mine-count digits are visually distinguishable from one another by color alone, confirmed by side-by-side visual comparison.

## Assumptions

- "Exit" means fully closing/terminating the application (equivalent to the existing window-close/idle-timeout behavior already handled by `requestExit`), not merely navigating to a different in-app screen.
- No confirmation dialog is required before exiting, since game state is already persisted continuously; exiting therefore never discards recoverable progress. This matches the app's existing idle-timeout and window-close behavior, which also exit without confirmation.
- "Return" after a win or loss means returning to the main New Game menu screen, not automatically starting a new game.
- "Deep blue" and "cherry red" are interpreted as a dark navy blue and a dark crimson/maroon red respectively, chosen so that all eight digit colors (including the existing "1=blue" and "3=red") remain distinguishable from one another. This matches the classic Minesweeper digit-color convention the user is referencing.
- The per-digit color mapping (FR-010) applies only when Color mode is active and the device supports color; devices without color support, or players who have selected Black & White mode, are unaffected and keep today's grayscale rendering.
- No new persisted data is introduced by this feature; existing settings (color mode) and existing save/resume state are reused as-is.
