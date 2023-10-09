cmake_minimum_required(VERSION 3.5.0)
project(PongC VERSION 0.0.0 LANGUAGES C CXX)

# C++ configuration -----
set(CMAKE_CXX_STANDARD 20)
# -----------------------

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "Output directory: ${OUTPUT_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Searchs for source and header files recursively
file(GLOB_RECURSE CSOURCES    src/*.c)
file(GLOB_RECURSE CPPSOURCES  src/*.cpp)
# -----------------------------------------------

add_executable(PongC ${CPPSOURCES} ${CSOURCES})

target_include_directories(PongC PUBLIC
    src
    include
)

set(LIBRARIES SDL2 SDL2main SDL2_ttf SDL2_image)

target_link_libraries(PongC PUBLIC ${LIBRARIES})
