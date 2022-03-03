#include "roart_wrapper.h"
#include <chrono>

size_t pool_size_ = ((size_t)(1024 * 1024 * 40) * 1024);
const char *pool_path_;

extern "C" tree_api *create_tree(const tree_options_t &opt)
{
#ifndef DRAM_MODE
  auto path_ptr = new std::string(opt.pool_path);
  if (*path_ptr != "")
    pool_path_ = (*path_ptr).c_str();
  else
    pool_path_ = "./pool";

  if (opt.pool_size != 0)
    pool_size_ = opt.pool_size;

  printf("PMEM Pool Path: %s\n", pool_path_);
  printf("PMEM Pool size: %lld\n", pool_size_);
#endif
  return new roart_wrapper();
}
