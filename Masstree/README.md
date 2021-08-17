# Masstree PiBench Wrapper

PiBench wrapper for Masstree.

## Installation
cd masstree-beta
./bootstrap.sh
./configure --disable-assertions --with-malloc=<jemalloc|tcmalloc|flow|hoard>
make


## This repo contains souce code from https://github.com/kohler/masstree-beta
The following modifications are made in order to compile the pibench wrappers
1. Added #include "config.h" at top of compiler.hh
2. Modified GNUMakefile.in to include libmasstree_wrapper