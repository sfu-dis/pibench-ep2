#include "lbtree_wrapper.h"
#include <chrono>

constexpr const auto MEMPOOL_ALIGNMENT = 4096LL;

extern "C" tree_api *create_tree(const tree_options_t &opt)
{
#ifdef TSX_FAKE
  puts("Using Faked TSX Instructions! Multithreaded not valid");
#endif
  long long mempool_size = (long long)6 * (long long)1024 * (long long)MB; // 6 GB for inner nodes
  if (const char *env_p = std::getenv("MEMPOOL"))
  {
    mempool_size = atoll(env_p);
  }
  initUseful();
  worker_id = 0;
  worker_thread_num = opt.num_threads;
  long long nvmpool_size = (opt.pool_size == 0) ? (long long)14 * (long long)1024 * (long long)MB : opt.pool_size; // 14 for leaves
  if (const char *env_p = std::getenv("NVMSIZE"))
  {
    nvmpool_size = atoll(env_p);
  }
  auto path_ptr = new std::string(opt.pool_path); // init method keeps reference to string
  if (const char *env_p = std::getenv("NVMPOOL"))
  {
    path_ptr->assign(env_p);
  }
  if (*path_ptr == "")
    path_ptr->assign("./pool");
  std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
  the_thread_mempools.init(opt.num_threads, mempool_size, MEMPOOL_ALIGNMENT);
  std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
  std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  printf("mempools init time: %lld ms \n", ms.count());

  start = std::chrono::steady_clock::now();
  the_thread_nvmpools.init(opt.num_threads, path_ptr->c_str(), nvmpool_size);
  end = std::chrono::steady_clock::now();
  ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  printf("nvmpools init time: %lld ms \n", ms.count());
  char *nvm_addr = (char *)nvmpool_alloc(256);
  nvmLogInit(opt.num_threads);

  printf("MemPool Size: %lld, NVMPool Size: %lld. Using file %s\n", mempool_size, nvmpool_size, path_ptr->c_str());
  return new lbtree_wrapper(nvm_addr, false);
}
