#include <omp.h>
#include "tree_api.hpp"
#include "masstree-beta/masstree.hh"
#include "masstree-beta/masstree_struct.hh"
#include "masstree-beta/config.h"
#include "masstree-beta/query_masstree.hh"
#include "masstree-beta/masstree_tcursor.hh"
#include "masstree-beta/masstree_stats.hh"
#include "masstree-beta/masstree_scan.hh"
#include "masstree-beta/masstree_remove.hh"
#include "masstree-beta/masstree_print.hh"
#include "masstree-beta/masstree_get.hh"
#include "masstree-beta/masstree_insert.hh"
#include "masstree-beta/kvthread.hh"

using namespace Masstree;

class masstree_wrapper : public tree_api
{
public:

  masstree_wrapper();
  virtual ~masstree_wrapper();

  virtual bool find(const char *key, size_t key_sz, char *value_out) override;
  virtual bool insert(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool update(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool remove(const char *key, size_t key_sz) override;
  virtual int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) override;
  void swap_endian(uint64_t &i);

static __thread typename default_query_table_params::threadinfo_type * ti;

private:
  default_table mt;
};

__thread typename default_query_table_params::threadinfo_type * masstree_wrapper::ti = nullptr;
static default_query_table_params::threadinfo_type * main_threadinfo = nullptr;

struct ThreadHelper
{
  ThreadHelper()
  {
    if (masstree_wrapper::ti == nullptr)
    {
      int id = omp_get_thread_num();
      if (id == 0)
        masstree_wrapper::ti = main_threadinfo;
      else
        masstree_wrapper::ti = threadinfo::make(threadinfo::TI_PROCESS, id);
    }
  }
  ~ThreadHelper()
  {
  }
};


masstree_wrapper::masstree_wrapper()
{
  ti = threadinfo::make(threadinfo::TI_MAIN, -1);
  main_threadinfo = ti;
  mt.initialize(*ti);
  srand(time(NULL));
}

masstree_wrapper::~masstree_wrapper()
{
}

bool masstree_wrapper::find(const char *key, size_t key_sz, char *value_out)
{
  thread_local ThreadHelper t;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  swap_endian(k);
  Masstree::default_table::unlocked_cursor_type lp(mt.table(), Str((const char*)&k, key_sz));
  bool found = lp.find_unlocked(*ti);
  if (found)
    memcpy(value_out, lp.value()->col(0).s, key_sz);
  else
    std::cout << "Search Key not found!\n";
  return found;
}

bool masstree_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local ThreadHelper t;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  swap_endian(k);
  Masstree::default_table::cursor_type lp(mt.table(), Str((const char*)&k, key_sz));
  bool found = lp.find_insert(*ti);
  if (found)
    std::cout << "Insert Key already exists!\n";
  else
    lp.value() = row_type::create1(Str(value, value_sz), 2, *ti);
  lp.finish(1, *ti);
  return !found;
}

bool masstree_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local ThreadHelper t;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  swap_endian(k);
  Masstree::default_table::cursor_type lp(mt.table(), Str((const char*)&k, key_sz));
  bool found = lp.find_insert(*ti);
  if (!found)
    std::cout << "Update Key does not exist!\n";
  else
    lp.value()->deallocate_rcu(*ti);
  lp.value() = row_type::create1(Str(value, value_sz), 2, *ti);
  lp.finish(1, *ti);
  return found;
}

bool masstree_wrapper::remove(const char *key, size_t key_sz)
{
  thread_local ThreadHelper t;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  swap_endian(k);
  Masstree::default_table::cursor_type lp(mt.table(), Str((const char*)&k, key_sz));
  bool found = lp.find_locked(*ti);
  if (!found)
    std::cout << "Delete Key does not exist!\n";
  lp.finish(-1, *ti);
  return found;
}

 struct scanner {
  char *values;
  int range;
  int key_len;
  int val_len;

  scanner(char *values, int range, int key_len, int val_len)
    : values(values), range(range), key_len(key_len), val_len(val_len) {
  }

  template <typename SS2, typename K2>
  void visit_leaf(const SS2&, const K2&, threadinfo&) {}
  bool visit_value(Str key, const row_type* row, threadinfo&) {
      memcpy(values, key.s, key_len);
      values += key_len;
      memcpy(values, row->col(0).s, val_len);
      values += val_len;
      --range;
      return range > 0;
  }
};

int masstree_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
{
  constexpr size_t ONE_MB = 1ULL << 20;
  static thread_local char results[ONE_MB];
  thread_local ThreadHelper t;
  int scanned = 0;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  swap_endian(k);

  values_out = results;
  scanner s(values_out, scan_sz, key_sz, key_sz);

  scanned = mt.table().scan(Str((const char*)&k, key_sz), true, s, *ti);

  return scanned;
}

inline void masstree_wrapper::swap_endian(uint64_t &i) {
    // masstree treats input as big-endian
    i = __bswap_64(i);
}
