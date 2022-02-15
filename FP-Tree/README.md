# FPTree PiBench Wrapper

### 1. Customize TBB

To achieve better scalability, we are using a customized Intel TBB library for FP-Tree (which is also the approach taken by the original author). Here are the steps to generate `libtbb.so`:

- Clone oneTBB from its original repo: https://github.com/oneapi-src/oneTBB.git
- Modify the read/write retry from 10 to 256 in `oneTBB/src/tbb/rtm_mutex.cpp` and `oneTBB/src/tbb/rtm_rw_mutex.cpp`
- Build it: `$ cd oneTBB && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -jN`
- Make sure that `libtbb.so` exists in `oneTBB/build/gnu_11.1_cxx11_64_release`

### 2. Build FPTree
First Modify `PMEMOBJ_POOL_SIZE` in `fptree.h` if `BACKEND = PMEM`. Then:

```
$ cd FP-Tree && mkdir build && cd build
$ cmake -DPMEM_BACKEND=${BACKEND} -DTEST_MODE=0 -DBUILD_INSPECTOR=0 -DNDEBUG=1 .. # BACKEND = DRAM/PMEM
$ make
````

### 3. Notes
FPTree code from https://github.com/sfu-dis/fptree.git (commit `98c25fa65070fe188ec4ae163e8b440c00cceaaf`)

A summary of modifications made:
1. Modified `CMakeLists.txt` to use the customized TBB (with # retries increased to 256).
2. Changed header files in `fptree.h` to include those from the customized TBB.
3. For DRAM mode, we introduced in-place node updates for better performance.
