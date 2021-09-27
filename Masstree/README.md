# Masstree PiBench Wrapper

PiBench wrapper for Masstree.

## Installation
```bash
cd masstree-beta 
./bootstrap.sh    
./configure --disable-assertions --with-malloc=jemalloc
make
```


## This repo contains souce code from https://github.com/kohler/masstree-beta 
commit cef4cc4f68953bdc4d0aec736cb8e1ce0700a4ae
The following modifications are made in order to compile the pibench wrappers

1. Added #include "config.h" at top of compiler.hh file 
2. Modified GNUMakefile.in to include libmasstree_wrapper
