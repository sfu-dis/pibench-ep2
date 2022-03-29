CC=g++
# CC=clang++

# Flag for debugging runs
# CFLAGS=-O0 -g -std=c++11 -pthread -mrtm -msse4.1 -mavx2

# Flag for test runs
override CFLAGS += -g -ljemalloc -DNDEBUG -DMUTEX -m64 -g -D_REENTRANT -fno-strict-aliasing -DINTEL -Wno-unused-value -Wno-format -fPIC -m64 -O3

#CFLAGS=-g -std=c++17 -pthread -msse4.1 -mavx2 -Wall -O3 -fsanitize=undefined,implicit-conversion,nullability,float-divide-by-zero,unsigned-integer-overflow,bounds,bool

INCLUDE=-I./common
LIB=-lpmem -lpmemobj -lpthread

COMMON_DEPENDS= ./fptree.h ./utils.h ./fptree.cpp
COMMON_SOURCES= ./fptree.cpp

# -----------------------------------------------------------------------------
TARGETS=fptree_wrapper

#wbtree fptree

all: ${TARGETS}

# -----------------------------------------------------------------------------
lbtree_wrapper: fptree_wrapper.hpp fptree_wrapper.cpp ${COMMON_DEPENDS}
	${CC} -o libfptree_wrapper.so ${CFLAGS} -fPIC -shared ${INCLUDE} fptree_wrapper.cpp ${COMMON_SOURCES} ${LIB}
# lbtree: lbtree-src/lbtree.h lbtree-src/lbtree.cc ${COMMON_DEPENDS}
# 	${CC} -o $@ ${CFLAGS} ${INCLUDE} lbtree-src/lbtree.cc ${COMMON_SOURCES} ${LIB}
# -----------------------------------------------------------------------------
clean:
	-rm -rf a.out core *.s ${TARGETS} libfptree_wrapper.so
