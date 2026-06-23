cmake_minimum_required(VERSION 3.20)
project(NekoCurX LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile options
add_compile_options(-Wall -Wextra -Wpedantic)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    # add_compile_options(-fsanitize=address -fsanitize=undefined)
    # add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

include(FetchContent)

# 1. Fetch CLI11
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
)

# 2. Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# 3. Fetch Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.0
)

# 4. Fetch tl-expected
FetchContent_Declare(
    tl_expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG        v1.1.0
)

# Disable tests and package building for tl_expected
set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(EXPECTED_BUILD_PACKAGE OFF CACHE BOOL "" FORCE)

# Make dependencies available
FetchContent_MakeAvailable(CLI11 nlohmann_json Catch2 tl_expected)

# Find X11 and Xcursor
find_package(PkgConfig REQUIRED)
pkg_check_modules(XCURSOR REQUIRED xcursor)
pkg_check_modules(LIBPNG REQUIRED libpng)

# Include directories
include_directories(include)
include_directories(${XCURSOR_INCLUDE_DIRS} ${LIBPNG_INCLUDE_DIRS})
link_directories(${XCURSOR_LIBRARY_DIRS} ${LIBPNG_LIBRARY_DIRS})

add_subdirectory(src)
add_subdirectory(tests)
