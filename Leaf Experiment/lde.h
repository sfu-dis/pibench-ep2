#include <stdio.h>

#include <libpmemobj.h>




#define LEAF_SIZE 14 


#define FINGERPRINT

#define UNSORTED_LEAF

#define NUM_NEXT_PTR 2

typedef uint64_t keytype;
typedef uint64_t valtype;
typedef uint16_t bmptype;
typedef unsigned char fgpttype;


struct KV 
{
	keytype k;
	valtype v;
};

class leafnode
{
public:

#ifdef UNSORTED_LEAF
	bmptype bitmap;
#endif

#ifdef FINGERPRINT
    fgpttype fgpt[LEAF_SIZE]; /* fingerprints */
#endif
    KV ent[LEAF_SIZE];


    leafnode * next[NUM_NEXT_PTR];
}