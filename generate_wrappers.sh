#!/bin/bash


# Can choose multiple indexes to generate pibench wrappers, for example: indexes="HOT Masstree"
indexes="" # DPTree HOT LBTree Masstree ROART uTree

# This is the absolute path for which the generated binaries will be copied to, will be created if not exist
binary_path="/mnt/pmem0/georgehe/pibench-ep2/wrappers"




if [ ! -d "$binary_path" ]; then
	eval "mkdir ${binary_path}"
	echo "Directory ${binary_path} created."
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
#if [[ "$indexes" == *"PACTree"* ]]; then	
	#echo "PACTree wrappers built complete."
#fi

# uTree
if [[ "$indexes" == *"uTree"* ]]; then
	eval "cd utree && rm -rf build && mkdir build && cd build && cmake .. && make && cp src/libutree_pibench_wrapper.so ${binary_path}/libutree_wrapper.so"
	eval "cd ../../../"
	echo "uTree wrapper built complete."
fi
