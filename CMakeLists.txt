cmake_minimum_required(VERSION 3.20)

project(vcc
    VERSION     0.1.0
    DESCRIPTION "VCC – V Compiler Collection"
    LANGUAGES   CXX
)

# ─── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ─── Default build type ──────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug|Release|RelWithDebInfo)" FORCE)
endif()

# ─── Compiler diagnostics ────────────────────────────────────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow
    -Wno-unused-parameter
    $<$<CONFIG:Debug>:-g3>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-DNDEBUG>
)

# Address & UB sanitizers – opt-in via -DVCC_ENABLE_SANITIZERS=ON
option(VCC_ENABLE_SANITIZERS "Enable AddressSanitizer + UBSan (Debug only)" OFF)
if(VCC_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# ─── Global include root ─────────────────────────────────────────────────────
include_directories(${PROJECT_SOURCE_DIR}/include)

# ─── Feature options ─────────────────────────────────────────────────────────
option(VCC_BUILD_TESTS  "Build unit tests"     ON)
option(VCC_BUILD_TOOLS  "Build compiler tools" ON)
option(VCC_ENABLE_LLVM  "Enable LLVM backend"  ON)

# ─── Optional dependencies ───────────────────────────────────────────────────
if(VCC_BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
            GIT_SHALLOW    TRUE
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
endif()

if(VCC_ENABLE_LLVM)
    # Default to LLVM 18 if LLVM_DIR is not already set externally.
    if(NOT DEFINED LLVM_DIR)
        set(LLVM_DIR "/usr/lib/llvm-18/lib/cmake/llvm"
            CACHE PATH "Directory containing LLVMConfig.cmake")
    endif()

    # Ubuntu 22.04's libzstd-dev ships no CMake config; LLVM 18's export
    # targets require `zstd::libzstd_shared`.  Create it manually if absent.
    if(NOT TARGET zstd::libzstd_shared)
        find_library(ZSTD_LIBRARY NAMES zstd REQUIRED)
        add_library(zstd::libzstd_shared SHARED IMPORTED)
        set_target_properties(zstd::libzstd_shared PROPERTIES
            IMPORTED_LOCATION "${ZSTD_LIBRARY}")
    endif()

    find_package(LLVM REQUIRED CONFIG)
    if(LLVM_VERSION_MAJOR LESS 18)
        message(FATAL_ERROR "LLVM >= 18 required (found ${LLVM_PACKAGE_VERSION})")
    endif()
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
    include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
endif()

# ─── Compiler modules (order matters: dependencies first) ────────────────────
add_subdirectory(src/common)
add_subdirectory(src/lexer)
add_subdirectory(src/ast)
add_subdirectory(src/parser)
add_subdirectory(src/semantic)
add_subdirectory(src/ir)
add_subdirectory(src/codegen)
add_subdirectory(src/driver)

if(VCC_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(VCC_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# ─── Summary ─────────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "┌─────────────────────────────────────────┐")
message(STATUS "│   VCC – V Compiler Collection ${PROJECT_VERSION}      │")
message(STATUS "├─────────────────────────────────────────┤")
message(STATUS "│  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "│  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "│  Tests       : ${VCC_BUILD_TESTS}")
message(STATUS "│  LLVM backend: ${VCC_ENABLE_LLVM}")
message(STATUS "└─────────────────────────────────────────┘")
message(STATUS "")
