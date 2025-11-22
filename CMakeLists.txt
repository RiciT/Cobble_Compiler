cmake_minimum_required(VERSION 3.25)
project(cobble VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find all .cpp files recursively in src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 2. Create the executable using those sources
add_executable(cobble ${SOURCES})

# 3. Add 'src' to the include path
# This allows you to do #include "lexing/token.hpp" instead of "../lexing/token.hpp"
target_include_directories(cobble PRIVATE src)

# Optional: Enable all warnings (Very helpful for compilers!)
if(MSVC)
    target_compile_options(cobble PRIVATE /W4)
else()
    target_compile_options(cobble PRIVATE -Wall -Wextra -pedantic)
endif()