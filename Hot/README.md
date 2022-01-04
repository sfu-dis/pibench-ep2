# HOT PiBench Wrapper

PiBench wrapper for HOT.

## Installation
```bash
git clone https://github.com/speedskater/hot.git
cp CMakeLists.txt hot/
cd hot
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make
```

## Important Information
1. Hot can only embed 63 bits key in index, therefore the most significant bit 
of every key is cleared before insertion (see offset in wrapper.h)
2. Since hot only stores the pointer to entire record. The key/value struct 
needs to be in scope until benchmark finishes. The current approach is to 
create new record on heap upon insert/update.
3. Current version of HOT using ROWEX does not support delete operation

## This repo contains souce code from https://github.com/speedskater/hot.git
The following modifications are made in order to compile the pibench wrapper
1. Modified CMakeLists.txt to include libhot_wrapper
2. hot/libs/hot/rowex/include/hot/rowex/HOTRowexNode.hpp posix_memalign is where memory allocation happens
