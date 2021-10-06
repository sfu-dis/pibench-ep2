# ROART PiBench Wrapper

Mostly working PiBench wrapper for ROART.

## Installation
1. (optional) Configure pool_path and pool_size in Key.h
2. Set Mode (#define DRAM_MODE if running entirely in dram) in Key.h and util.h
If running in DRAM_MODE and you wish to run using thread-local mempool instead of just malloc/jemalloc,
add -DPOOL at end of CFLAGS in Makefile.
3. Toggle allocator (#define ARTPMDK) only when DRAM_MODE is not defined.
If DRAM_MODE is defined then malloc will be used for all allocation.
4. make


## Important information
This repo contains source code from https://github.com/MiracleMa/ROART 
commit f3b30f56a722fc0b96a107da7eddec225baf3b70

Some modifications are made in order to compile pibench wrapper

## The original code is modified as follows:
* Modified Tree constructor in Tree.cpp
* Removed #include "generator.h" in util.h (file is not provided)
* Removed #include "timer.h" in Tree.cpp and threadinfo.cpp (file is not provided)
* Modified get_filename() in nvm_mgr.h
* Added #include <map> in LeafArray.h
* Removed definition and use of graphviz_debug 
* Removed node->old_pointer.store(0); from rebuild_node() in N.cpp (compiler error)
* Added macros to Key.h
* Modified k->value into &k->value in line 27 of N.cpp (runtime error)
* Applied a fix from github https://github.com/MiracleMa/ROART/issues/2 (runtime error)
* Modified PMDK allocator when #ARTPMDK is defined
* Modified max_threads to 100 in nvm_mgr.h
* Modified N::key_keylen_lt in N.cpp for little endian machines
* Added DRAM_MODE in Key.h and util.h

