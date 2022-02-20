#!/bin/bash


# Can choose multiple indexes to generate pibench wrappers, for example: indexes="HOT Masstree"
indexes="FPTree DPTree HOT LBTree Masstree ROART PACTree uTree" # FPTree DPTree HOT LBTree Masstree ROART PACTree uTree

# This is the absolute path to folder for which the generated binaries will be copied to, will be created if not exist
binary_path="/mnt/pmem0/georgehe/pibench-ep2/wrappers"




if [ ! -d "$binary_path" ]; then
	eval "mkdir ${binary_path}"
	echo "Directory ${binary_path} created."
fi


# FPTree
if [[ "$indexes" == *"FPTree"* ]]; then
	eval "cd FPTree"
	if [ ! -d "./oneTBB" ]; then
		eval "git clone https://github.com/oneapi-src/oneTBB.git"
	fi
	eval "cp rtm_mutex.cpp oneTBB/src/tbb/rtm_mutex.cpp && cp rtm_rw_mutex.cpp oneTBB/src/tbb/rtm_rw_mutex.cpp"
	eval "cd oneTBB && rm -rf build && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j"
	eval "cd ../../ && rm -rf build && mkdir build && cd build"

	eval "cmake -DPMEM_BACKEND=PMEM -DTEST_MODE=0 -DBUILD_INSPECTOR=0 -DNDEBUG=1 .. && make"
	eval "mv src/libfptree_pibench_wrapper.so ${binary_path}/libfptree_pmem.so"

	eval "cmake -DPMEM_BACKEND=PMEM -DTEST_MODE=0 -DBUILD_INSPECTOR=0 -DNDEBUG=1 -DVAR_KEY=1 .. && make"
        eval "mv src/libfptree_pibench_wrapper.so ${binary_path}/libfptree_pmem_varkey.so"	

	eval "cmake -DPMEM_BACKEND=DRAM -DTEST_MODE=0 -DBUILD_INSPECTOR=0 -DNDEBUG=1 .. && make"
	eval "mv src/libfptree_pibench_wrapper.so ${binary_path}/libfptree_dram.so"

	eval "cd ../../"
        echo "FPTree wrappers built complete."
fi

# DPTree
if [[ "$indexes" == *"DPTree"* ]]; then
	eval "cd DPTree && rm -rf build && mkdir build && cd build && cmake .. && make && mv libdptree_pibench_wrapper.so ${binary_path}/libdptree_pmem.so"
	eval "cmake -DVAR_KEY=1 .. && make && mv libdptree_pibench_wrapper.so ${binary_path}/libdptree_pmem_varkey.so"
	eval "cd ../../"
        echo "DPTree wrappers built complete."
fi

# Hot
if [[ "$indexes" == *"HOT"* ]]; then
	eval "cd Hot"
	if [ ! -d "./hot" ]; then
		eval "git clone https://github.com/speedskater/hot.git && cp CMakeLists.txt hot/ && cd hot && git submodule update --init --recursive"
	else
		eval "cd hot"
	fi
	eval "rm -rf build && mkdir build && cd build && cmake .. && make && mv libhot_wrapper.so ${binary_path}/libhot_wrapper.so"
	eval "cd ../../../"
        echo "HOT wrapper built complete."
fi

# LBTree
if [[ "$indexes" == *"LBTree"* ]]; then
	eval "cd LB+-Tree && make && mv liblbtree_wrapper.so ${binary_path}/liblbtree_dram.so"
	eval "make CFLAGS=-DPMEM && mv liblbtree_wrapper.so ${binary_path}/liblbtree_pmem.so"
	eval "make CFLAGS='-DPMEM -DVAR_KEY' && mv liblbtree_wrapper.so ${binary_path}/liblbtree_pmem_varkey.so"
	eval "cd .."        
	echo "LBTree wrappers built complete."
fi

# Masstree
if [[ "$indexes" == *"Masstree"* ]]; then
	eval "cd Masstree/masstree-beta && ./bootstrap.sh && ./configure --disable-assertions --with-malloc=jemalloc && make"
	eval "mv libmasstree_wrapper.so ${binary_path}/libmasstree_wrapper.so"
	eval "cd ../../"
	echo "Masstree wrapper built complete."       
fi

# ROART
if [[ "$indexes" == *"ROART"* ]]; then
	eval "cd ROART && make && mv libroart_wrapper.so ${binary_path}/libroart_dcmm.so"
        eval "make CFLAGS=-DARTPMDK && mv libroart_wrapper.so ${binary_path}/libroart_pmdk.so"
        eval "make CFLAGS=-DDRAM_MODE && mv libroart_wrapper.so ${binary_path}/libroart_dram.so"
        eval "cd .."        
	echo "ROART wrappers built complete."
fi

# PACTree
if [[ "$indexes" == *"PACTree"* ]]; then
	eval "cd pactree && rm -rf build && mkdir build && cd build && cmake .. && make && cp src/libpactree_pibench_wrapper.so ${binary_path}/libpactree_pmem.so"
	eval "cd ../../"	
	echo "PACTree wrappers built complete."
fi

# uTree
if [[ "$indexes" == *"uTree"* ]]; then
	eval "cd utree && rm -rf build && mkdir build && cd build && cmake .. && make && cp src/libutree_pibench_wrapper.so ${binary_path}/libutree_wrapper.so"
	eval "cd ../../"
	echo "uTree wrapper built complete."
fi
