cmake_minimum_required(VERSION 3.3)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O2 -g3 -Wall -Werror -Wno-sign-compare -march=native -DMAP_HEIGHT=$ENV{MAP_HEIGHT} -DMAP_WIDTH=$ENV{MAP_WIDTH} -DPIECES=$ENV{PIECES}")

include_directories("src")
add_executable(mklinjat
               src/main.cc)
