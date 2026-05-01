cmake_minimum_required(VERSION 3.21)

# ── Compiler — MUST be before project() ──────────────────────────────────
set(CMAKE_C_COMPILER
    "D:/clang+llvm-22.1.0-x86_64-pc-windows-msvc/bin/clang.exe"
    CACHE STRING "" FORCE)
set(CMAKE_CXX_COMPILER
    "D:/clang+llvm-22.1.0-x86_64-pc-windows-msvc/bin/clang++.exe"
    CACHE STRING "" FORCE)
# ─────────────────────────────────────────────────────────────────────────

project(SAGE LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core/common)

target_include_directories(sage_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(sage_common
    INTERFACE
        cxx_std_20
)

target_compile_options(sage_common
    INTERFACE
        $<$<CXX_COMPILER_ID:Clang>:-Wall>
        $<$<CXX_COMPILER_ID:Clang>:-Wextra>
        $<$<CXX_COMPILER_ID:Clang>:-Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-c++17-extensions>
        $<$<CXX_COMPILER_ID:GNU>:-Wall>
        $<$<CXX_COMPILER_ID:GNU>:-Wextra>
        $<$<CXX_COMPILER_ID:GNU>:-Wpedantic>
)