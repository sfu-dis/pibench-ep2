#include "tree_api.hpp"
#include "utree.h"
#include <sys/types.h>

// #define DEBUG_MSG

class utree_wrapper : public tree_api
{
public:
  utree_wrapper();
  virtual ~utree_wrapper();

  virtual bool find(const char *key, size_t key_sz, char *value_out) override;
  virtual bool insert(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool update(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool remove(const char *key, size_t key_sz) override;
  virtual int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) override;

private:
  btree utree; 
};

utree_wrapper::utree_wrapper()
{
}

utree_wrapper::~utree_wrapper()
{
}

bool utree_wrapper::find(const char *key, size_t key_sz, char *value_out)
{
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  auto value = utree.search(k);
  if (value != NULL)
  {
    memcpy(value_out, &value, key_sz);
    return true;
  }
#ifdef DEBUG_MSG
  std::cout << "Key not found!\n";
#endif
  return false;
}

bool utree_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  uint64_t k = *reinterpret_cast<const uint64_t*>(key);
  utree.insert(k, (char* )value);
  return true;
}

bool utree_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  utree.update(*reinterpret_cast<const uint64_t*>(key), (char* )value); // utree insert --> upsert 
  return true;
}

bool utree_wrapper::remove(const char *key, size_t key_sz)
{
  utree.new_remove(*reinterpret_cast<const uint64_t*>(key));
  return true;
}

int utree_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
{
  constexpr size_t ONE_MB = 1ULL << 20;
  static thread_local char results[ONE_MB];
  int scanned = utree.scan(*reinterpret_cast<uint64_t*>(const_cast<char*>(key)), scan_sz, results);
#ifdef DEBUG_MSG
  if (scanned != 100)
    printf("%d records scanned\n", scanned);
#endif
  return scanned;
}
