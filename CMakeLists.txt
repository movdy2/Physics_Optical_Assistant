cmake_minimum_required(VERSION 3.21)
project(PHY104OpticalAssistant VERSION 3.0.9 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PHY104_BUILD_GUI   "Build the Qt6 desktop application" ON)
option(PHY104_BUILD_TESTS "Build unit tests"                  ON)

# ------------------------------------------------------------------
# Version / build identity (used by the About dialog and version.json)
# ------------------------------------------------------------------
set(PHY104_VERSION_MAJOR 3)
set(PHY104_VERSION_MINOR 0)
set(PHY104_VERSION_PATCH 0)

# Git commit hash (empty if not a git repo or no commits)
set(PHY104_GIT_COMMIT "unknown")

# Build date (UTC)
string(TIMESTAMP PHY104_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC")

# Configure a header the About dialog can include
configure_file(${CMAKE_SOURCE_DIR}/src/core/version.h.in
               ${CMAKE_BINARY_DIR}/src/core/version.h
               @ONLY)

# Apply warning flags to each target explicitly so the production layers can use the
# stricter set without imposing new diagnostics on test fixtures.
function(phy104_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4)   # /utf-8: Greek symbols in source files
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if("${ARGV1}" STREQUAL "STRICT")
            target_compile_options(${target} PRIVATE -Wshadow -Wold-style-cast -Wconversion)
        endif()
    endif()
endfunction()

# Business layer: pure C++, no Qt -> easy to unit test
add_subdirectory(src/core)

# Data layer + application (needs Qt 6)
if(PHY104_BUILD_GUI)
    add_subdirectory(src)
endif()

if(PHY104_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
