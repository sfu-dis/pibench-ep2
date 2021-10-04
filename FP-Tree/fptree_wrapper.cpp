#include "fptree_wrapper.hpp"

extern "C" tree_api* create_tree(const tree_options_t& opt)
{
#ifdef POOL
	worker_id = 0;
	size_t pool_size = MB*(uint64_t)1024*(uint64_t)64;
	the_thread_mempools.init(opt.num_threads, MB*1024*64, 64);
#endif
    return new fptree_wrapper();
}