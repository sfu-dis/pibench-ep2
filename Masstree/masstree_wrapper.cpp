#include "masstree_wrapper.h"

volatile uint64_t globalepoch = 1;
volatile uint64_t active_epoch = 1;
volatile bool recovering = false;
kvtimestamp_t initial_timestamp;


extern "C" tree_api *create_tree(const tree_options_t &opt)
{
#ifdef POOL
  worker_id = 0;
  the_thread_mempools.init(opt.num_threads, pool_size, 64);
#endif
  return new masstree_wrapper();
}
