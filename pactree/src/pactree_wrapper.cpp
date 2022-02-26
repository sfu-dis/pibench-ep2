#include "pactree_wrapper.h"

size_t pool_size_ = ((size_t)(1024 * 1024 * 4) * 1024);
std::string *pool_dir_;

extern "C" tree_api* create_tree(const tree_options_t& opt) {
	auto path_ptr = new std::string(opt.pool_path);
    if (*path_ptr != "")
    	pool_dir_ = path_ptr;
    else
		pool_dir_ = new std::string("./");
	
    if (opt.pool_size != 0)
    	pool_size_ = opt.pool_size;

    printf("PMEM Pool Dir: %s\n", pool_dir_->c_str());
    printf("PMEM Pool size: %lld\n", pool_size_);
	return new pactree_wrapper();
}
