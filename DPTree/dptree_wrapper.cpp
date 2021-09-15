#include "dptree_wrapper.hpp"

int parallel_merge_worker_num = 1;

extern "C" tree_api* create_tree(const tree_options_t& opt)
{
	parallel_merge_worker_num = opt.num_threads;
    return new dptree_wrapper();
}