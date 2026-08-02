# Changelog

## v1.0.0 — 2026-08-02

First release.

- Full Minesweeper game for Kobo e-readers: Beginner/Intermediate/Expert
  presets plus a custom board size (5–16 per side, 1..width×height−9
  mines), first-click safety, cascading flood-fill reveal, flagging
  (long-press or an explicit Flag Mode toggle), chording, win/loss
  detection, mine counter, and an elapsed-time HUD.
- Color / Black-and-white display setting, persisted across restarts —
  every cell state and adjacency number stays distinguishable by shape
  and contrast alone regardless of mode.
- Auto-save after every move via atomic writes; resumes any in-progress
  game across app restarts, and degrades to a fresh game on a corrupt
  save instead of crashing.
- E-ink aware rendering: partial refreshes per move/cascade, a
  minute-granularity timer (no per-second redraws), full refreshes on
  screen transitions and game end.
- Desktop SDL2 simulator sharing the exact device UI code, plus a host
  unit test suite covering game rules, first-click-safety across many
  seeds, and persistence round-trip/corrupted-input recovery.
- Not yet verified on real Kobo hardware — see [README](README.md) status
  note.
