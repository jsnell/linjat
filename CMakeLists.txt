cmake_minimum_required(VERSION 2.8)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -g3 -Wall -Werror -Wno-sign-compare -march=native")

include_directories("src")
add_executable(mkpicju
               src/main.cc)

# foreach(level
#     00 01 02 03 04 05 06 07 08 09
#     10 11 12 13 14 15 16 17 18 19
#     20 21 22 23 24 25 26 27 28 29
#     30 31 32 33 34 35 36 37 38 39
#     40 41 42 43 44 45
#     star1 star2 star3 star4 star5 star6 void) 
#   add_executable(snakebird.${level}
#     src/snakebird/level${level}.cc
#     src/third-party/cityhash/city.cc)
#   target_link_libraries(snakebird.${level} zstd)
# endforeach()

# find_library(zstd libstd)

