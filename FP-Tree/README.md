# FP+-Tree PiBench Wrapper

## Installation
1. To achieve better scalability, we are using customized tbb library for FP-Tree
(which is also the approach taken by the original author). Here are the steps to generate libtbb.so:
	a. Clone oneTBB from github (https://github.com/oneapi-src/oneTBB.git)
	b. Modify the read/write retry from 10 to 256 in oneTBB/src/tbb/rtm_mutex.cpp and oneTBB/src/tbb/rtm_rw_mutex.cpp
	c. cd oneTBB & mkdir build & cd build & cmake -DCMAKE_BUILD_TYPE=Release .. & make -j N
	d. check that libtbb.so exists in oneTBB/build/gnu_11.1_cxx11_64_release
2. `$ cd FP-Tree & mkdir build & cd build`
3. `$ cmake -DPMEM_BACKEND=${BACKEND} -DTEST_MODE=0 -DBUILD_INSPECTOR=0 -DNDEBUG=1 .. # BACKEND = DRAM/PMEM`
4. `$ make`

## Important information
This repo contains source code from https://github.com/sfu-dis/fptree.git
commit 98c25fa65070fe188ec4ae163e8b440c00cceaaf

Some modifications are made to compile pibench wrapper

1. Modified CMakeLists.txt to use custom tbb (with # retries increased to 256)
2. Changed header files in fptree.h to include those from custom tbb
