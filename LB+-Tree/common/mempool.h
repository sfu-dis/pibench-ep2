/**
 * @file mempool.h
 * @author  Shimin Chen <shimin.chen@gmail.com>, Jihang Liu, Leying Chen
 * @version 1.0
 *
 * @section LICENSE
 *
 * TBD
 *
 * @section DESCRIPTION
 *
 * The mempool class implements a memory pool for experiments purpose.  It will
 * manage a contiguous region of memory (DRAM or NVM).  Then it will serve memory
 * allocation requests from this memory pool.  Freed btree nodes will be
 * appended into a linked list for future reuse.
 *
 * threadMemPools allocates contiguous region of DRAM using memalign from OS. 
 * The memory is divided into segments.  A mempool instance is used to manage
 * a segment.  Each worker thread has its own mempool instance.
 * A thread local variable worker_id is used to locate the current mempool
 * used by the calling thread.
 *
 * threadNVMPools allocates and maps contiguous NVM.  Otherwise, it is similar
 * to threadMemPools.  Each worker thread has its own mempool instance for
 * NVM allocation and free.
 *
 * During crash recovery, the B+-Tree object will take the first 4KB.
 * We can find the first nonleaf node.  Then, the NVM can be scanned to 
 * determine the allocated nodes and unused nodes.
 *
 */
#ifndef _BTREE_MEM_POOL_H
#define _BTREE_MEM_POOL_H

/* -------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>

/* -------------------------------------------------------------- */
/* NVMPOOL_REAL: use pmdk to map NVM
 * undefined:    use memalign to allocate memory to simulate NVM
 */
#include <libpmemobj.h>

#define PMEM // comment this out to use DRAM as NVM (DRAM version LB+-Tree)
//#define POOL // comment this out to use malloc and PMDK if #define PMEM

// #define VAR_KEY

#if defined(PMEM) && defined(VAR_KEY)
   PMEMobjpool * pop_;
#endif

#define ENTRY_MOVING
#define PREFETCH

#define FREE_ON_DELETE // comment this out to reuse freed leaves

struct dummy { // dummy class for using PMDK
char* arr[32];
};

extern uint64_t class_id;

#ifdef PMEM
   #define NVMPOOL_REAL
   #include <libpmem.h>
   #ifndef POOL
      POBJ_LAYOUT_BEGIN(LBtree);
      POBJ_LAYOUT_TOID(LBtree, dummy);
      POBJ_LAYOUT_END(LBtree);

      #ifdef VAR_KEY
         POBJ_LAYOUT_BEGIN(Char);
         POBJ_LAYOUT_TOID(Char, char);
         POBJ_LAYOUT_END(Char);
      #endif
   #endif
#endif

/* -------------------------------------------------------------- */

#ifndef MB
#define MB (1024LL * 1024LL)
#endif


/**
 * mempool: allocate memory using malloc-like calls then manage the memory
 *          by itself
 */
class mempool
{
private:
   long long mempool_align;
   long long mempool_size;
   char *mempool_start;
   char *mempool_cur;
   char *mempool_end;
   char *mempool_free_node;
   const char *mempool_name;
   PMEMobjpool * pop;

public:
   // ---
   // Initialization and basics
   // ---

   /**
   * constructor to set all internal states to null
   */
   mempool(PMEMobjpool * p = NULL)
   {
      mempool_start = mempool_cur = mempool_end = NULL;
      mempool_free_node = NULL;
      pop = p;
   }

   /**
   * destructor does nothing
   */
   ~mempool() {}

   /**
   * initialize the memory pool.
   *
   * @param start the memory pool start address
   * @param size  the memory pool size in bytes
   * @param align alignment in bytes
   */
   void init(char *start, long long size, long long align, const char *name)
   {
      mempool_align = align;
      mempool_size = size;
      mempool_start = start;

      mempool_cur = mempool_start;
      mempool_end = mempool_start + size;
      mempool_free_node = NULL;

      mempool_name = name;
   }

   /**
   * obtain the starting address of the memory pool
   */
   char *get_base() { return mempool_start; }

   /**
   * print all the parameters and addresses of the memory pool 
   */
   void print_params(FILE *fp = stdout)
   {
      fprintf(fp, "%s\n", mempool_name);
      fprintf(fp, "mempool_align=%lld\n", mempool_align);
      fprintf(fp, "mempool_size=%lld\n", mempool_size);
      fprintf(fp, "mempool_start=%p\n", mempool_start);
      fprintf(fp, "mempool_cur=%p\n", mempool_cur);
      fprintf(fp, "mempool_end=%p\n", mempool_end);
      fprintf(fp, "mempool_free_node=%p\n\n", mempool_free_node);
   }

   void print_usage()
   {
      long long used = (mempool_cur - mempool_start);
      long long ff = 0;
      for (char *p = mempool_free_node; p; p = *((char **)p))
         ff++;

      printf("%s: total %.1lfMB, use %.1lfMB, among which %lld free nodes\n",
             mempool_name, ((double)mempool_size) / MB, ((double)used) / MB, ff);
   }

public:
   // ---
   // allocation and free
   // ---

   /**
   * allocate a piece of memory from the memory pool
   *
   * @param size the allocation size
   * @return the starting address of the allocated memory
   * 
   * Note: the memory pool will be allocated from the end.
   * Make sure the size is aligned.  The code will not check this.
   */
   void *alloc(unsigned long long size)
   {
   #ifdef POOL
      if (mempool_cur)
      {
         if (mempool_cur + size <= mempool_end)
         {
            char *p;
            p = mempool_cur;
            mempool_cur += size;
            return (void *)p;
         }
         fprintf(stderr, "%s alloc - run out of memory!\n", mempool_name);
         abort();
      }
   #elif defined(PMEM)
      if (pop)
      {
         thread_local pobj_action act;
         auto x = POBJ_XRESERVE_NEW(pop, dummy, &act, POBJ_CLASS_ID(class_id));
         D_RW(x)->arr[0] = NULL;
         D_RW(x)->arr[31] = NULL;
         (((unsigned long long)(D_RW(x)->arr)) & (~(unsigned long long)(64 - 1)));
         (((unsigned long long)(D_RW(x)->arr+32)) & (~(unsigned long long)(64 - 1)));
         return pmemobj_direct(x.oid);
      }
   #endif
      return new (std::align_val_t(256)) char[size];
   }

   /**
   * free memory that is previously allocated using alloc()
   *
   * @param p  the starting address of the memory
   */
   void free(void *p)
   {
   }

   /**
   * allocate a btree node
   * 
   * @param size  the size of the node
   * @return  the starting address of the allocated node
   *
   *  size should be always the same, and it should be larger than
   *  sizeof(void *) ||| won't check this
   */
   void *alloc_node(int size)
   {
      if (mempool_free_node)
      {
         char *p;
         p = mempool_free_node;
         mempool_free_node = *((char **)p);
         return (void *)p;
      }
      else
      {
         return alloc(size);
      }
   }

   /**
   * free a btree node and append it into a linked list for future alloc_node
   *
   * @param p btree node to free
   */
   void free_node(void *p)
   {
  	#if defined(FREE_ON_DELETE) && !defined(POOL)
      #ifdef PMEM
   		if (pop)
   		{
   			TOID(dummy) n = pmemobj_oid(p);
     			POBJ_FREE(&n);
     			return;
   		}
   	#endif
   		delete p;
    #else
      *((char **)p) = mempool_free_node;
      mempool_free_node = (char *)p;
    #endif
   }

   /**
   * print the nodes on the free linked list
   */
   void print_free_nodes()
   {
      char *p = mempool_free_node;

      printf("%s free nodes:\n", mempool_name);
      while (p != NULL)
      {
         printf("%p -> ", p);
         p = *((char **)p);
      }
      printf("nil\n\n");
   }

}; // mempool

/* -------------------------------------------------------------- */

/**
 * threadMemPools allocates a pool of memory from OS.  It divides the
 * memory into num workers's sub pools.  Then it uses mempool to manage
 * the sub pool.
 */
class threadMemPools
{
public:
   mempool *tm_pools; /* pools[0..num_workers-1] */
   int tm_num_workers;

   char *tm_buf;      /* start address of allocated memory */
   long long tm_size; /* tm_buf size */

public:
   /** 
    * Constructor sets all fields to empty
    */
   threadMemPools()
   {
      tm_pools = NULL;
      tm_num_workers = 0;
      tm_buf = NULL;
      tm_size = 0;
   }

   /**
    * Destructor frees memory if exists.
    */
   ~threadMemPools()
   {
      if (tm_buf)
      {
         free(tm_buf);
         tm_buf = NULL;
      }
      if (tm_pools)
      {
         delete[] tm_pools;
         tm_pools = NULL;
      }
   }

   /**
   * Initialize the memory pools
   * allocate memory from OS and initialize all per-worker mempool.
   *
   * @param num_workers  number of parallel worker threads 
   * @param size         the total memory pool size in bytes
   * @param align        alignment in bytes
   */
   void init(int num_workers, long long size = 20 * MB, long long align = 4096);

   /**
   * print information about memory pool for debugging purpose.
   */
   void print(void);

   /**
   * print memory usage for each thread pool
   */
   void print_usage(void);

}; // threadMemPools

/* -------------------------------------------------------------- */

/**
 * threadNVMPools allocates a contiguous region of NVM then divides it
 * among threads' individual mempools.
 */
class threadNVMPools
{
public:
   mempool *tm_pools; /* pools[0..num_workers-1] */
   int tm_num_workers;

   char *tm_buf;      /* start address of allocated memory */
   long long tm_size; /* tm_buf size */

   const char *tn_nvm_file;

public:
   /**
   * constructor
   */
   threadNVMPools()
   {
      tm_pools = NULL;
      tm_num_workers = 0;
      tm_buf = NULL;
      tm_size = 0;
      tn_nvm_file = NULL;
   }

   /**
   * destructor
   */
   ~threadNVMPools();

   /**
   * Initialize the NVM memory pools
   * allocate NVM and initialize all per-worker mempool.
   *
   * @param num_workers  number of parallel worker threads 
   * @param num_file     the nvm file name to map
   * @param size         the total memory pool size in bytes, must be multiple of 4KB
   */
   void init(int num_workers, const char *nvm_file, long long size = 20 * MB);

   void print(void);

   /**
   * print NVM usage for each thread pool
   */
   void print_usage(void);

}; // threadNVMPools

/* -------------------------------------------------------------- */
/* (1) call the_thread_mempool.init(...)
 * (2) In each thread that wants to use the mempool,
 *     set worker_id 
 */
extern thread_local int worker_id; /* in Thread Local Storage */

extern threadMemPools the_thread_mempools;
extern threadNVMPools the_thread_nvmpools;

#define the_mempool (the_thread_mempools.tm_pools[worker_id])
#define mempool_alloc the_mempool.alloc
// #define mempool_free the_mempool.free
#define mempool_alloc_node the_mempool.alloc_node
#define mempool_free_node the_mempool.free_node

#define the_nvmpool (the_thread_nvmpools.tm_pools[worker_id])
#define nvmpool_alloc the_nvmpool.alloc
// #define nvmpool_free the_nvmpool.free
#define nvmpool_alloc_node the_nvmpool.alloc_node
#define nvmpool_free_node the_nvmpool.free_node
/* -------------------------------------------------------------- */
#endif /* _BTREE_MEM_POOL_H */
