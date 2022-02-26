#include "dptree_wrapper.hpp"

int parallel_merge_worker_num = 1;
size_t key_size_ = 0;
size_t pool_size_ = ((size_t)(1024 * 1024 * 10) * 1024);
const char *pool_path_;

extern "C" tree_api* create_tree(const tree_options_t& opt)
{
#ifdef VAR_KEY
	key_size_ = opt.key_size;
	if (key_size_ < 8)
		key_size_ = 8;
	if (key_size_ > 128)
	{
		printf("Variable-length key with max length 128!\n");
		exit(1);
	}
	printf("Variable-length key with size: %lld\n", key_size_);
#endif
	parallel_merge_worker_num = opt.num_threads;

    auto path_ptr = new std::string(opt.pool_path);
    if (*path_ptr != "")
    	pool_path_ = (*path_ptr).c_str();
    else
		pool_path_ = "./pool";
	
    if (opt.pool_size != 0)
    	pool_size_ = opt.pool_size;

    printf("PMEM Pool Path: %s\n", pool_path_);
    printf("PMEM Pool size: %lld\n", pool_size_);

    return new dptree_wrapper();
}