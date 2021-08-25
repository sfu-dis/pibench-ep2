# utree PiBench Wrapper

PiBench wrapper for utree.

## Installation
```bash
mkdir build
cd build 
cmake ..
make
```


## This repo contains souce code from https://github.com/thustorage/nvm-datastructure.git 
commit c57b27f92baf9737e291b29378090da8feb166ce

The following modifications are made in order to compile the pibench wrappers
1. Modified linear_search_pred to cover a corner case that would cause delete to find
wrong previous node.

