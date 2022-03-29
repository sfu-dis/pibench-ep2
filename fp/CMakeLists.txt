cmake_minimum_required(VERSION 3.14)
include(FetchContent)
project(fptree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17 -lpmem -lpmemobj -ljemalloc -mavx512f -mavx512vl -mavx512bw -mavx512dq -mavx512cd -mrtm -pthread")


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)

add_definitions(-DNDEBUG)
message(STATUS "NDEBUG: defined")


##################### PiBench #########################
FetchContent_Declare(
        pibench
        GIT_REPOSITORY https://github.com/sfu-dis/pibench.git
        GIT_TAG master
)
if (NOT pibench_POPULATED)
  FetchContent_Populate(pibench)
  include_directories(${pibench_SOURCE_DIR}/include)
endif ()



add_library(fp_pibench_wrapper SHARED fptree_wrapper.cpp
                                                fptree.cpp)