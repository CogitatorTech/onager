cmake_minimum_required(VERSION 3.5)


set(CORROSION_VERBOSE_OUTPUT ON)

# We need C++17 for std::filesystem on all platforms
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --- Fallback platform detection (only if DuckDB build system did not set these) ---
# DuckDB's extension build usually injects OS_NAME / OS_ARCH / DUCKDB_PLATFORM.
# When users run `cmake .` manually these are empty -> we derive sensible defaults.
set(_ONAGER_ORIG_OS_NAME "${OS_NAME}")
set(_ONAGER_ORIG_OS_ARCH "${OS_ARCH}")
if("${OS_NAME}" STREQUAL "")
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" _ONAGER_OS_NAME)
  set(OS_NAME "${_ONAGER_OS_NAME}")
  set(_onager_fallback_platform_detected TRUE)
else()
  set(_ONAGER_OS_NAME "${OS_NAME}")
endif()
if("${OS_ARCH}" STREQUAL "")
  set(_ONAGER_OS_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
  set(OS_ARCH "${_ONAGER_OS_ARCH}")
  set(_onager_fallback_platform_detected TRUE)
else()
  set(_ONAGER_OS_ARCH "${OS_ARCH}")
endif()
# Normalise common architecture aliases
if(_ONAGER_OS_ARCH STREQUAL "x86_64" OR _ONAGER_OS_ARCH STREQUAL "amd64")
  set(_ONAGER_OS_ARCH amd64)
elseif(_ONAGER_OS_ARCH STREQUAL "aarch64" OR _ONAGER_OS_ARCH STREQUAL "arm64")
  set(_ONAGER_OS_ARCH arm64)
endif()
set(OS_ARCH "${_ONAGER_OS_ARCH}")

# Derive DUCKDB_PLATFORM if missing (this is a rough heuristic mirroring DuckDB naming convention)
# Valid values: linux_amd64, linux_arm64, osx_amd64, osx_arm64, osx_universal, windows_amd64
# Note: we do not currently handle Windows ARM (windows_arm64) as DuckDB does not provide such builds.
# Note: we do not currently handle musl vs gnu variants on Linux; users can manually set DUCKDB_PLATFORM if needed.
# Note: we do not currently handle wasm variants; users can manually set DUCKDB_PLATFORM if needed.
set(DUCKDB_PLATFORM "${DUCKDB_PLATFORM}")
if("${DUCKDB_PLATFORM}" STREQUAL "")
  if(_ONAGER_OS_NAME STREQUAL "linux")
    if(_ONAGER_OS_ARCH STREQUAL "amd64")
      set(DUCKDB_PLATFORM linux_amd64)
    elseif(_ONAGER_OS_ARCH STREQUAL "arm64")
      set(DUCKDB_PLATFORM linux_arm64)
    endif()
  elseif(_ONAGER_OS_NAME MATCHES "darwin|osx")
    if(_ONAGER_OS_ARCH STREQUAL "arm64")
      set(DUCKDB_PLATFORM osx_arm64)
    elseif(_ONAGER_OS_ARCH STREQUAL "amd64")
      set(DUCKDB_PLATFORM osx_amd64)
    else()
      set(DUCKDB_PLATFORM osx_universal)
    endif()
  elseif(WIN32)
    if(_ONAGER_OS_ARCH STREQUAL "amd64")
      set(DUCKDB_PLATFORM windows_amd64)
    endif()
  endif()
endif()

if(_onager_fallback_platform_detected)
  message(STATUS "[onager] Fallback platform detection engaged: OS_NAME='${OS_NAME}' OS_ARCH='${OS_ARCH}' DUCKDB_PLATFORM='${DUCKDB_PLATFORM}'")
endif()

set(prefix_to_check "wasm")
string(LENGTH "${prefix_to_check}" prefix_length)
string(SUBSTRING "${DUCKDB_PLATFORM}" 0 ${prefix_length} extracted_platform_prefix)

execute_process(
    COMMAND rustup target list --installed
    OUTPUT_VARIABLE RUST_TARGETS
)

# Propagate arch to rust build for CI (best-effort; corrosion will still build host default if unset)
set(Rust_CARGO_TARGET "")
if(_ONAGER_OS_NAME STREQUAL "linux")
    if (_ONAGER_OS_ARCH STREQUAL "arm64")
        set(Rust_CARGO_TARGET "aarch64-unknown-linux-gnu")
    elseif("${CMAKE_CXX_COMPILER}" MATCHES "aarch64")
        # Cross-compilation detected via compiler
        set(Rust_CARGO_TARGET "aarch64-unknown-linux-gnu")
    else()
        string(FIND "${RUST_TARGETS}" "musl" MUSL_TARGET_FOUND)
        if(NOT MUSL_TARGET_FOUND EQUAL -1)
            set(Rust_CARGO_TARGET "x86_64-unknown-linux-musl")
        else()
            set(Rust_CARGO_TARGET "x86_64-unknown-linux-gnu")
        endif()
    endif()
elseif(_ONAGER_OS_NAME MATCHES "darwin|osx")
    # Check OSX_BUILD_ARCH first (if set by DuckDB build system), then fall back to OS_ARCH
    if("${OSX_BUILD_ARCH}" STREQUAL "arm64")
        set(Rust_CARGO_TARGET "aarch64-apple-darwin")
    elseif("${OSX_BUILD_ARCH}" STREQUAL "x86_64")
        set(Rust_CARGO_TARGET "x86_64-apple-darwin")
    elseif(_ONAGER_OS_ARCH STREQUAL "arm64")
        set(Rust_CARGO_TARGET "aarch64-apple-darwin")
    elseif(_ONAGER_OS_ARCH STREQUAL "amd64")
        set(Rust_CARGO_TARGET "x86_64-apple-darwin")
    endif()
elseif(WIN32)
    # Distinguish between MINGW and MSVC toolchains
    if(MINGW AND _ONAGER_OS_ARCH STREQUAL "arm64")
        set(Rust_CARGO_TARGET "aarch64-pc-windows-gnu")
    elseif(MINGW AND _ONAGER_OS_ARCH STREQUAL "amd64")
        set(Rust_CARGO_TARGET "x86_64-pc-windows-gnu")
    elseif(MSVC AND _ONAGER_OS_ARCH STREQUAL "arm64")
        set(Rust_CARGO_TARGET "aarch64-pc-windows-msvc")
    elseif(MSVC AND _ONAGER_OS_ARCH STREQUAL "amd64")
        set(Rust_CARGO_TARGET "x86_64-pc-windows-msvc")
    elseif(_ONAGER_OS_ARCH STREQUAL "arm64")
        # Default to MSVC for ARM64
        set(Rust_CARGO_TARGET "aarch64-pc-windows-msvc")
    elseif(_ONAGER_OS_ARCH STREQUAL "amd64")
        # Default to MSVC for x64
        set(Rust_CARGO_TARGET "x86_64-pc-windows-msvc")
    endif()
endif()

string(FIND "${RUST_TARGETS}" "wasm32-unknown-emscripten" WASM_TARGET_FOUND)
if (NOT WASM_TARGET_FOUND EQUAL -1)
  set(Rust_CARGO_TARGET "wasm32-unknown-emscripten")
endif()

# Final safety net default
if("${Rust_CARGO_TARGET}" STREQUAL "")
  set(Rust_CARGO_TARGET "x86_64-unknown-linux-gnu")
  message(STATUS "[onager] Defaulted Rust_CARGO_TARGET to host x86_64-unknown-linux-gnu")
endif()

message(STATUS "RUST_TARGETS: ${RUST_TARGETS}")
message(STATUS "WASM_TARGET_FOUND: ${WASM_TARGET_FOUND}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "OSX_BUILD_ARCH: ${OSX_BUILD_ARCH}")
message(STATUS "MINGW: ${MINGW}")
message(STATUS "MSVC: ${MSVC}")
message(STATUS "OS_NAME: ${OS_NAME} (orig='${_ONAGER_ORIG_OS_NAME}')")
message(STATUS "OS_ARCH: ${OS_ARCH} (orig='${_ONAGER_ORIG_OS_ARCH}')")
message(STATUS "DUCKDB_PLATFORM: ${DUCKDB_PLATFORM}")
message(STATUS "Rust_CARGO_TARGET: ${Rust_CARGO_TARGET}")


# ==============================================================================
# Corrosion (Rust integration)
# ==============================================================================
include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5.2
)
FetchContent_MakeAvailable(Corrosion)

# Detect if we are being included in an in-tree DuckDB build (source dir name == duckdb)
get_filename_component(_TOP_SOURCE_DIR_NAME "${CMAKE_SOURCE_DIR}" NAME)
if(_TOP_SOURCE_DIR_NAME STREQUAL "duckdb")
  set(ONAGER_IN_TREE_DUCKDB_BUILD TRUE)
else()
  set(ONAGER_IN_TREE_DUCKDB_BUILD FALSE)
endif()

# Derive the extension source root robustly (directory containing this CMakeLists.txt)
get_filename_component(ONAGER_EXTENSION_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(ONAGER_CRATE_MANIFEST "${ONAGER_EXTENSION_DIR}/onager/Cargo.toml")
if(NOT EXISTS "${ONAGER_CRATE_MANIFEST}")
  message(FATAL_ERROR "[onager] Expected Rust crate manifest not found at ${ONAGER_CRATE_MANIFEST}. CMAKE_CURRENT_LIST_FILE='${CMAKE_CURRENT_LIST_FILE}'")
endif()

if(NOT ONAGER_IN_TREE_DUCKDB_BUILD)
  message(STATUS "[onager] Standalone mode: importing Rust crate via Corrosion (${ONAGER_CRATE_MANIFEST})")
  # Compose feature list for Rust build
  set(ONAGER_RUST_FEATURES duckdb_extension)
  message(STATUS "[onager] Rust features: ${ONAGER_RUST_FEATURES}")
  corrosion_import_crate(MANIFEST_PATH "${ONAGER_CRATE_MANIFEST}" CRATES "onager" FEATURES ${ONAGER_RUST_FEATURES})
else()
  message(STATUS "[onager] In-tree DuckDB build detected; skipping Corrosion import (Rust static lib expected to be pre-built and linked via extension_config.cmake)")
endif()

set(TARGET_NAME onager)
set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)
project(${TARGET_NAME})

include_directories(onager/bindings/include)
set(EXTENSION_SOURCES
    onager/bindings/onager_extension.cpp
    onager/bindings/functions/scalar_functions.cpp
    onager/bindings/functions/centrality.cpp
    onager/bindings/functions/community.cpp
    onager/bindings/functions/traversal.cpp
    onager/bindings/functions/generators.cpp
    onager/bindings/functions/links.cpp
    onager/bindings/functions/metrics.cpp
    onager/bindings/functions/mst.cpp
    onager/bindings/functions/approximation.cpp
    onager/bindings/functions/personalized.cpp
    onager/bindings/functions/subgraphs.cpp
    onager/bindings/functions/parallel.cpp
)

# Remove previous attempt to include DuckDB extension macros when unavailable.
# For a plain standalone configure (cmake ..) we just build the Rust crate; the
# DuckDB C++ extension requires DuckDB's macro definitions. Users should invoke
# the provided Makefile (e.g. `make release`) or integrate with DuckDB's build
# to compile the extension targets. We detect absence of build_static_extension
# and skip gracefully.
if(NOT COMMAND build_static_extension)
  message(STATUS "[onager] DuckDB extension build macros not present (standalone configure). Skipping C++ extension targets.")
else()
  build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
  build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})
endif()

# Remove stale linking logic guarded by targets that won't exist in standalone mode.
if(COMMAND build_static_extension)
  if(TARGET ${EXTENSION_NAME})
    if(TARGET onager-static)
      target_link_libraries(${EXTENSION_NAME} onager-static)
    elseif(TARGET onager)
      target_link_libraries(${EXTENSION_NAME} onager)
    endif()
  endif()
  if(TARGET ${LOADABLE_EXTENSION_NAME})
    if(TARGET onager)
      target_link_libraries(${LOADABLE_EXTENSION_NAME} onager)
    elseif(TARGET onager-static)
      target_link_libraries(${LOADABLE_EXTENSION_NAME} onager-static)
    endif()
  endif()
endif()

if(TARGET ${EXTENSION_NAME})
  install(
    TARGETS ${EXTENSION_NAME}
    EXPORT "${DUCKDB_EXPORT_SET}"
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
else()
  message(STATUS "[onager] No C++ extension target built; skipping install directive.")
endif()

# Link filesystem library on older libstdc++ (GCC < 9 requires -lstdc++fs)
set(_ONAGER_FS_LIB "")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    set(_ONAGER_FS_LIB stdc++fs)
  endif()
endif()

if(_ONAGER_FS_LIB)
  if (TARGET ${EXTENSION_NAME})
    target_link_libraries(${EXTENSION_NAME} ${_ONAGER_FS_LIB})
  endif()
  if (TARGET ${LOADABLE_EXTENSION_NAME})
    target_link_libraries(${LOADABLE_EXTENSION_NAME} ${_ONAGER_FS_LIB})
  endif()
endif()
