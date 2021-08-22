/**
 * @file lbtree.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 *
 * @section DESCRIPTION
 *
 *
 * The class implements the LB+-Tree. 
 *
 * Non-leaf nodes are in DRAM.  They are normal B+-Tree nodes.
 * Leaf nodes are in NVM.
 */

#ifndef _LBTREE_H
#define _LBTREE_H

/* ---------------------------------------------------------------------- */

#include "tree.h"
/* ---------------------------------------------------------------------- */

/* In a non-leaf, there are NON_LEAF_KEY_NUM keys and NON_LEAF_KEY_NUM+1
 * child pointers.
 */
#define NON_LEAF_KEY_NUM (NONLEAF_SIZE / (KEY_SIZE + POINTER_SIZE) - 1)

/* In a leaf, there are 16B header, 14x16B entries, 2x8B sibling pointers.
 */
#if LEAF_SIZE != 256
#error "LB+-Tree requires leaf node size to be 256B."
#endif

#define LEAF_KEY_NUM (14)

/* ---------------------------------------------------------------------- */
/**
 * Pointer8B defines a class that can be assigned to either bnode or bleaf.
 */
class Pointer8B
{
public:
    unsigned long long value; /* 8B to contain a pointer */

public:
    Pointer8B() {}

    Pointer8B(const void *ptr)
    {
        value = (unsigned long long)ptr;
    }

    Pointer8B(const Pointer8B &p)
    {
        value = p.value;
    }

    Pointer8B &operator=(const void *ptr)
    {
        value = (unsigned long long)ptr;
        return *this;
    }
    Pointer8B &operator=(const Pointer8B &p)
    {
        value = p.value;
        return *this;
    }

    bool operator==(const void *ptr)
    {
        bool result = (value == (unsigned long long)ptr);
        return result;
    }
    bool operator==(const Pointer8B &p)
    {
        bool result = (value == p.value);
        return result;
    }

    operator void *() { return (void *)value; }
    operator char *() { return (char *)value; }
    operator struct bnode *() { return (struct bnode *)value; }
    operator struct bleaf *() { return (struct bleaf *)value; }
    operator unsigned long long() { return value; }

    bool isNull(void) { return (value == 0); }

    void print(void) { printf("%llx\n", value); }

}; // Pointer8B

/**
 *  An IdxEntry consists of a key and a pointer.
 */
typedef struct IdxEntry
{
    key_type k;
    Pointer8B ch;
} IdxEntry;

/**
 *  bnodeMeta: the 8B meta data in Non-leaf node
 */
typedef struct bnodeMeta
{             /* 8B */
    int lock; /* lock bit for concurrency control */
    int num;  /* number of keys */
} bnodeMeta;

/**
 * bnode: non-leaf node
 *
 *   metadata (i.e. k(0))
 *
 *      k(1) .. k(NON_LEAF_KEY_NUM)
 *
 *   ch(0), ch(1) .. ch(NON_LEAF_KEY_NUM)
 */
class bnode
{
public:
    IdxEntry ent[NON_LEAF_KEY_NUM + 1];

public:
    key_type &k(int idx) { return ent[idx].k; }
    Pointer8B &ch(int idx) { return ent[idx].ch; }

    char *chEndAddr(int idx)
    {
        return (char *)&(ent[idx].ch) + sizeof(Pointer8B) - 1;
    }

    int &num(void) { return ((bnodeMeta *)&(ent[0].k))->num; }
    int &lock(void) { return ((bnodeMeta *)&(ent[0].k))->lock; }
}; // bnode

typedef union bleafMeta
{
    unsigned long long word8B[2];
    struct
    {
        uint16_t bitmap : 14;
        uint16_t lock : 1;
        uint16_t alt : 1;
        unsigned char fgpt[LEAF_KEY_NUM]; /* fingerprints */
    } v;
} bleafMeta;

/**
 * bleaf: leaf node
 *
 * We guarantee that each leaf must have >=1 key.
 */
class bleaf
{
public:
    uint16_t bitmap : 14;
    uint16_t lock : 1;
    uint16_t alt : 1;
    unsigned char fgpt[LEAF_KEY_NUM]; /* fingerprints */
    IdxEntry ent[LEAF_KEY_NUM];
    bleaf *next[2];

public:
    key_type &k(int idx) { return ent[idx].k; }
    Pointer8B &ch(int idx) { return ent[idx].ch; }

    int num() { return countBit(bitmap); }
    bleaf *nextSibling() { return next[alt]; }

    bool isFull(void) { return (bitmap == 0x3fff); }

    void setBothWords(bleafMeta *m)
    {
        bleafMeta *my_meta = (bleafMeta *)this;
        my_meta->word8B[1] = m->word8B[1];
        my_meta->word8B[0] = m->word8B[0];
    }

    void setWord0(bleafMeta *m)
    {
        bleafMeta *my_meta = (bleafMeta *)this;
        my_meta->word8B[0] = m->word8B[0];
    }

}; // bleaf

/* ---------------------------------------------------------------------- */

class treeMeta
{
public:
    int root_level; // leaf: level 0, parent of leaf: level 1
    Pointer8B tree_root;
    bleaf **first_leaf; // on NVM

public:
    treeMeta(void *nvm_address, bool recover = false)
    {
        root_level = 0;
        tree_root = NULL;
        first_leaf = (bleaf **)nvm_address;

        if (!recover)
            setFirstLeaf(NULL);
    }

    void setFirstLeaf(bleaf *leaf)
    {
        *first_leaf = leaf;
        clwb(first_leaf);
        sfence();
    }

}; // treeMeta

/* ---------------------------------------------------------------------- */

class lbtree : public tree
{
public: // root and level
    treeMeta *tree_meta;

public:
    lbtree(void *nvm_address, bool recover = false)
    {
        tree_meta = new treeMeta(nvm_address, recover);
        if (!tree_meta)
        {
            perror("new");
            exit(1);
        }
    }

    ~lbtree()
    {
        delete tree_meta;
    }

private:
    int bulkloadSubtree(keyInput *input, int start_key, int num_key,
                        float bfill, int target_level,
                        Pointer8B pfirst[], int n_nodes[]);

    int bulkloadToptree(Pointer8B ptrs[], key_type keys[], int num_key,
                        float bfill, int cur_level, int target_level,
                        Pointer8B pfirst[], int n_nodes[]);

    void getMinMaxKey(bleaf *p, key_type &min_key, key_type &max_key);

    void getKeyPtrLevel(Pointer8B pnode, int pnode_level, key_type left_key,
                        int target_level, Pointer8B ptrs[], key_type keys[], int &num_nodes,
                        bool free_above_level_nodes);

    // sort pos[start] ... pos[end] (inclusively)
    void qsortBleaf(bleaf *p, int start, int end, int pos[]);

public:
    // bulkload a tree and return the root level
    // use multiple threads to do the bulkloading
    int bulkload(int keynum, keyInput *input, float bfill);

    void randomize(Pointer8B pnode, int level);
    void randomize()
    {
        srand48(12345678);
        randomize(tree_meta->tree_root, tree_meta->root_level);
    }

    // given a search key, perform the search operation
    // return the leaf node pointer and the position within leaf node
    void *lookup(key_type key, int *pos);

    void *get_recptr(void *p, int pos)
    {
        return ((bleaf *)p)->ch(pos);
    }

    // insert (key, ptr)
    void insert(key_type key, void *ptr);

    // delete key
    void del(key_type key);

    // Range scan -- Author: Lu Baotong
    int range_scan_by_size(const key_type& key,  uint32_t to_scan, char* result);
    int range_scan_in_one_leaf(bleaf *lp, const key_type& key, uint32_t to_scan, std::pair<key_type, void*>* result);
    int add_to_sorted_result(std::pair<key_type, void*>* result, std::pair<key_type, void*>* new_record, int total_size, int cur_idx);

private:
    void print(Pointer8B pnode, int level);
    void check(Pointer8B pnode, int level, key_type &start, key_type &end, bleaf *&ptr);
    void checkFirstLeaf(void);

public:
    void print()
    {
        print(tree_meta->tree_root, tree_meta->root_level);
    }

    void check(key_type *start, key_type *end)
    {
        bleaf *ptr = NULL;
        check(tree_meta->tree_root, tree_meta->root_level, *start, *end, ptr);
        checkFirstLeaf();
    }

    int level() { return tree_meta->root_level; }

}; // lbtree

// Continue to add new record to result array, and shift the cur_idx
// cur_idx points to a position that are empty
// Return the cur_idx
inline int lbtree::add_to_sorted_result(std::pair<key_type, void*>* result, std::pair<key_type, void*>* new_record, int total_size, int cur_idx){
    if (cur_idx >= total_size)
    {
      if (result[total_size - 1].first < new_record->first)
      {
        return cur_idx;
      }
      cur_idx = total_size - 1; // Remove the last element
    }

    // Start the insertion sort
    int j = cur_idx - 1;
    while((j >= 0) && (result[j].first > new_record->first)){
      result[j + 1] = result[j];
      --j;
    }

    result[j + 1] = *new_record;
    ++cur_idx;
    return cur_idx;
  }

// Range scan in one node -- Author: Lu Baotong
int lbtree::range_scan_in_one_leaf(bleaf *lp, const key_type& key, uint32_t to_scan, std::pair<key_type, void*>* result){
    unsigned int mask = (unsigned int)(lp->bitmap);
    int cur_idx = 0;
    std::pair<key_type, void*> new_record;

    while (mask) {
        int jj = bitScan(mask)-1;  // next candidate
        if (lp->k(jj) >= key) { // found
            new_record.first = lp->k(jj);
            new_record.second = lp->ch(jj);
            // Add KV to the result array and matain its sort order
            cur_idx = add_to_sorted_result(result, &new_record, to_scan, cur_idx);
        }
        mask &= ~(0x1<<jj);  // remove this bit
    } // end while

    /*
    for(int i = 0; i < cur_idx; i++){
        std::cout << "key " << i << " = " << result[i].first << std::endl;
    }*/

    return cur_idx;
}

// Range query, first get the lock of node, and then atomically access each node in range
int lbtree::range_scan_by_size(const key_type& key,  uint32_t to_scan, char* my_result)
{   
    std::pair<key_type, void*> *result = reinterpret_cast<std::pair<key_type, void*> *>(my_result);
    bnode *p;
    bleaf *lp;
    int i,t,m,b;
    int result_idx;
    
    unsigned char key_hash= hashcode1B(key);
    int ret_pos;
    
Again1:
    // 1. RTM begin
    result_idx = 0; // The idx of result array
    if(_xbegin() != _XBEGIN_STARTED) goto Again1;

    // 2. search nonleaf nodes
    p = tree_meta->tree_root;
    
    for (i=tree_meta->root_level; i>0; i--) {
        
        // prefetch the entire node
        NODE_PREF(p);

        // if the lock bit is set, abort
        if (p->lock()) {_xabort(1); goto Again1;}
        
        // binary search to narrow down to at most 8 entries
        b=1; t=p->num();
        while (b+7<=t) {
            m=(b+t) >>1;
            if (key > p->k(m)) b=m+1;
            else if (key < p->k(m)) t = m-1;
            else {p=p->ch(m); goto inner_done;}
        }
        
        // sequential search (which is slightly faster now)
        for (; b<=t; b++)
            if (key < p->k(b)) break;
        p = p->ch(b-1);
        
    inner_done: ;
    }
    
    // 3. search leaf node
    lp= (bleaf *)p;

    // prefetch the entire node
    LEAF_PREF (lp);

    // 4. real range query
    auto remaining_scan = to_scan;
    auto cur_result = result;
    while((remaining_scan != 0) && (lp != nullptr)){
        // if the lock bit is set, abort
        if (lp->lock) {_xabort(2); goto Again1;}
        auto cur_scan = range_scan_in_one_leaf(lp, key, remaining_scan, cur_result);
        remaining_scan = remaining_scan - cur_scan;
        cur_result = cur_result + cur_scan;
        lp = lp->next[0]; // BT(FIX ME), the next pointer is not always this
    }

    // 5. RTM commit
    _xend();

    return (to_scan - remaining_scan);
}

void initUseful();
/* ---------------------------------------------------------------------- */
#endif /* _LBTREE_H */
