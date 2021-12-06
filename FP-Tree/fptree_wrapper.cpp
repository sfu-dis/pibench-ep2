#include "fptree_wrapper.hpp"

size_t key_size_ = 0;

extern "C" tree_api* create_tree(const tree_options_t& opt)
{
#ifdef VAR_KEY
	key_size_ = opt.key_size;
	printf("Variable-length key with size: %lld\n", key_size_);
#endif
    return new fptree_wrapper();
}