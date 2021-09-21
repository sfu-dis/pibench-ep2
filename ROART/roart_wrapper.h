// #include <omp.h>
#include "tree_api.hpp"
#include "Tree.h"
#include "threadinfo.h"
#include <sys/types.h>

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
  inline Key createKey(const char *key, size_t key_sz, char *value_out);
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
}

inline Key createKey(const char *key, size_t key_sz, char *value)
{
  #ifdef KEY_INLINE
    return Key(*reinterpret_cast<const uint64_t*>(key), key_sz, *reinterpret_cast<const uint64_t*>(value));
  #else
    Key k;
    k.Init(key, key_sz, value, 8);
    return k;
  #endif
}

bool roart_wrapper::find(const char *key, size_t key_sz, char *value_out)
{
  thread_local ThreadHelper t;
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, 0);
  auto leaf = roart.lookup(&k);

  if (leaf != nullptr)
  {
    memcpy(value_out, leaf->GetValue(), key_sz);
    return true;
  }
  std::cout << "Key not found!\n";
  return false;
}

bool roart_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
#ifdef KEY_INLINE
  printf("Key inline!\n");
#endif
  thread_local ThreadHelper t;
  Key* k = new Key(*reinterpret_cast<const uint64_t*>(key), key_sz, *reinterpret_cast<const uint64_t*>(value));
  Tree::OperationResults result = roart.insert(k);
  if (result != Tree::OperationResults::Success)
  {
    std::cout << "Insert failed!\n";
    return false;
  }
  return true;
}

bool roart_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local ThreadHelper t;
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, *reinterpret_cast<const uint64_t*>(value));
  Tree::OperationResults result = roart.update(&k);
  if (result != Tree::OperationResults::Success)
  {
    std::cout << "Update failed!\n";
    return false;
  }
  return true;
}

bool roart_wrapper::remove(const char *key, size_t key_sz)
{
  thread_local ThreadHelper t;
  Key k = Key(*reinterpret_cast<const uint64_t*>(key), key_sz, 0);
  Tree::OperationResults result = roart.remove(&k);
  if (result != Tree::OperationResults::Success)
  {
    // std::cout << "Remove failed!\n";
    return false;
  }
  return true;
}

int roart_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
{
  thread_local ThreadHelper t;
  //FIXME
  int scanned = 0;
  // Iterator is not supported by roart.
  return scanned;
}

