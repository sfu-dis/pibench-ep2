# ROART PiBench Wrapper

Mostly working PiBench wrapper for ROART.

## Installation
1. Modify pool_path and pool_size in Key.h
make


## Important information
This repo contains source code from https://github.com/MiracleMa/ROART 
commit f3b30f56a722fc0b96a107da7eddec225baf3b70
Some modifications are made in order to compile pibench wrapper

## The original code is modified as follows:
* Modified Tree constructor in Tree.cpp
* Removed #include "generator.h" in util.h (the file is not provided)
* Removed #include "timer.h" in Tree.cpp and threadinfo.cpp (the file is not provided)
* Modified get_filename() in nvm_mgr.h
* Added #include <map> in LeafArray.h
* Removed definition and use of graphviz_debug 
* Removed node->old_pointer.store(0); from rebuild_node() in N.cpp
* Added macros to Key.h
* Modified k->value into &k->value in line 27 of N.cpp

## Comment
KEY_INLINE --> Insert, Read, Update, Delete
KEY_INLINE + ZENTRY --> Segfault
KEY_INLINE + ZENTRY + LEAF_ARRAY --> Insert no error, read always fail, update segfault, delete no error