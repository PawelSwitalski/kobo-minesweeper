# Contract: Device Package & Launch Integration

Installation = unzip one archive to the Kobo's USB mass-storage root (e.g. `D:\`), eject, done. One-time prerequisite: NickelMenu and/or KFMon installed (see [installation.md](../installation.md)).

## Package layout (zip root = device root)

```text
.adds/
├── minesweeper/
│   ├── minesweeper               # armhf binary (koxtoolchain build)
│   ├── start.sh                # launch wrapper (see below)
│   ├── assets/                 # fonts (bundled TTF)
│   └── (counter.json created at runtime)
├── nm/
│   └── minesweeper                # NickelMenu entry (FW 4.x)
└── kfmon/
    └── config/
        └── minesweeper.ini         # KFMon watch config

kfmon-minesweeper.png               # "book cover" trigger image at device root
```

## KFMon config (`minesweeper.ini`)

```ini
[watch]
filename = /mnt/onboard/kfmon-minesweeper.png
action = /mnt/onboard/.adds/minesweeper/start.sh
```

Tapping the "Minesweeper" cover in the Kobo library launches the app. Works on firmware 4.x and 5.x (KFMon is inotify-based; NickelMenu is FW-4.x-only).

## `start.sh` obligations

- `cd` into `/mnt/onboard/.adds/minesweeper`, set `LD_LIBRARY_PATH` if any bundled `.so` (target: none — static/vendored).
- Run `./minesweeper`; on exit, control returns to Nickel.
- Log stderr to `.adds/minesweeper/crash.log` (truncate per run) for field debugging.

## NickelMenu entry (`.adds/nm/minesweeper`, FW 4.x only)

```text
menu_item : main : Minesweeper : cmd_spawn : quiet : exec /mnt/onboard/.adds/minesweeper/start.sh
```

## Compatibility contract

- Binary: ARMv7 hard-float, linked against koxtoolchain glibc floor → runs on every Nickel-era Kobo (Touch C and newer), including sunxi (Sage/Elipsa) and 2024 colour devices (Clara/Libra Colour) via FBInk's device abstraction.
- Layout: all geometry computed from `Renderer::info()` (width/height/dpi) — no hardcoded pixel positions.
- Uninstall: delete `.adds/minesweeper/`, `.adds/nm/minesweeper`, `.adds/kfmon/config/minesweeper.ini`, `kfmon-minesweeper.png`.
