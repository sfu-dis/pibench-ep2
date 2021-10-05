CC=g++
# CC=clang++

# Flag for debugging runs
# CFLAGS=-O0 -g -std=c++11 -pthread -mrtm -msse4.1 -mavx2

# Flag for test runs
CFLAGS=-g -std=c++17 -pthread -mrtm -msse4.1 -mavx2 -Wall -O3 -ltbb 

#CFLAGS=-g -std=c++17 -pthread -msse4.1 -mavx2 -Wall -O3 -fsanitize=undefined,implicit-conversion,nullability,float-divide-by-zero,unsigned-integer-overflow,bounds,bool

INCLUDE=-I./ART -I./nvm_mgr
LIB=-lpmem

COMMON_DEPENDS= ./tree_api.hpp ./ART/LeafArray.cpp ./ART/N.cpp ./ART/N4.cpp ./ART/N16.cpp ./ART/N48.cpp ./ART/N256.cpp ./ART/Tree.cpp ./nvm_mgr/Epoch.h ./nvm_mgr/Epoch.cpp ./nvm_mgr/EpochGuard.h ./nvm_mgr/nvm_mgr.h ./nvm_mgr/nvm_mgr.cpp ./nvm_mgr/pmalloc_wrap.h ./nvm_mgr/threadinfo.h ./nvm_mgr/threadinfo.cpp ./nvm_mgr/util.h ./mempool.cc ./mempool.h
COMMON_SOURCES= ./ART/LeafArray.cpp ./ART/N.cpp ./ART/N4.cpp ./ART/N16.cpp ./ART/N48.cpp ./ART/N256.cpp ./ART/Tree.cpp ./nvm_mgr/Epoch.cpp ./nvm_mgr/nvm_mgr.cpp ./nvm_mgr/threadinfo.cpp ./mempool.cc

# -----------------------------------------------------------------------------
TARGETS=roart_wrapper

#wbtree fptree

all: ${TARGETS}

# -----------------------------------------------------------------------------
roart_wrapper: roart_wrapper.h roart_wrapper.cpp ${COMMON_DEPENDS}
	${CC} -o libroart_wrapper.so ${CFLAGS} -fPIC -shared ${INCLUDE} roart_wrapper.cpp ${COMMON_SOURCES} ${LIB}

# -----------------------------------------------------------------------------
clean:
	-rm -rf a.out core *.s ${TARGETS} libroart_wrapper.so
