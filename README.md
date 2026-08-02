# Kobo Minesweeper

A native, fully offline Minesweeper game for Kobo e-readers.

Built for the Kobo Libra Colour and portable across the Kobo lineup — one
ARM binary, with screen geometry computed at runtime. Classic Beginner /
Intermediate / Expert presets plus custom board sizes, first-click safety,
cascading reveal, flagging, and chording. No network, no accounts, no ads.

> **Status:** implemented and verified via the full host unit test suite
> and the SDL desktop simulator; not yet tested on real Kobo hardware.

## Features

- Beginner (9×9, 10 mines), Intermediate (16×16, 40 mines), and Expert
  (30×16, 99 mines) presets, plus a custom board (5–16 per side, 1 to
  width×height−9 mines)
- First-click safety — the first cell you open is never a mine
- Cascading flood-fill reveal on blank cells
- Flag suspected mines with a long-press, or toggle an explicit Flag Mode
  for one-tap flagging
- Chording: tap a satisfied numbered cell to open all its remaining safe
  neighbors at once
- Color / Black-and-white display modes — every cell state and number
  stays distinguishable by shape and contrast alone, so black-and-white
  play never loses information
- Auto-save after every move; resume any in-progress game, including
  across app restarts
- E-ink friendly: minute-granularity timer, partial refreshes for moves,
  no per-second redraws

## Quick start

1. Install [NickelMenu](https://pgaskin.github.io/NickelMenu/) (one-time,
   per firmware update).
2. Download `minesweeper.zip` from the
   [latest release](https://github.com/PawelSwitalski/kobo-minesweeper/releases/latest)
   and extract it onto the Kobo's USB drive root.
3. Eject, open **More** on the device, tap **Minesweeper**.

Full steps, the KFMon alternative and uninstall: [docs/installation.md](docs/installation.md).

## Documentation

| Document | Contents |
|---|---|
| [Installation](docs/installation.md) | Install, launchers (NickelMenu / KFMon), uninstall |
| [Settings](docs/settings.md) | In-game color-mode settings, launcher options, touch calibration, device files |
| [Building](docs/building.md) | Host tests, desktop simulator, Kobo cross-build, CI and releases |

## Building in short

```bash
# unit tests + desktop simulator (Windows/Linux/macOS, CMake + C++17)
cmake -B build/sim -DMINESWEEPER_BACKEND=sdl && cmake --build build/sim --config Release

# device binary (Linux/WSL2 + koxtoolchain)
tools/build-fbink.sh
cmake -B build/kobo -DCMAKE_TOOLCHAIN_FILE=cmake/kobo-toolchain.cmake -DMINESWEEPER_BACKEND=fbink
cmake --build build/kobo && tools/package.sh build/kobo
```

Details in [docs/building.md](docs/building.md).

## License

[MIT](LICENSE). Vendored third-party components keep their own licenses:
FBInk, nlohmann/json, doctest, stb_truetype, SDL2 (simulator only) and the
DejaVu Sans fonts (`dist/.adds/minesweeper/assets/FONT-LICENSE.txt`).
