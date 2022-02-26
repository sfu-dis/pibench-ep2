# ROART PiBench Wrapper

PiBench wrapper for ROART.

## Installation

To build PMEM ROART that uses its own DCMM allocator
```
make
```

To build PMEM ROART that uses PMDK allocator
```
make CFLAGS=-DARTPMDK
```

To build DRAM ROART
```
make CFLAGS=-DDRAM_MODE
```


## Important information
This repo contains source code from https://github.com/MiracleMa/ROART 

commit f3b30f56a722fc0b96a107da7eddec225baf3b70

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
* Set max_threads to 100 in nvm_mgr.h
* Modified N::key_keylen_lt in N.cpp for little endian machines
* Added DRAM_MODE in Key.h and util.h

