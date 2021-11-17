cmake_minimum_required(VERSION 3.14)
include(FetchContent)
project(DPTree)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

#add_definitions(-DMEMORY_FOOTPRINT)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c++17 -DNDEBUG -lpmem -lpmemobj -ljemalloc -mavx512f -mavx512vl -mavx512bw -mavx512dq -mavx512cd -mrtm -ltbb -pthread")

#macro(use_cxx11)
#    if (CMAKE_VERSION VERSION_LESS "3.1")
#        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
#        endif ()
#    else ()
#        set (CMAKE_CXX_STANDARD 11)
#    endif ()
#endmacro(use_cxx11)

#set(CMAKE_CXX_STANDARD 11)
#use_cxx11()

include_directories(include)
include_directories(misc)

set (SIMD_FLAGS "-g3 -msse -msse2 -mavx -mrtm")
execute_process(COMMAND bash "-c" "cat /proc/cpuinfo | grep avx2"
        OUTPUT_VARIABLE AVX2_ENABLED)
if (AVX2_ENABLED STREQUAL "")
    message(STATUS "avx2 instruction set is required" )
else ()
    set (SIMD_FLAGS "${SIMD_FLAGS} -mavx2")
endif()
execute_process(COMMAND bash "-c" "cat /proc/cpuinfo | grep avx512"
        OUTPUT_VARIABLE AVX512_ENABLED)
if (AVX512_ENABLED STREQUAL "")
    message(STATUS "avx512 is not enabled on this machine")
else()
    set (SIMD_FLAGS "${SIMD_FLAGS} -mavx512f -DHAS_AVX512")
endif()

#-DCOUNT_CLFLUSH  -DCOUNT_CLFLUSH -DNVM_DRAM_MODE -DNVM_DRAM_MODE  -DMEASURE_LEAF_SEARCH  -DNVM_DRAM_MODE
set (FLAGS " -DCOUNT_CLFLUSH -fpermissive -ftree-vectorize")
set (FLAGS "${FLAGS} ")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS} ${SIMD_FLAGS}")

message(STATUS "${CMAKE_CXX_FLAGS}")

#set(CMAKE_CXX_FLAGS_MYREL "-O3 -g")

#set (CMAKE_BUILD_TYPE "RelWithDebInfo")


#add_executable (concur_dptree test/concur_dptree_test.cxx
#        src/util.cpp
#        src/ART.cpp
#        misc/ARTOLC/Epoche.cpp
#        misc/ARTOLC/Tree.cpp
#        src/art_idx.cpp
#        src/MurmurHash2.cpp
#        src/bloom.c)

#add_executable (dptree test/dptree_test.cxx
#        src/util.cpp
#        src/ART.cpp
#        misc/ARTOLC/Epoche.cpp
#        misc/ARTOLC/Tree.cpp
#        src/art_idx.cpp
#        src/MurmurHash2.cpp
#        src/bloom.c)


#target_link_libraries (concur_dptree pthread tbb tcmalloc_minimal)
#target_link_libraries (dptree pthread tbb tcmalloc_minimal)


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


add_library(dptree_pibench_wrapper SHARED dptree_wrapper.cpp
                                        src/art_idx.cpp
                                        src/util.cpp
                                        src/bloom.c
                                        src/MurmurHash2.cpp)

