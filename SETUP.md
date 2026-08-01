# Setup

## What this is

A CMake/C++17 template for Kobo e-reader apps with an FBInk device
rendering backend, an SDL2 desktop simulator, NickelMenu/KFMon packaging,
and a working tap-counter demo proving the architecture builds, runs,
tests, and packages end to end.

## Start a new project

```bash
cp -r kobo-games-template my-new-game && cd my-new-game
tools/rename-project.sh my_new_game --title "My New Game"
git init && git add -A && git commit -m "Initial commit from kobo-games-template"
```

`tools/rename-project.sh` rebrands the CMake project/target names, C++
namespace, device install paths, env var prefixes, and doc/README text from
the `minesweeper` placeholder to your chosen name. Run `--dry-run` first if you
want to see what it would touch. See the script's header comment for full
details.

## Verify it builds

```bash
# host unit tests
cmake -B build/host -DMY_NEW_GAME_BACKEND=none -DBUILD_TESTS=ON
cmake --build build/host --config Release
ctest --test-dir build/host -C Release --output-on-failure

# desktop simulator
cmake -B build/sim -DMY_NEW_GAME_BACKEND=sdl
cmake --build build/sim --config Release
build/sim/Release/my_new_game --width 1264 --height 1680 --dpi 300
```

(Replace `MY_NEW_GAME`/`my_new_game` with whatever `tools/rename-project.sh`
derived from the name you picked — it prints the exact commands to use at
the end of its run.)

Tap the button: the count should increment, autosave to
`my_new_game-data/counter.json`, and persist across relaunches. Tap "About"
to check screen navigation (push/pop).

## Where to add your own logic

- Domain logic goes in `src/core/` — delete `counter.h`/`counter.cpp` once
  you have real state to replace it with.
- Delete `src/ui/screens/counter_screen.*` and `about_screen.*`, add your
  own screens implementing `src/ui/screens/screen.h`.
- Extend `src/ui/app.h`'s `App` interface with your own state
  accessors/persist hooks (following the shape `counter()`/`autosave()`
  already show), and update `AppImpl` in `src/main.cpp` to match.
- Persisted state goes through `src/persist/` (`paths.h` resolves where to
  store files; `store.h` does atomic load/save).
- Reusable UI pieces (`Button`, `Label`, `Dialog`) live in `src/ui/widgets.h`.

## Spec-driven workflow

This template carries over the [spec-kit](https://github.com/github/spec-kit)
SDD tooling used to build the project it was extracted from:
`.specify/memory/constitution.md` plus the `.claude/skills/speckit-*`
Claude Code skills (`/speckit-specify`, `/speckit-plan`, `/speckit-tasks`,
`/speckit-implement`, etc.).

Before running `/speckit-specify` for your first real feature, review and
adjust `.specify/memory/constitution.md` — its six principles are already
generic to "a Kobo device app," but you may want to tighten Principle III's
language for your actual domain.

## Device packaging / cross-build

The FBInk cross-compile (`tools/build-fbink.sh` +
`-DMINESWEEPER_BACKEND=fbink` + `cmake/kobo-toolchain.cmake`, or the renamed
equivalent after `rename-project.sh`) requires Linux/WSL2 and the
[koxtoolchain](https://github.com/koreader/koxtoolchain) ARM cross-compiler
— it cannot be built or verified on a Windows-only machine. See
[docs/building.md](docs/building.md) for the full steps, or push to GitHub
and let `.github/workflows/build.yml`'s `kobo-cross-build` job do it in CI.
