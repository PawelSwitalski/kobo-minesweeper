# Installation

The app ships as a single zip you extract onto the e-reader over USB. There
are two ways to launch it:

| Launcher | Notes |
|---|---|
| **NickelMenu** (recommended) — a **Minesweeper** entry in the **More** menu | Firmware 4.x only |
| **KFMon** — tap a disguised book cover in your library | Works on firmware 4.x and 5.x |

Both launchers start the same binary and share the same saved progress; you
can install either one, or both.

## 1. Prerequisite: NickelMenu (one-time per firmware update)

NickelMenu lets Nickel (the Kobo home software) show custom entries in the
**More** menu. It survives until a Kobo firmware update, after which you just
redo this step.

1. Download the latest `KoboRoot.tgz` from the
   [NickelMenu project](https://pgaskin.github.io/NickelMenu/) (firmware 4.x).
2. Connect the Kobo over USB, copy `KoboRoot.tgz` into the hidden `.kobo`
   folder on the device, and eject safely. The Kobo reboots and installs it.

## 2. Install the app

1. Build `dist/minesweeper.zip` (see [building.md](building.md)), or download
   it from a release if this project publishes one.
2. Connect the Kobo over USB (it shows up as a drive, e.g. `D:`).
3. Extract the zip onto the drive **root**, merging the `.adds` folder with
   any existing one.
4. Eject safely and let the Kobo finish importing.
5. Open **More** (bottom navigation) — a **Minesweeper** entry appears
   alongside Settings and your other items. Tap it to start the app.

If taps land in the wrong place on your model, see
[Touch calibration](settings.md#touch-calibration).

## Alternative launcher: KFMon cover-tap

[KFMon](https://github.com/NiLuJe/kfmon) launches programs when you tap a
specific "book" cover in your library. The required config
(`.adds/kfmon/config/minesweeper.ini`) and cover image (`kfmon-minesweeper.png`)
already ship in `minesweeper.zip` — only the KFMon prerequisite is extra:

1. Download the latest `KoboRoot.tgz` from
   [KFMon releases](https://github.com/NiLuJe/kfmon/releases) (the
   "uninstaller-less" package is fine), copy it into `.kobo` on the device,
   and eject. The Kobo reboots and installs it.
2. After installing the app, a book called **kfmon-minesweeper** appears in
   your library. Tap its cover to start the app.

## Uninstall

Delete these from the device over USB:

- `.adds/minesweeper/` (the app, its settings and saved progress)
- `.adds/nm/minesweeper` (the NickelMenu entry)
- `.adds/kfmon/config/minesweeper.ini` and `kfmon-minesweeper.png` (the KFMon launcher)

NickelMenu/KFMon themselves are separate installs; removing them is described
by their own projects.

---

**Verification note:** these steps follow the standard Kobo homebrew
NickelMenu/KFMon pattern (inherited from a sibling project, kobo-sudoku,
that tested the NickelMenu path on real hardware), but this game itself has
not yet been verified on a physical device — confirm both launchers on your
target model before relying on it.
