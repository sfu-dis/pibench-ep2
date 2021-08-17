#include "hot_wrapper.h"


extern "C" tree_api *create_tree(const tree_options_t &opt)
{
  return new hot_wrapper();
}
