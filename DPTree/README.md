# DPTree PiBench Wrapper

## Installation
1. Set # of merge workers in dptree_wrapper.cpp (optional, default equals to # of worker threads)

```
mkdir build && cd build && cmake ..
make
```

For long-key support
```
mkdir build && cd build && cmake -DVar_Key=1 ..
make
```

## Important information
Source code originated from https://github.com/zxjcarrot/DPTree-code.git

commit 027d9122e6fa831a8ab48fa3a7d7a566adda85ae

Modifications were applied:
1. Changed const key_type &key to const key_type &k in concur_dptree.hpp line 2438 (compiler error)
2. Added scan that returns both key and value (lookup_range only returns value)
3. Modified CMakeLists.txt
4. Added PMDK allocator support in util.h .cpp
