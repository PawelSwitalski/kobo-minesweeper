cmake_minimum_required(VERSION 3.20)
project(minesweeper CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build flavors:
#   -DMINESWEEPER_BACKEND=none   host: core + tests only
#   -DMINESWEEPER_BACKEND=sdl    host: desktop simulator
#   -DMINESWEEPER_BACKEND=fbink  device: Kobo e-ink build (cross, cmake/kobo-toolchain.cmake)
set(MINESWEEPER_BACKEND "none" CACHE STRING "Rendering backend: none | sdl | fbink")
option(BUILD_TESTS "Build host unit tests" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Portable core: pure logic, no OS calls ---
add_library(minesweeper_core STATIC
  src/core/difficulty.cpp
  src/core/board.cpp
  src/core/game_session.cpp
  src/core/settings.cpp
  src/core/active_time_tracker.cpp
  src/core/board_viewport.cpp
  src/core/gesture_recognizer.cpp
)
target_include_directories(minesweeper_core PUBLIC src third_party)

# --- Persistence: atomic JSON files ---
add_library(minesweeper_persist STATIC
  src/persist/paths.cpp
  src/persist/store.cpp
)
target_link_libraries(minesweeper_persist PUBLIC minesweeper_core)

# --- UI: renderer-agnostic screens and widgets ---
add_library(minesweeper_ui STATIC
  src/ui/theme.cpp
  src/ui/widgets.cpp
  src/ui/screens/board_screen.cpp
  src/ui/screens/new_game_screen.cpp
  src/ui/screens/settings_screen.cpp
)
target_link_libraries(minesweeper_ui PUBLIC minesweeper_core)

if(MINESWEEPER_BACKEND STREQUAL "sdl")
  find_package(SDL2 CONFIG QUIET)
  if(NOT SDL2_FOUND AND MSVC)
    # Prebuilt VC development package (host simulator only).
    include(FetchContent)
    FetchContent_Declare(sdl2_prebuilt
      URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-devel-2.30.11-VC.zip
    )
    FetchContent_MakeAvailable(sdl2_prebuilt)
    set(SDL2_DIR "${sdl2_prebuilt_SOURCE_DIR}/cmake")
    find_package(SDL2 CONFIG REQUIRED)
  elseif(NOT SDL2_FOUND)
    find_package(SDL2 REQUIRED)
  endif()

  add_executable(minesweeper
    src/main.cpp
    src/platform/softcanvas.cpp
    src/platform/sdl/sdl_renderer.cpp
    src/platform/sdl/mouse_touch.cpp
  )
  target_compile_definitions(minesweeper PRIVATE MINESWEEPER_BACKEND_SDL SDL_MAIN_HANDLED)
  target_link_libraries(minesweeper PRIVATE minesweeper_ui minesweeper_persist SDL2::SDL2)
  if(TARGET SDL2::SDL2 AND MSVC)
    add_custom_command(TARGET minesweeper POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:minesweeper>)
  endif()
elseif(MINESWEEPER_BACKEND STREQUAL "fbink")
  # Cross build (cmake/kobo-toolchain.cmake). FBInk is built by
  # tools/build-fbink.sh into third_party/FBInk/Release/.
  set(FBINK_LIB "${CMAKE_SOURCE_DIR}/third_party/FBInk/Release/libfbink.a")
  if(NOT EXISTS "${FBINK_LIB}")
    message(FATAL_ERROR "FBInk not built. Run tools/build-fbink.sh first (see docs/building.md).")
  endif()

  # FBInk's KOBO build resolves I2C (e.g. query_accelerometer) against its own
  # vendored/staged libi2c.a rather than bundling those symbols into
  # libfbink.a itself, so consumers must link it too.
  set(FBINK_I2C_LIB "${CMAKE_SOURCE_DIR}/third_party/FBInk/libi2c-staged/lib/libi2c.a")
  if(NOT EXISTS "${FBINK_I2C_LIB}")
    message(FATAL_ERROR "FBInk's staged libi2c.a not found. Run tools/build-fbink.sh first (see docs/building.md).")
  endif()

  add_executable(minesweeper
    src/main.cpp
    src/platform/softcanvas.cpp
    src/platform/kobo/fbink_renderer.cpp
    src/platform/kobo/evdev_touch.cpp
  )
  target_compile_definitions(minesweeper PRIVATE MINESWEEPER_BACKEND_FBINK)
  target_include_directories(minesweeper PRIVATE third_party/FBInk)
  target_link_libraries(minesweeper PRIVATE minesweeper_ui minesweeper_persist "${FBINK_LIB}" "${FBINK_I2C_LIB}" m rt pthread)
endif()

if(BUILD_TESTS)
  enable_testing()
  add_executable(minesweeper_tests
    tests/main.cpp
    tests/test_smoke.cpp
    tests/test_persist.cpp
    tests/test_difficulty.cpp
    tests/test_board.cpp
    tests/test_game_session.cpp
    tests/test_settings.cpp
    tests/test_active_time_tracker.cpp
    tests/test_board_viewport.cpp
    tests/test_gesture_recognizer.cpp
  )
  target_link_libraries(minesweeper_tests PRIVATE minesweeper_core minesweeper_persist)
  add_test(NAME core COMMAND minesweeper_tests)
endif()
