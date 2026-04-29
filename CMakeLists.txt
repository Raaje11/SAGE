cmake_minimum_required(VERSION 3.21)

project(SAGE LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional but useful for clangd / tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only Phase 1 (core/common) for now
add_subdirectory(core/common)