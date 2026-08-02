# Contract: Mine-count digit → color mapping

Governs `BoardScreen::drawCell()` (chooses the accent/shade per digit) and
`SoftCanvas::resolveColor()` (resolves an accent/shade to actual RGB on color-capable displays).
Applies only when `app_.theme().color` is true (Color mode selected *and* the device supports
color — unchanged composition rule from `001` research.md #7). When false, this table is entirely
bypassed: every digit renders `accent = Color::None`, `shade = Gray::Black`, exactly as `001`
already does.

## Mapping (FR-010, FR-011, FR-012)

| Mine count | `TextStyle::accent` | `TextStyle::shade` | Resolved RGB (Color mode, color-capable device) |
|---|---|---|---|
| 1 | `Color::Blue` | `Gray::Black` (unused when accent set) | `#1030C0` |
| 2 | `Color::Green` | `Gray::Black` | `#127A12` |
| 3 | `Color::Red` | `Gray::Black` | `#B42020` (unchanged from `001`) |
| 4 | `Color::Navy` | `Gray::Black` | `#0A1868` |
| 5 | `Color::Crimson` | `Gray::Black` | `#8B0A2A` |
| 6 | `Color::Cyan` | `Gray::Black` | `#0A7A82` |
| 7 | `Color::None` | `Gray::Black` | n/a — renders as plain black, unchanged from `001` |
| 8 | `Color::None` | `Gray::Mid` | n/a — renders as plain mid-gray via the existing grayscale shade axis |

Exact hex values are a starting point (research.md #5); implementation may retune them for
on-device legibility as long as SC-006 (all 8 mutually distinguishable) and FR-012 (glyph is the
primary signal, not color) hold.

## Invariants

- **FR-011 (grayscale unaffected)**: `resolveColor()`'s `colorDisplay == false` branch must remain
  exactly as it is today (`r = g = b = shade`, no accent applied) — this table only ever executes
  inside the `colorDisplay == true` branch.
- **FR-012 (glyph is the signal)**: the printed character (`"1"`..`"8"`) is set identically
  regardless of color mode or accent — this contract only ever changes accent/shade, never the
  glyph text itself.
- **No cross-talk with `Color::Red`'s other use**: `CanvasRenderer::fillRect`'s existing
  `accent == Color::Red` special case (a pink-tinted fill, `canvas_renderer.h:13-14`) is unrelated
  to this contract (no call site in this feature passes a non-`None` accent to `fillRect`, only to
  `drawText` via `TextStyle`) and is left untouched.
