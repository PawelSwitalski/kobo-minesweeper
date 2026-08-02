# Phase 0 Research: Fix Game Timer & Add Hide-Timer Setting

No items in Technical Context were left as `NEEDS CLARIFICATION` — this feature reuses the
existing `001`/`002` codebase's language, dependencies, testing, and target platform unchanged.
The research below is grounded in direct inspection of `src/main.cpp`, `src/core/game_session.h/.cpp`,
`src/core/settings.h/.cpp`, `src/ui/screens/board_screen.h/.cpp`, `src/ui/screens/settings_screen.h/.cpp`,
and `CMakeLists.txt`.

## 1. Root cause of the timer undercount, and the fix

**Root cause**: `main.cpp`'s event loop (`main.cpp:220-277`) maintains one tick reference,
`lastTickSteady`, updated unconditionally at the very bottom of every iteration
(`main.cpp:270`, `lastTickSteady = nowSteady;`). But the code that actually accumulates active
play time — `screen->onTick(activeSeconds)`, which calls `GameSession::addActiveSeconds` — is only
reached inside the `else` branch, i.e. **only on loop iterations where `waitForTap` returned no
tap** (`main.cpp:239-253`). On an iteration where a tap *did* occur, the loop calls `screen->onTap(*tap)`
instead and then falls through to unconditionally reset `lastTickSteady = nowSteady` — silently
discarding the real wall-clock time that elapsed since the previous iteration, because that
interval is never handed to `onTick`. During active play the loop wakes on essentially every tap
(opening cells, flagging, chording), so most of the real elapsed time during exactly the period the
player is most actively playing never reaches `elapsedSeconds_` at all. The timer only advances
correctly during stretches where the player pauses long enough for the 20-second idle timeout
(`kTimeoutMs`, `main.cpp:199`) to fire the no-tap branch.

**Decision**: Call `screen->onTick(activeSeconds)` on **every** loop iteration — tap or not — using
a tick reference that always advances by exactly the interval since the previous iteration,
regardless of what that iteration did. Concretely: compute `activeSeconds` and call `onTick`
immediately after the existing `slept`/`sdlRedraw` repaint check, before branching on `tap`;
`screen->onTap(*tap)` remains a separate call gated by `if (tap)`, unchanged in its own effect.

**Rationale**: This directly closes the gap — no wall-clock interval between two loop iterations is
ever excluded from consideration again. Whether that interval counts toward `elapsedSeconds_`
still depends only on `Screen::countsPlayTime()` (unchanged contract: only the in-progress board
screen counts) and the existing `slept` flag (unchanged: device-sleep gaps are excluded). `BoardScreen::onTick`
already calls `app_.autosaveSession()` on every invocation; now doing so on tap iterations too is an
accepted, harmless side effect (see plan.md Technical Context "Constraints" — an atomic
write-then-rename is idempotent, so an extra call in the same iteration as `afterMutation()`'s own
autosave just costs one redundant write).

**Alternatives considered**:
- *Track `lastTickSteady` only in the no-tap branch, and add the "missing" interval retroactively
  the next time the no-tap branch runs*: rejected — this still requires threading a second
  accumulator through the loop and produces the same result as simply calling `onTick` every
  iteration, but with more state to keep in sync (Constitution VI).
- *Make `onTap` itself respect elapsed time by passing a timestamp into it*: rejected — conflates
  two independent concerns (tap handling vs. time bookkeeping) in one call signature, and
  `Screen::onTap`'s signature is shared by every screen, most of which don't care about timing at
  all.

## 2. Where the fix lives: a new `core::ActiveTimeTracker`, not inline `main.cpp` arithmetic

**Decision**: Extract the tick-timing computation into a new, small, portable type,
`core::ActiveTimeTracker` (`src/core/active_time_tracker.h/.cpp`), added as a source file in the
existing `minesweeper_core` static library target. Its only method, `tick(now, counts)`, takes an
explicit `std::chrono::steady_clock::time_point` and a `bool` (whether this interval should count),
returns the whole seconds to attribute, and unconditionally advances its own internal reference
point to `now`. `main.cpp` becomes a thin caller: it still owns the only `steady_clock::now()` call
and the `slept`/`countsPlayTime()` inputs, but the actual "how many seconds, and did I remember to
move the reference point" logic — the exact logic that was buggy — now lives in one host-testable
place.

**Rationale**: `CMakeLists.txt:99-111` links `minesweeper_tests` against only `minesweeper_core`
and `minesweeper_persist` — `main.cpp` itself is compiled solely into the final `minesweeper`
executable target and is never part of any test binary. Leaving the fix as inline arithmetic inside
`main.cpp`'s `while` loop, as it is today, would leave the corrected code exactly as untestable as
the buggy code was — which is arguably *why* this bug shipped in the first place (Constitution III
is non-negotiable for correctness-critical logic, and this is about as correctness-critical as a
persisted, player-facing timer gets). Placing the extracted type in `src/core/` — even though it is
about input-loop timing rather than gameplay rules — is the only way to get it linked into
`minesweeper_tests` without inventing a new library target, and it satisfies Principle I's "no OS
calls" requirement exactly as written: the class receives timestamps as parameters and never calls
`steady_clock::now()` itself, so it remains pure arithmetic, just like `GameSession::addActiveSeconds(uint32_t)`
already is pure arithmetic despite being "about" a real-world quantity (elapsed seconds).

**Alternatives considered**:
- *Leave the fix inline in `main.cpp`, verify only via the manual quickstart scenario*: rejected —
  same category of gap that let the bug ship unnoticed; a manual scenario cannot exhaustively cover
  arbitrary tap-timing patterns the way a synthetic-timestamp unit test can.
- *Put `ActiveTimeTracker` under `src/ui/` (screens already deal with `onTick`)*: rejected —
  `minesweeper_ui` is also not linked into `minesweeper_tests` (`CMakeLists.txt:99-111`), so this
  would not solve the testability problem either.
- *Add a fourth CMake target (e.g. `minesweeper_platform_util`) just for this one class, linked into
  both `minesweeper` and `minesweeper_tests`*: rejected — a new build target for one class is more
  structure than the problem needs (Constitution VI); `src/core/` already exists, is already linked
  into tests, and already hosts small self-contained value-ish types (`DifficultyConfig`).

## 3. Hide-timer setting: one boolean field, one toggle button, two read sites

**Decision**: Add `bool hideTimer = false;` to `core::Settings` (mirrors `colorMode`'s existing
field-plus-JSON-round-trip pattern exactly). `SettingsScreen` gains one new `Button`,
`hideTimerButton_`, laid out as its own row below the existing Color/Black-and-white row and above
`backButton_`; unlike the two mutually-exclusive color buttons, this is a single toggle (same UX
shape as `BoardScreen`'s own `flagModeButton_`: `button.toggled` reflects the current boolean,
tapping it flips the boolean, calls `app_.autosaveSettings()`, then redraws). `BoardScreen::drawTimer()`
keeps its `r.fillRect(timerRect_, Gray::White)` unconditionally (so the HUD slot is always blanked,
never shows stale text) but skips constructing/drawing the `Label` when `app_.settings().hideTimer`
is true. `BoardScreen::onTick()` skips its minute-boundary `drawTimer()` + `flushPartial(timerRect_)`
call entirely when `hideTimer` is true (nothing to redraw, and skipping avoids a pointless e-ink
partial refresh — a small bonus alignment with Constitution II, not a requirement of the spec).

**Rationale**: `BoardScreen::drawOutcomeBanner()`'s own elapsed-time `Label` (`board_screen.cpp:160-164`)
is a completely separate code path from `drawTimer()`/`drawHud()` and never reads `hideTimer` —
so FR-008 ("final elapsed time still shown when hidden") requires *zero* new code; it is already
exactly what happens by construction, since only the live HUD timer's own draw path is touched.
FR-009 ("takes effect immediately, no new game needed") likewise requires no new plumbing: tapping
"Back" on `SettingsScreen` already calls `app_.pop()`, which sets `navDirty_`, which the existing
main-loop `if (app.consumeNavDirty())` block (`main.cpp:272-276`) already turns into a full
`top()->draw()` + `flushFull()` — so the board screen redraws itself (respecting the just-changed
`hideTimer`) through the exact same mechanism `colorMode` changes already rely on.

**Alternatives considered**:
- *Reuse the two-button mutually-exclusive pattern (`"Show"`/`"Hide"` buttons) instead of one
  toggle*: rejected — `hideTimer` is a plain boolean, and `flagModeButton_` already establishes the
  simpler single-toggle-button idiom for exactly this shape of setting in this codebase
  (Constitution VI: prefer the existing, simpler idiom over introducing a second pattern for the
  same kind of choice).
- *Reserve the timer's HUD space for something else when hidden (e.g. widen the mine-count label)*:
  rejected — not requested by the spec (FR-007 only requires the timer not be displayed; all other
  board information stays as-is), and would be an unrequested layout change (Constitution VI).

## 4. `settings.json` backward compatibility: additive field, no schema version bump

**Decision**: `Settings::fromJson` reads the new field as `j.value("hideTimer", false)` (falls back
to `false` if the key is absent) rather than `j.at("hideTimer")` (which would throw on any
already-existing `settings.json` written by `001`/`002`, since those predate this field).
`schemaVersion` stays `1`.

**Rationale**: `colorMode` uses `.at()` because it has existed since `schemaVersion: 1` was first
defined — every valid file is guaranteed to have it. `hideTimer` is being added *to* schema version
1 after the fact, so an existing installed `settings.json` on a player's device (written by an
already-shipped build) will not have the key; `.value(..., false)` treats that as "use the default,"
which is exactly SC-004/FR-006's intent (defaults to off) and avoids spuriously discarding an
existing player's `colorMode` choice just because the file predates this one new field. Bumping
`schemaVersion` to `2` would be the conventional move for a field with no reasonable default, but
`hideTimer` has an obvious, spec-mandated default (`false`, per FR-006/clarified as "off by
default"), so treating its absence as that default is more precise than a version bump that would
force every pre-existing install's settings file to be rejected and recreated from scratch on next
load — a strictly worse outcome for those players (they'd silently lose their saved `colorMode` too,
since the whole object is discarded on any `fromJson` failure per `persistence-schema.md`).

**Alternatives considered**:
- *Bump `schemaVersion` to `2`, require `hideTimer` via `.at()`*: rejected per the data-loss
  argument above.
- *Write a one-time migration that reads `schemaVersion: 1` files with `.value()` fallbacks but
  writes `schemaVersion: 2` afterward*: rejected as unnecessary ceremony for a single additive
  boolean with a spec-mandated default (Constitution VI) — nothing about the shape of the *other*
  fields changes, so there is nothing to "migrate."
