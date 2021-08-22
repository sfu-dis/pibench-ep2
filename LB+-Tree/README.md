# LB+-Tree PiBench Wrapper

## Installation


## Important information
This repo contains source code from https://github.com/schencoding/lbtree
commit 92f7304feef4c62a90fe0b2b6e29bd427649d38e
Some modifications are made to compile dram wrapper

1. Applied fix for integer overflow bug, see https://github.com/schencoding/lbtree/pull/5/commits/7e92605f8a1316db1208e5723de07fb97d802b82
2. Uncommented backoff in insert to avoid hanging threads (line 665-666 in lbtree.cc)
3. Modified mempool
4. Added range scan (tree.h, lbtree.h)

