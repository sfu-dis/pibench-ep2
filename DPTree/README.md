# DPTree PiBench Wrapper

## Installation
1. 

## Important information
This repo contains source code from https://github.com/zxjcarrot/DPTree-code.git
commit 027d9122e6fa831a8ab48fa3a7d7a566adda85ae
Some modifications are made to compile dptree wrapper

1. Changed const key_type &key to const key_type &k in concur_dptree.hpp line 2438
2. Added scan that returns both key and value (lookup_range only returns value)
3. Modified CMakeLists.txt