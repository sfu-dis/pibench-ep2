CC=g++
# CC=clang++

# Flag for debugging runs
# CFLAGS=-O0 -g -std=c++11 -pthread -mrtm -msse4.1 -mavx2

# Flag for test runs
CFLAGS=-g -std=c++17 -pthread -mrtm -msse4.1 -mavx2 -Wall -O3

#CFLAGS=-g -std=c++17 -pthread -msse4.1 -mavx2 -Wall -O3 -fsanitize=undefined,implicit-conversion,nullability,float-divide-by-zero,unsigned-integer-overflow,bounds,bool

INCLUDE=-I./hot/libs/hot/rowex/include/hot/rowex -I./hot/libs/hot/commons/include/hot/commons
LIB=

COMMON_DEPENDS= ./tree_api.hpp 
COMMON_SOURCES= 

# -----------------------------------------------------------------------------
TARGETS=hot_wrapper

all: ${TARGETS}

# -----------------------------------------------------------------------------
hot_wrapper: lbtree-src/hot_wrapper.h lbtree-src/hot_wrapper.cpp ${COMMON_DEPENDS}
	${CC} -o libhot_wrapper.so ${CFLAGS} -fPIC -shared ${INCLUDE} lbtree-src/hot_wrapper.cpp ${COMMON_SOURCES} ${LIB}
# lbtree: lbtree-src/lbtree.h lbtree-src/lbtree.cc ${COMMON_DEPENDS}
# 	${CC} -o $@ ${CFLAGS} ${INCLUDE} lbtree-src/lbtree.cc ${COMMON_SOURCES} ${LIB}
# -----------------------------------------------------------------------------
clean:
	-rm -rf a.out core *.s ${TARGETS} libhot_wrapper.so
