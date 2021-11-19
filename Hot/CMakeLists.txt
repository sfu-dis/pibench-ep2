cmake_minimum_required(VERSION 2.8)

PROJECT(HOT_ROOT)

set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/third-party/bilke-cmake-modules/")

#add_definitions(-DMEMORY_FOOTPRINT)

execute_process(COMMAND cat /proc/cpuinfo COMMAND grep avx2 OUTPUT_VARIABLE HAVE_AVX2)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep bmi2 OUTPUT_VARIABLE HAVE_BMI2)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep avx512 OUTPUT_VARIABLE HAVE_AVX512)

if(HAVE_AVX2 AND HAVE_BMI2)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ljemalloc -std=c++17 -march=native -mrtm -mcx16 -mavx -mavx2 -mbmi2 -mlzcnt -Wno-deprecated-declarations -Wall -Wextra -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -faligned-new=64 -DNDEBUG -ltbb")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
else()
	message(FALSE_ERROR "Cannot find avx2 & bmi2 --> cannot run HOT")
endif()

if(HAVE_AVX512)
	SET(CMAKE_CXX_FLAGS "-mavx512f -mavx512vl -mavx512bw -mavx512dq -mavx512cd ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
	add_definitions(-DUSE_AVX512)
	message(STATUS "Using avx512")
endif()


set(TC_MALLOC_NAMES tcmalloc tcmalloc_minimal libtcmalloc_minimal.so.4.3.0 )

find_library(TC_MALLOC_LIBRARY
	NAMES ${TC_MALLOC_NAMES}
	PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /lib /usr/lib /usr/local/lib /opt/local/lib
	NO_DEFAULT_PATH
)

#SET(TBB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/tbb")
#include(${TBB_ROOT}/cmake/TBBBuild.cmake)
#tbb_build(TBB_ROOT ${TBB_ROOT} CONFIG_DIR TBB_DIR MAKE_ARGS)
#find_package(TBB REQUIRED)

#SET(Hana_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/boost-hana")


ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND})

#add_subdirectory(third-party/boost-cmake)

ADD_SUBDIRECTORY(libs)
include_directories("libs/hot/commons/include"
		    "libs/idx/content-helpers/include"
		    "libs/hot/rowex/include")


# Build hot shared library
add_library(hot_wrapper SHARED ../hot_wrapper.cpp
								third-party/tbb/src/old/concurrent_vector_v2.cpp
								third-party/tbb/src/tbb/concurrent_vector.cpp)
