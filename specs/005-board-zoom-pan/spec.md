# Feature Specification: Board Zoom & Pan

**Feature Branch**: `005-board-zoom-pan`

**Created**: 2026-08-02

**Status**: Draft

**Input**: User description: "User should be zoom in and zoom out the game is that possible"

**Correction (2026-08-03)**: The player must control zoom and pan using touch gestures only —
two-finger pinch to zoom, one-finger drag to pan — the same interaction pattern used to zoom and
pan a map. No on-screen zoom/pan buttons are used.

## Clarifications

### Session 2026-08-02

- Q: When zoomed in and a cascade (opening a blank cell) reveals cells currently off-screen, what should happen to the view? → A: The view automatically recenters/pans to keep the newly revealed cascade visible.

### Session 2026-08-03

- Q: Given the app's e-ink-friendly design (infrequent, deliberate full-screen redraws — no
  per-second updates), how should the view respond while a pinch or drag gesture is still in
  progress (finger(s) still down)? → A: The view does not live-track the finger. It redraws only
  in discrete steps, each time the gesture crosses a defined zoom-level or pan-distance threshold —
  never on every raw touch-movement sample, and never with a smooth/animated transition.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Zoom in to see and tap cells more clearly (Priority: P1)

A player on a larger board (Intermediate, Expert, or a large Custom board) finds the cells too
small to comfortably read the numbers or tap accurately, since the board is automatically shrunk
to fit the whole grid on screen at once. They place two fingers on the board and spread them apart
(pinch-out), the same gesture used to zoom in on a map, to make the cells larger and easier to see
and tap, at the cost of no longer seeing the whole board at once.

**Why this priority**: This is the core problem the request is about, and it addresses a real,
existing usability gap — on the largest boards, cells already render smaller than the app's own
comfortable-tap-target guidance. It delivers value on its own: a player can already see the benefit
(bigger, clearer cells) without needing to zoom back out or pan.

**Independent Test**: Start an Expert-difficulty game, perform a pinch-out gesture one or more
times, and confirm cells render larger and remain individually tappable with the correct cell
responding to each tap.

**Acceptance Scenarios**:

1. **Given** a board is displayed at its default (fit-to-screen) size, **When** the player
   pinch-out gestures on the board, **Then** the cells render larger than before.
2. **Given** a zoomed-in board, **When** the player taps a cell that is currently visible, **Then**
   that exact cell opens/flags/chords, exactly as it would at the default zoom level.
3. **Given** the player keeps pinching out, **When** the maximum zoom level is reached, **Then**
   further pinch-out gestures have no additional effect (the view stops zooming further rather than
   producing an error).
4. **Given** a zoomed-in board, **When** the player opens a cell whose cascade (flood-fill reveal)
   extends beyond the currently visible area, **Then** the view automatically recenters/pans so the
   newly revealed cells are visible.

---

### User Story 2 - Zoom back out to see the whole board again (Priority: P2)

After zooming in, a player wants to see the full board again — to get an overview, check overall
progress, or simply return to the familiar default view. They bring two fingers together on the
board (pinch-in), possibly repeating the gesture, until the board returns to showing the entire
grid at once.

**Why this priority**: Depends on User Story 1 existing (there must be a zoomed-in state to zoom
out of), but is essential for the feature to feel complete and non-disorienting — a player who can
only zoom in and never back out would consider the feature broken.

**Independent Test**: From a zoomed-in board, pinch-in repeatedly and confirm the board returns to
the default fit-to-screen view showing every cell at once, with no further shrinking possible.

**Acceptance Scenarios**:

1. **Given** a zoomed-in board, **When** the player pinch-in gestures enough, **Then** the board
   returns to the default view where the entire grid is visible on screen.
2. **Given** the board is already at the default fit-to-screen view, **When** the player pinch-in
   gestures further, **Then** nothing further shrinks (the view never zooms out past showing the
   whole board).

---

### User Story 3 - Pan around a zoomed-in board to reach every cell (Priority: P2)

Once zoomed in far enough that the whole board no longer fits on screen, the player needs a way to
shift the visible portion of the board to reach cells that are currently off-screen. They drag one
finger across the board — the same gesture used to pan a map — to move the view up, down, left, or
right.

**Why this priority**: Without this, zooming in on a board larger than one screenful would strand
the player unable to reach part of the grid — this story is what makes User Story 1 usable on
boards where the zoomed board doesn't fit the screen, so it ships alongside it rather than being
deferred.

**Independent Test**: Zoom in on an Expert board until part of the grid is off-screen, then drag
across the board to bring every corner into view and successfully interact with cells there.

**Acceptance Scenarios**:

1. **Given** a zoomed-in view where part of the board is off-screen, **When** the player drags one
   finger across the board, **Then** the visible portion shifts in the dragged direction to reveal
   the previously off-screen cells.
2. **Given** the visible portion is already at an edge of the board, **When** the player drags
   further toward that same edge, **Then** the view does not move past the edge (no empty space
   beyond the grid is ever shown).
3. **Given** the entire board already fits on screen (default zoom or zoomed in without exceeding
   the screen), **When** the player drags a finger across the board, **Then** the drag has no
   effect, since there is nothing to pan to.

---

### Edge Cases

- Zooming and panning never change game state: the mine layout, opened/flagged cells, and elapsed
  time are identical before and after any zoom or pan action.
- Starting a new game (any difficulty) resets the view to the default fit-to-screen zoom level,
  rather than carrying over a zoom/pan state that might not make sense for a different board size.
- On a board small enough that even the maximum zoom level still fits the whole grid on screen
  (e.g., Beginner), zooming in still works, but drag-to-pan is never needed since nothing is
  off-screen.
- When the game ends (win or loss) while zoomed/panned, the outcome banner is still fully visible
  and usable regardless of the current zoom/pan state.
- When opening a cell while zoomed in triggers a cascade (flood-fill reveal of a connected blank
  region) that extends beyond the currently visible area, the view automatically recenters/pans to
  keep the newly revealed cells visible, rather than leaving the player to discover them manually.
- A single-finger touch that moves more than a small distance before lifting is treated as the
  start of a pan drag, not a tap: it MUST NOT open, flag, or chord the cell under the finger,
  matching how a drag on a map does not activate whatever is underneath it.
- A long-press-to-flag gesture is a single finger held stationary; because it doesn't move beyond
  the drag threshold, it is unaffected by pan-drag detection and continues to flag as before.
- If a third finger touches the board during an in-progress two-finger pinch, it is ignored — only
  the first two contact points are tracked for the pinch.
- Because the view updates in discrete steps rather than tracking fingers live (see Clarification,
  2026-08-03), a player will see the board "jump" directly from one zoom/pan step to the next
  rather than smoothly resizing or sliding while their fingers are still moving.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The board screen MUST let the player zoom in — increasing cell size beyond the
  automatic fit-to-screen size — by performing a two-finger pinch-out (spread) gesture on the
  board, the same gesture used to zoom in on a map.
- **FR-002**: The board screen MUST let the player zoom out — decreasing cell size back down to,
  but never below, the automatic fit-to-screen size that shows the entire board — by performing a
  two-finger pinch-in gesture on the board.
- **FR-003**: Zooming in or out MUST NOT change any game state — mine layout, opened/flagged
  cells, and elapsed time remain exactly as they were.
- **FR-004**: Whenever the zoomed board is larger than the visible screen area, the system MUST
  let the player pan (shift) the visible area by dragging one finger across the board, so every
  cell on the board can be reached.
- **FR-005**: Panning MUST NOT move the visible area beyond the edges of the board.
- **FR-006**: Tapping a visible cell MUST always act on the correct cell, regardless of the current
  zoom level or pan position.
- **FR-006a**: When opening a cell while zoomed in causes a cascade (flood-fill reveal of a
  connected blank region) that extends beyond the currently visible area, the view MUST
  automatically recenter/pan so the newly revealed cells are visible, without requiring the player
  to pan manually.
- **FR-007**: Zoom and pan MUST be operable via multi-touch gestures only — two-finger pinch to
  zoom, one-finger drag to pan — mirroring the standard map-zoom/pan interaction. The board screen
  MUST NOT show on-screen zoom or pan buttons.
- **FR-007a**: The system MUST distinguish a tap (to open/flag/chord a cell) from the start of a
  pan drag: a single-finger touch that moves beyond a small movement threshold before lifting MUST
  be treated as a pan and MUST NOT open, flag, or chord any cell.
- **FR-008**: The zoom level MUST reset to the default fit-to-screen view whenever a new game is
  started.
- **FR-009**: The drag-to-pan gesture MUST have no effect when the entire board already fits on
  the screen, since there is nothing to pan to.
- **FR-010**: Opening, flagging, and chording cells MUST work identically at any zoom level and pan
  position, exactly as they do at the default view.
- **FR-011**: The win/loss outcome banner MUST remain fully visible and usable regardless of the
  zoom level or pan position in effect when the game ends.
- **FR-012**: While a pinch or drag gesture is still in progress (fingers still touching the
  screen), the system MUST NOT redraw the screen on every raw touch-movement sample. The view MUST
  update only in discrete steps, each time the gesture crosses a defined zoom-level or
  pan-distance threshold, consistent with the app's e-ink-friendly design of infrequent, deliberate
  full-screen redraws rather than continuous/animated updates.

### Key Entities

- **Board View**: The player's current zoom level and pan position for the board currently being
  played. Not part of the saved game itself — it is transient display state that resets to the
  default (fit-to-screen, no pan offset) whenever a new game starts.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: On the largest supported board, a player can zoom in far enough that every cell
  renders at or above the app's existing comfortable minimum tap-target size.
- **SC-002**: On the largest supported board, a player can zoom in, pan to reach any cell, and
  successfully open or flag it — for every cell on the board, not just the ones initially visible.
- **SC-003**: From any zoomed-in and/or panned state, a player can return to the full default view
  with a single continued pinch-in gesture (no need to lift and re-pinch multiple times).
- **SC-004**: Across 100% of test sessions that zoom and/or pan without opening or flagging a cell,
  the game's mine count, flagged cells, opened cells, and elapsed time are unchanged.

## Assumptions

- Zoom is invoked with a two-finger pinch (spread to zoom in, pinch together to zoom out); pan is
  invoked with a one-finger drag. There are no on-screen zoom or pan buttons anywhere on the board
  screen.
- **Significant technical gap, called out explicitly**: today the app's touch input layer (used by
  both the Kobo on-device backend and the desktop SDL simulator backend) only ever reports a single
  discrete tap or long-press per touch — it has no notion of two simultaneous touch points or of
  tracking motion during a touch. Delivering pinch-to-zoom and drag-to-pan requires adding real
  multi-touch tracking (recognizing two simultaneous contacts and the changing distance between
  them) and single-finger motion tracking (distinguishing a drag from a stationary tap or
  long-press) to the input layer on both backends. This is a materially larger implementation than
  a button-based zoom/pan control would have been, and downstream planning should scope it as such.
- Given the app's e-ink-friendly design principle (minute-granularity timer, partial refreshes,
  no per-second redraws), gesture-driven view changes are rendered as discrete steps, not
  continuous, frame-by-frame tracking of the finger(s): the screen redraws only when the pinch
  distance crosses a zoom-level threshold, or the drag crosses a pan-distance threshold — never on
  every raw touch-movement sample, and never with a smooth/animated transition. This was confirmed
  in Clarifications (Session 2026-08-03).
- Each zoom or pan step is a discrete, one-shot view update (similar to switching screens), not a
  smooth or animated transition — appropriate for the display technology this app targets.
- Zoom/pan state is not saved as a player preference and does not persist between games; it always
  starts at the default fit-to-screen view when a game begins.
- Zoom is available uniformly on every difficulty and board size, not only on boards where cells
  already render below the comfortable tap-target size — smaller boards benefit too (e.g., a player
  who wants larger, easier-to-read digits).
- "Comfortable minimum tap-target size" refers to the same touch-target guidance the app already
  follows for its other on-screen controls.
- A pinch gesture is defined by exactly two simultaneous touch points; if a third finger touches
  the board while a pinch is in progress, it is ignored rather than restarting or disrupting the
  gesture.
