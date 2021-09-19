#include "roart_wrapper.h"
#include <chrono>

extern "C" tree_api *create_tree(const tree_options_t &opt)
{
  // long long nvmpool_size = opt.pool_size; 
  // if (const char *env_p = std::getenv("ROART_POOL_SIZE"))
  // {
  //   nvmpool_size = atoll(env_p);
  // }
  // if (nvmpool_size == 0)
  //   nvmpool_size = 64LL * 1024 * 1024 * 1024; // default 8GB

  // auto path_ptr = new std::string(opt.pool_path); // init method keeps reference to string
  // if (const char *env_p = std::getenv("ROART_POOL_PATH"))
  // {
  //   path_ptr->assign(env_p);
  // }
  // if (*path_ptr == "")
  //   path_ptr->assign("./pool"); // default pool path

  return new roart_wrapper();
}
