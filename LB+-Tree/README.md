# LB+-Tree PiBench Wrapper

## Installation
1. Configure allocation scheme in mempool.h (#define PMEM, #define POOL)
2. Set mempool and nvmpool size in lbtree_wrapper.cpp (Optional and Only if running in PMEM)
3. (Optional) Set the weak isolation fix type in lbtree.h (NONTEMP or UNLOCK_AFTER, the later is the default)
```
make
```

## Important information
This repo contains source code from https://github.com/schencoding/lbtree

commit 92f7304feef4c62a90fe0b2b6e29bd427649d38e

Modifications: 
1. Applied fix for integer overflow bug, see https://github.com/schencoding/lbtree/pull/5/commits/7e92605f8a1316db1208e5723de07fb97d802b82
2. Uncommented backoff in insert to reduce aborts (lbtree.cc)
3. Modified mempool to support PMDK & DRAM (malloc) allocation scheme (mempool.h .cc)
4. Added range scan (tree.h, lbtree.h, lbtree.cc)
5. Added option to free node instead of reuse (only workds when POOL is not defined, mempool.h)
6. Applied two types of weak isolation fix, one is from https://github.com/schencoding/lbtree/pull/6. 
The other approach (update the lock bit after clwb/sfence) from the author's discussion https://github.com/schencoding/lbtree/issues/2
7. Update is implemented as lookup followed by CAS. Thus only safe to run concurrently when workload is pure updates.
