#include "masstree_wrapper.h"

volatile uint64_t globalepoch = 1;
volatile uint64_t active_epoch = 1;
volatile bool recovering = false;
kvtimestamp_t initial_timestamp;


extern "C" tree_api *create_tree(const tree_options_t &opt)
{
  return new masstree_wrapper();
}
