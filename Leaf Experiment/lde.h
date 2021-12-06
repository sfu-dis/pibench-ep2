#include <stdio.h>

#include <libpmemobj.h>


#define PMEM

#define LEAF_SIZE 14 

#define FINGERPRINT
#ifdef FINGERPRINT
	#define SIMD
#endif

#define UNSORTED_LEAF
#ifdef UNSORTED_LEAF
	#define INDIR_ARR
#else
	#define LINEAR_SEARCH
#endif

#define NUM_NEXT_PTR 1

#define ALIGNMENT 256

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
	#ifdef INDIR_ARR
		unsigned char iarr[LEAF_SIZE];
	#endif
#else
	unsigned char numKey; // change to unsigned short or int if LEAF_SIZE exceeds 255
#endif

#ifdef FINGERPRINT
    fgpttype fgpt[LEAF_SIZE]; /* fingerprints */
#endif

    KV ent[LEAF_SIZE];

    leafnode * next[NUM_NEXT_PTR];

public:
	valtype find(keytype k); // return valid v if k is found

	bool insert(KV kv);	// return true if kv successfully inserted into leaf, false if kv.k already exists

	bool update(KV kv); // return true if kv successfully updated, false if kv.k does not exist

	bool del(keytype k); // return false if k not found

	int scan(KV* addr, int size); // return number of KV scanned, scanned KVs will be copied to addr
}
