# Cross toolchain for Kobo e-readers via koxtoolchain.
#
# One-time setup inside WSL2 (Ubuntu):
#   git clone https://github.com/koreader/koxtoolchain
#   cd koxtoolchain && ./gen-tc.sh kobo        # ~30 min, installs to ~/x-tools
#
# Then:
#   cmake -B build/kobo -DCMAKE_TOOLCHAIN_FILE=cmake/kobo-toolchain.cmake \
#         -DMINESWEEPER_BACKEND=fbink
# Override the toolchain location with -DCROSS_TC_ROOT=... or $CROSS_TC_ROOT.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(TC_TRIPLE arm-kobo-linux-gnueabihf)
if(NOT DEFINED CROSS_TC_ROOT)
  if(DEFINED ENV{CROSS_TC_ROOT})
    set(CROSS_TC_ROOT "$ENV{CROSS_TC_ROOT}")
  else()
    set(CROSS_TC_ROOT "$ENV{HOME}/x-tools/${TC_TRIPLE}")
  endif()
endif()

set(CMAKE_C_COMPILER   "${CROSS_TC_ROOT}/bin/${TC_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${CROSS_TC_ROOT}/bin/${TC_TRIPLE}-g++")

set(CMAKE_FIND_ROOT_PATH "${CROSS_TC_ROOT}/${TC_TRIPLE}/sysroot")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ARMv7 hard-float floor: runs on every Nickel-era Kobo (docs/contracts/install-layout.md).
set(ARCH_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT   "${ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ARCH_FLAGS}")
# No bundled .so on the device: keep the C++ runtime inside the binary.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static-libstdc++ -static-libgcc")
