CC=g++
# CC=clang++

# Flag for debugging runs
# CFLAGS=-O0 -g -std=c++11 -pthread -mrtm -msse4.1 -mavx2

# Flag for test runs
CFLAGS=-g -ljemalloc -std=c++17 -pthread -mrtm -msse4.1 -mavx2 -Wall -O3

#CFLAGS=-g -std=c++17 -pthread -msse4.1 -mavx2 -Wall -O3 -fsanitize=undefined,implicit-conversion,nullability,float-divide-by-zero,unsigned-integer-overflow,bounds,bool

INCLUDE=-I./common
LIB=-lpmem -lpmemobj -ltbb

COMMON_DEPENDS= ./common/tree_api.hpp ./common/lbtree.h ./common/lbtree.cc ./common/tree.h ./common/tree.cc ./common/keyinput.h ./common/mempool.h ./common/mempool.cc ./common/nodepref.h ./common/nvm-common.h ./common/nvm-common.cc ./common/performance.h
COMMON_SOURCES= ./common/lbtree.cc ./common/tree.cc ./common/mempool.cc ./common/nvm-common.cc

# -----------------------------------------------------------------------------
TARGETS=lbtree_wrapper

#wbtree fptree

all: ${TARGETS}

# -----------------------------------------------------------------------------
lbtree_wrapper: lbtree-src/lbtree_wrapper.h lbtree-src/lbtree_wrapper.cpp ${COMMON_DEPENDS}
	${CC} -o liblbtree_wrapper.so ${CFLAGS} -fPIC -shared ${INCLUDE} lbtree-src/lbtree_wrapper.cpp ${COMMON_SOURCES} ${LIB}
# lbtree: lbtree-src/lbtree.h lbtree-src/lbtree.cc ${COMMON_DEPENDS}
# 	${CC} -o $@ ${CFLAGS} ${INCLUDE} lbtree-src/lbtree.cc ${COMMON_SOURCES} ${LIB}
# -----------------------------------------------------------------------------
clean:
	-rm -rf a.out core *.s ${TARGETS} liblbtree_wrapper.so
