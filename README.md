# Minesweeper Template

A CMake/C++17 template for native Kobo e-reader programs and games: an
FBInk on-device rendering backend, an SDL2 desktop simulator, NickelMenu +
KFMon packaging, GitHub Actions cross-build/release CI, and a working
tap-counter demo proving the whole pipeline end to end.

This is a **template**, not a finished app — see [SETUP.md](SETUP.md) to
start a new project from it.

## What's included

- 4-layer architecture: `core` (pure logic) / `persist` (atomic JSON
  save/load) / `platform` (Renderer + TouchInput abstraction, `kobo/` and
  `sdl/` backends) / `ui` (renderer-agnostic screens and widgets)
- A minimal working placeholder demo (tap-to-increment counter with
  autosave and a second screen) exercising every layer
- Desktop simulator (SDL2) and Kobo device (FBInk, cross-compiled) build
  flavors from one CMake project
- NickelMenu + KFMon device packaging (`tools/package.sh`)
- CI: host tests → Kobo cross-build → tag-triggered GitHub release
- Spec-driven-development tooling (spec-kit) under `.specify/` and
  `.claude/skills/`
- `tools/rename-project.sh` to rebrand a fresh copy into a new project

## Documentation

| Document | Contents |
|---|---|
| [SETUP.md](SETUP.md) | Start a new project from this template |
| [Installation](docs/installation.md) | Install, launchers (NickelMenu / KFMon), uninstall |
| [Settings](docs/settings.md) | Launcher options, touch calibration, device files |
| [Building](docs/building.md) | Host tests, desktop simulator, Kobo cross-build, CI and releases |
| [Platform abstraction contract](docs/contracts/platform-abstraction.md) | The Renderer/TouchInput interfaces `ui` and backends must honor |
| [Install layout contract](docs/contracts/install-layout.md) | Device package shape and launcher integration |

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
