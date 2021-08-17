# HOT PiBench Wrapper

PiBench wrapper for HOT.

## Installation
git clone https://github.com/speedskater/hot.git
cp CMakeLists.txt hot/
cd hot
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make


## Important Information
1. Hot can only embed 63 bits key in index, therefore the most significant bit 
of every key is cleared before insertion (see offset in wrapper.h)
2. Since hot only stores the pointer to entire record. The key/value struct 
needs to be in scope until benchmark finishes. The current approach is to 
reserve a large vector for insert & update threads (see InsertHelper)
Loading thread can hold up to 150M records and other insert/update thread can
hold 50M records. Tune these parameters to suit your benchmark.
3. Current version of HOT using ROWEX does not support delete operation
4. The Scan operation in hot may not be thread-safe

## This repo contains souce code from https://github.com/speedskater/hot.git
The following modifications are made in order to compile the pibench wrapper
1. Modified CMakeLists.txt in hot/ to include libhot_wrapper
