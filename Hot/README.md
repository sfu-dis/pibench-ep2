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
of every key is cleared before insertion (see offset in hot_wrapper.h)
2. Since hot only stores the pointer to entire record. The key/value struct 
needs to be in scope until benchmark finishes. The current approach is to 
create new record on heap upon insert/update. (see hot_wrapper.h)
3. Current version of HOT using ROWEX does not support delete operation
