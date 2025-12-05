cmake_minimum_required(VERSION 3.25)
project(cobble_compiler CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Define Sources ---
# Gather all source files except main.cpp and unit_tests.cpp
# This allows us to share the implementation between the compiler and the tests.
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "/main\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX "/unit_tests\\.cpp$")

# --- 2. Create Core Library ---
# We build the logic as a static library
add_library(cobble_core STATIC ${SOURCES})
target_include_directories(cobble_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- 3. Compiler Executable ---
# The actual compiler just links against the core logic
add_executable(cobble src/main.cpp)
target_link_libraries(cobble PRIVATE cobble_core)

# --- 4. Google Test Setup ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)
FetchContent_MakeAvailable(googletest)

# --- 5. Test Executable ---
enable_testing()

add_executable(unit_tests src/unit_tests.cpp)

# Link against your core library AND gtest
target_link_libraries(unit_tests PRIVATE
        cobble_core
        GTest::gtest_main
)

# Helper to auto-discover tests so you can see them in your IDE
include(GoogleTest)
gtest_discover_tests(unit_tests)