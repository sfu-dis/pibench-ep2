// #include <omp.h>
#include "tree_api.hpp"
#include "Tree.h"
#include "threadinfo.h"
#include <sys/types.h>
#include <algorithm>

// #define DEBUG_MSG

using namespace PART_ns;

class roart_wrapper : public tree_api
{
public:
  roart_wrapper();
  virtual ~roart_wrapper();

  virtual bool find(const char *key, size_t key_sz, char *value_out) override;
  virtual bool insert(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool update(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool remove(const char *key, size_t key_sz) override;
  virtual int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) override;

private:
  Tree roart; 
};

struct ThreadHelper
{
  ThreadHelper()
  {
    NVMMgr_ns::register_threadinfo();
  }
  ~ThreadHelper()
  {
    // NVMMgr_ns::unregister_threadinfo();
  }
};

roart_wrapper::roart_wrapper()
{
}

roart_wrapper::~roart_wrapper()
{
#ifdef MEMORY_FOOTPRINT
    printf("DRAM Footprint: %llu\n", dram_footprint);
    for (auto itr = dram_map.begin(); itr != dram_map.end(); itr++)
        printf("DRAM Block size: %llu    Count: %llu \n", itr->first, itr->second);
    printf("\n\nPMEM Footprint: %llu\n", pmem_footprint);
    for (auto itr = pmem_map.begin(); itr != pmem_map.end(); itr++)
        printf("PMEM Block size: %llu    Count: %llu \n", itr->first, itr->second);
#endif
}

bool roart_wrapper::find(const char *key, size_t key_sz, char *value_out)
{
  thread_local ThreadHelper t;
#ifdef KEY_INLINE
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, 0);
#else
  Key k;
  k.Init(const_cast<char*>(key), key_sz, const_cast<char*>(value_out), 8);
#endif
  auto leaf = roart.lookup(&k);

  if (leaf != nullptr)
  {
    memcpy(value_out, leaf->GetValue(), key_sz);
    return true;
  }
#ifdef DEBUG_MSG
  std::cout << "Key not found!\n";
#endif 
  return false;
}

bool roart_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local ThreadHelper t;
#ifdef KEY_INLINE
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, *reinterpret_cast<const uint64_t*>(value));
#else
  Key k;
  k.Init(const_cast<char*>(key), key_sz, const_cast<char*>(value), value_sz);
#endif
  Tree::OperationResults result = roart.insert(&k);
  if (result != Tree::OperationResults::Success)
  {
#ifdef DEBUG_MSG
    std::cout << "Insert failed!\n";
#endif
    return false;
  }
  return true;
}

bool roart_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local ThreadHelper t;
#ifdef KEY_INLINE
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, *reinterpret_cast<const uint64_t*>(value));
#else
  Key k;
  k.Init(const_cast<char*>(key), key_sz, const_cast<char*>(value), value_sz);
#endif
  Tree::OperationResults result = roart.update(&k);
  if (result != Tree::OperationResults::Success)
  {
#ifdef DEBUG_MSG
    std::cout << "Update failed!\n";
#endif
    return false;
  }
  return true;
}

bool roart_wrapper::remove(const char *key, size_t key_sz)
{
  thread_local ThreadHelper t;
#ifdef KEY_INLINE
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, 0);
#else
  Key k;
  k.Init(const_cast<char*>(key), key_sz, const_cast<char*>(key), key_sz);
#endif
  Tree::OperationResults result = roart.remove(&k);
  if (result != Tree::OperationResults::Success)
  {
#ifdef DEBUG_MSG
    std::cout << "Remove failed!\n";
#endif
    return false;
  }
  return true;
}

int roart_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
{
  thread_local ThreadHelper t;
  constexpr size_t ONE_MB = 1ULL << 20;
  static thread_local char results[ONE_MB];
  size_t scanned = 0;
  values_out = results;
  uint64_t max = (uint64_t)-1;
#ifdef KEY_INLINE
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, 0);
  Key end_k = Key(max, key_sz, 0);
#else
  Key k, end_k;
  k.Init(const_cast<char*>(key), key_sz, const_cast<char*>(key), key_sz);
  end_k.init((char*)&max, key_sz, (char*)&max, key_sz);
#endif
  roart.lookupRange(&k, &end_k, nullptr, (PART_ns::Leaf**)&results, scan_sz, scanned);
  auto arr = (PART_ns::Leaf**)&results;
  std::sort(arr, arr + scanned, [] (const PART_ns::Leaf* l1, const PART_ns::Leaf* l2) {
            return *(uint64_t*)(l1->kv) < *(uint64_t*)(l2->kv);
    });
#ifdef DEBUG_MSG
  if (scanned != 100)
    printf("%d records scanned.\n", scanned);
#endif
  
  return scanned;
}

