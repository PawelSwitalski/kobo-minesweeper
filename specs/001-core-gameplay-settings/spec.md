# Feature Specification: Core Minesweeper Gameplay & Display Settings

**Feature Branch**: `001-core-gameplay-settings`

**Created**: 2026-08-01

**Status**: Draft

**Input**: User description: "Core Minesweeper gameplay logic and settings, for the Kobo e-ink Minesweeper app. Feature scope: the core Minesweeper game — board generation, cell states, gameplay rules, and a settings screen that includes a color-mode toggle. Board & cells: rectangular grid of cells scattered with mines; three cell states (unopened, opened, flagged). Difficulty presets Beginner (9x9, 10 mines), Intermediate (16x16, 40 mines), Expert (30x16, 99 mines), plus customizable board size/mine count. Core interactions: open (loss if mine, else reveal number/blank with cascade flood-fill on blank), flag/unflag (touch-appropriate gesture, no move/game-end consequence), chording (open all unflagged neighbors when adjacent flag count matches the cell's number, with a touch-appropriate trigger), first-click safety (first opened cell is never a mine). HUD: remaining mine count (total minus flags, can go negative) and elapsed time (no per-second redraw per constitution). Win when all non-mine cells opened; loss when a mine is opened (including via chording); game end shows outcome, difficulty, elapsed time. Settings: color-mode switch (color / black-and-white), persisted across restarts, board legible by shape/contrast alone in black-and-white; difficulty/board-size selection as part of starting a new game. Out of scope: pixel-level visual theming beyond color/black-white, leaderboards, undo/redo, hints, multiplayer."

## Clarifications

### Session 2026-08-01

- Q: Starting a new game (e.g., picking a different difficulty) while one is already in progress — what should happen to the in-progress game? → A: Requires an explicit confirmation ("Abandon current game?") before proceeding; only discarded after the player confirms.
- Q: Does elapsed time keep counting in real time while the app is closed/backgrounded, or does it pause and count only active play time? → A: Elapsed time pauses while the app is closed/backgrounded and counts only active play time.
- Q: Does long-press on an unopened cell still flag/unflag it while Flag Mode is toggled on, or does Flag Mode change/disable long-press's behavior? → A: Long-press still flags/unflags the cell exactly as when Flag Mode is off — long-press behavior is mode-independent.
- Q: SC-006 claims chording "cuts taps needed by at least 80%," but a corner cell has only 3 neighbors (best case 66.7% reduction), making the blanket 80% claim geometrically unachievable for every cell — how should this be fixed? → A: Reframe the metric as action-count instead of percentage: chording opens all N remaining safe neighbors in 1 action instead of N separate taps — measurable and universally true regardless of neighbor count.
- Q: SC-006 (as reframed) claims chording works "for every value of N from 1 to 8," but N=8 is unreachable — a chordable cell needs at least 1 adjacent mine, capping the safe-neighbors-opened count at 7 for a standard 8-neighbor interior cell — how should this be fixed? → A: Narrow the claim to "for every value of N from 1 to 7," the actual achievable range for a standard interior cell.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Play a game to a win or a loss (Priority: P1)

A player picks a difficulty preset, opens cells to deduce and clear the board, and the game correctly ends as either a win (all safe cells opened) or a loss (a mine opened), showing the outcome, the difficulty used, and how long it took.

**Why this priority**: This is the entire reason the app exists. Without a correctly generated board, safe first click, cascading reveal, and accurate win/loss detection, there is no game — every other story only adds convenience or polish on top of this loop.

**Independent Test**: Can be fully tested by starting a Beginner game, opening cells until either every safe cell is revealed (win) or a mine is hit (loss), and confirming the end-of-game outcome, difficulty, and elapsed time are shown correctly — no flagging or chording required.

**Acceptance Scenarios**:

1. **Given** a new Beginner game has just been started, **When** the player opens any cell as their first action, **Then** that cell is never a mine.
2. **Given** an in-progress game, **When** the player opens a cell that is not a mine and has one or more adjacent mines, **Then** the cell reveals the exact count of adjacent mines and the game continues.
3. **Given** an in-progress game, **When** the player opens a cell that is not a mine and has zero adjacent mines, **Then** that cell and all of its non-mined neighbors are automatically opened, and this cascade continues through every newly revealed zero-count cell.
4. **Given** an in-progress game, **When** the player opens a cell containing a mine, **Then** the game immediately ends as a loss and no further cell can be opened, flagged, or chorded.
5. **Given** an in-progress game where every non-mine cell has just been opened, **Then** the game immediately ends as a win.
6. **Given** a game has just ended (win or loss), **Then** the outcome, the difficulty/board configuration played, and the elapsed time are all displayed.

---

### User Story 2 - Flag suspected mines (Priority: P2)

While deducing the board, a player marks cells they believe contain a mine so they can track their reasoning and avoid opening them by mistake, using either a long-press or an explicit Flag Mode toggle suited to the e-ink touchscreen.

**Why this priority**: Flagging is the primary tool players use to think out loud on the board and protect their progress from mis-taps; it is not required to win a game but is core to how Minesweeper is played.

**Independent Test**: Can be fully tested by long-pressing an unopened cell to flag it, confirming it cannot be opened until unflagged, then enabling Flag Mode and confirming a plain tap on an unopened cell also flags/unflags it, all without needing to trigger a chord or finish the game.

**Acceptance Scenarios**:

1. **Given** an unopened cell, **When** the player long-presses it, **Then** the cell becomes flagged, and long-pressing it again returns it to unopened.
2. **Given** Flag Mode is toggled on, **When** the player taps an unopened cell, **Then** the cell is flagged/unflagged instead of opened.
3. **Given** a flagged cell, **When** the player attempts a normal open action on it, **Then** the cell does not open until it is first unflagged.
4. **Given** an in-progress game with the mine counter showing some value, **When** the player flags or unflags a cell, **Then** the displayed remaining-mine count updates accordingly (total mines minus current flags) and may go negative if flags exceed the mine total, and the game's win/loss state is unaffected by this action alone.

---

### User Story 3 - Chord an opened cell to clear neighbors quickly (Priority: P2)

Once a player has flagged all the mines around an already-opened numbered cell, they tap that cell again to instantly open all of its remaining unflagged neighbors, instead of opening each one individually.

**Why this priority**: Chording is a well-known efficiency mechanic that experienced players expect; it doesn't change what's achievable but significantly changes how quickly a deduced region can be cleared.

**Independent Test**: Can be fully tested by opening a numbered cell, flagging exactly the number of adjacent mines it indicates, tapping the opened cell again, and confirming all remaining adjacent cells open at once (or, with a deliberately wrong flag, confirming the chord ends the game as a loss).

**Acceptance Scenarios**:

1. **Given** an opened numbered cell whose adjacent flagged-cell count equals its own number, **When** the player taps that opened cell, **Then** every remaining unopened, unflagged adjacent cell opens at once.
2. **Given** the same setup as above but one flag is on the wrong cell, **When** the chord action opens a cell that is actually a mine, **Then** the game immediately ends as a loss.
3. **Given** an opened numbered cell whose adjacent flagged-cell count does NOT equal its own number, **When** the player taps that opened cell, **Then** no adjacent cells open and the board is unchanged.

---

### User Story 4 - Switch and keep a color or black-and-white display (Priority: P2)

A player opens settings and switches the board display between "Color" and "Black-and-white," and the choice is legible and remains in effect the next time they open the app.

**Why this priority**: The app must work identically well on monochrome and color e-ink hardware per the project's constitution; this is a small but constitution-mandated setting that must be verified as correct now rather than retrofitted later.

**Independent Test**: Can be fully tested by opening settings, switching color mode, confirming all board states (unopened/opened/flagged, each number) remain distinguishable in black-and-white, then restarting the app and confirming the same mode is still selected.

**Acceptance Scenarios**:

1. **Given** the settings area, **When** the player switches the color-mode control to "Black-and-white," **Then** every cell state (unopened, opened, flagged) and every adjacency number remains distinguishable using shape, symbol, or contrast alone.
2. **Given** the player has selected a color mode, **When** the app is closed and reopened, **Then** the same color mode is active without the player needing to reselect it.

---

### User Story 5 - Start a game with a custom board size (Priority: P3)

Instead of a preset, a player specifies their own board width, height, and mine count to start a new game at a difficulty of their choosing.

**Why this priority**: Custom games are a nice-to-have for players who outgrow the three presets, but the app is fully playable and testable without this story.

**Independent Test**: Can be fully tested by entering a custom width, height, and mine count within the supported bounds, starting the game, and confirming it behaves exactly like a preset game of that size; and by attempting an out-of-bounds configuration and confirming it is refused with a clear reason.

**Acceptance Scenarios**:

1. **Given** the new-game setup, **When** the player enters a custom width and height each between 5 and 16 cells and a mine count between 1 and (width × height − 9), **Then** a new game starts on a board of exactly that size and mine count.
2. **Given** the new-game setup, **When** the player enters a width, height, or mine count outside those bounds, **Then** the game is not started and the player is shown why the configuration is invalid.

---

### Edge Cases

- Opening a currently-flagged cell via a normal open action must be a no-op (per US2, AS3) — it must not open, and must not consume the action as if it had.
- Chording a cell whose adjacent flagged count matches its number, but where none of the remaining unflagged neighbors are actually unopened (already cleared), must be a no-op with no effect on game state.
- A chord or open action that completes the win condition (opens the last remaining non-mine cell) must register as a win, not simply "no visible effect."
- A chord action where the flag mismatch causes a mine to open must end the game as a loss exactly as an ordinary mine-open would (US3, AS2), including disabling further board actions.
- Once a game has ended (win or loss), any further tap, long-press, or Flag Mode interaction on the board must have no effect on board or game state.
- A custom configuration at the minimum bound (5×5 board, 1 mine) and at the maximum bound for a given size (mine count = width×height − 9) must both be startable and playable like any other board.
- If the app is closed before the first cell of a game is opened, relaunching must resume at that same not-yet-started state rather than losing the selected difficulty/configuration.
- If the persisted in-progress game data is missing, unreadable, or invalid, the app must discard only that game data and return the player to new-game/difficulty selection, without crashing and without affecting the persisted color-mode setting.
- The three named difficulty presets (including Expert at 30×16) are exact, fixed configurations and are not constrained by the 5–16 custom-size bounds that apply to custom games.
- If the player chooses to start a new game (any preset or custom configuration) while a game is already in progress, the system must ask for explicit confirmation before discarding the in-progress game; declining the confirmation leaves the in-progress game untouched and does not start a new one.
- Elapsed time does not advance while the app is closed or backgrounded; resuming a game continues accumulating time from the paused value rather than including the time away.
- Long-press on an unopened cell flags/unflags it the same way whether Flag Mode is on or off; Flag Mode only changes what a plain tap does.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST offer three built-in difficulty presets when starting a new game — Beginner (9×9 board, 10 mines), Intermediate (16×16 board, 40 mines), and Expert (30×16 board, 99 mines).
- **FR-002**: System MUST allow a player to start a custom game by specifying width and height, each independently between 5 and 16 cells, and a mine count between 1 and (width × height − 9).
- **FR-003**: System MUST prevent a custom game from starting when the specified width, height, or mine count falls outside the bounds in FR-002, and MUST make the reason clear to the player.
- **FR-004**: Every cell on a game board MUST at all times be in exactly one of three states: unopened, opened, or flagged.
- **FR-005**: System MUST place all mines only after the player's first cell-open action of a new game, and MUST guarantee the specific cell first opened is never a mine.
- **FR-006**: When a player opens an unopened, unflagged, non-mine cell, the system MUST reveal it showing either the count of mines among its up-to-eight horizontally, vertically, and diagonally adjacent cells, or a blank indicator when that count is zero.
- **FR-007**: When a revealed cell's adjacent-mine count is zero, the system MUST automatically open all of its non-mined adjacent cells, and repeat this cascade through every newly revealed zero-count cell until no further cell in the cascade has an unopened, non-mined neighbor.
- **FR-008**: When a player opens a cell containing a mine (directly or via chording per FR-012), the system MUST immediately end the game as a loss and MUST prevent any further open, flag, or chord action on the board.
- **FR-009**: System MUST let a player flag or unflag an unopened cell via a long-press on that cell.
- **FR-010**: System MUST provide an explicit Flag Mode control the player can toggle on or off; while Flag Mode is on, a normal tap on an unopened cell flags or unflags it instead of opening it, and tapping an already-opened numbered cell still triggers chording (FR-012) when applicable. Long-press (FR-009) MUST continue to flag/unflag an unopened cell identically regardless of whether Flag Mode is on or off.
- **FR-011**: An opened cell MUST NOT be flaggable, and a flagged cell MUST NOT be opened by a normal open action until it is first unflagged.
- **FR-012**: For an opened numbered cell whose adjacent flagged-cell count equals its own number, the system MUST support a chord action — triggered by tapping that already-opened cell — that opens every remaining unopened, unflagged adjacent cell at once.
- **FR-013**: If an adjacent flagged cell's count does not equal an opened numbered cell's own number, tapping that cell MUST NOT open any adjacent cells and MUST leave board state unchanged.
- **FR-014**: A flag or unflag action MUST NOT by itself end the game or otherwise change whether the game is won or lost.
- **FR-015**: System MUST continuously track and display the remaining mine count, computed as total mines minus the current number of flagged cells, including displaying a negative value when flags exceed the mine total.
- **FR-016**: System MUST track elapsed time as accumulated active-play time only, starting from the first cell-open action and pausing whenever the app is closed or backgrounded (i.e., time away from the app does not count), updating the on-screen display no more often than once per minute during play, while still capturing and displaying the precise accumulated elapsed time once the game ends.
- **FR-017**: System MUST declare a win the instant every non-mine cell has been opened, regardless of whether any mines are flagged, and MUST end the game at that instant.
- **FR-018**: On game end (win or loss), system MUST stop accepting open, flag, and chord actions on the board and MUST present the outcome, the difficulty/board configuration played, and the elapsed time.
- **FR-019**: System MUST provide a settings area with a display color-mode control letting the player choose between "Color" and "Black-and-white."
- **FR-020**: In black-and-white mode, every distinct piece of board information — unopened, opened, and flagged cell states, and each adjacency number — MUST remain distinguishable using shape, symbol, or contrast alone, without relying on color.
- **FR-021**: System MUST persist the player's chosen color mode so it is automatically restored the next time the app is launched, and MUST keep the color-mode control visible and usable regardless of the underlying device's color or monochrome hardware.
- **FR-022**: System MUST persist in-progress game state — board configuration, mine layout once placed, every cell's state, and elapsed time — after every mutating action, so that closing and relaunching the app resumes the same in-progress game.
- **FR-023**: If persisted in-progress game state is missing, unreadable, or invalid, the system MUST discard it and return to new-game/difficulty selection without crashing, and MUST leave the persisted color-mode setting unaffected.
- **FR-024**: When a player initiates starting a new game (a preset or a custom configuration) while a different game is already in progress, the system MUST require explicit player confirmation before discarding the in-progress game; if the player declines, the in-progress game MUST remain unchanged and no new game MUST start.

### Key Entities

- **Game Board**: A rectangular grid of Cells defined by a width, height, and mine count; tracks overall status (not started, in progress, won, or lost) and the remaining-mine and elapsed-time values shown to the player.
- **Cell**: A single grid position with a state (unopened, opened, or flagged), whether it contains a mine, and — once opened — its adjacent-mine count.
- **Difficulty Configuration**: Either one of the three named presets (Beginner, Intermediate, Expert) or a custom width/height/mine-count triple, used to start a new Game Board.
- **Game Session**: One played-through attempt, combining a Game Board, its Difficulty Configuration, and elapsed time; this is the unit that is persisted and resumed across app restarts.
- **Settings**: Player-level preferences independent of any single game — currently the color-mode choice (Color or Black-and-white) — persisted across both games and app restarts.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A first-time player can start and finish (win or lose) a Beginner game using only in-app controls, with no outside instructions, in under 5 minutes.
- **SC-002**: Across all difficulty presets and every valid custom configuration, 0% of games end on the very first cell a player opens.
- **SC-003**: In black-and-white mode, every board state (unopened, opened, flagged) and every adjacency number (1–8) is correctly identified using shape/contrast cues alone, with no reliance on color.
- **SC-004**: A player who closes the app mid-game and reopens it finds the board exactly as left — same opened and flagged cells, same elapsed time — in 100% of cases.
- **SC-005**: A player who changes the color mode and restarts the app sees that same mode restored in 100% of cases.
- **SC-006**: Chording a correctly-flagged numbered cell opens all N of its remaining safe, unopened neighbors in a single action — 1 tap instead of N separate taps — for every value of N from 1 to 7 (the achievable range for a standard interior cell: chording requires at least 1 adjacent mine, capping the safe-neighbor count at 7 of a cell's 8 neighbors).

## Assumptions

- The primary input is touch (tap and long-press), consistent with the Kobo touchscreen platform; there is no mouse right-click or simultaneous two-button chord input, so flagging and chording use the touch-appropriate gestures defined in FR-009/FR-010/FR-012.
- Custom board width and height are each bounded to 5–16 cells, and mine count to 1..(width×height−9); this "small-screen" bound is a deliberate choice for this device class and does not apply to the three fixed named presets (including 30×16 Expert).
- The color-mode control is always shown and usable regardless of a given device's actual color/monochrome hardware capability; whether the physical display can render color is a platform/rendering concern outside this feature's scope.
- The Flag Mode toggle's on/off state is a per-session UI convenience and does not need to persist across app restarts — only the color-mode preference and in-progress game state persist (FR-021, FR-022).
- Leaderboards/high scores, undo/redo, hint systems, and multiplayer are explicitly out of scope for this feature, per the feature description.
