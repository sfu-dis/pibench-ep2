#include <vector>
#include <omp.h>
#include <hot/rowex/HOTRowex.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>
#include "tree_api.hpp"

// #define DEBUG_MSG

struct KV {
  uint64_t key;
  uint64_t value;
  KV(uint64_t k, uint64_t v): key(k), value(v){}
};

class hot_wrapper : public tree_api
{
public:
  template <typename ValueType = KV*>
  class KeyExtractor{
  public:
    inline uint64_t operator()(ValueType const& kv) const{
      return kv->key;
    }
  };

  hot_wrapper();
  virtual ~hot_wrapper();

  virtual bool find(const char *key, size_t key_sz, char *value_out) override;
  virtual bool insert(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool update(const char *key, size_t key_sz, const char *value, size_t value_sz) override;
  virtual bool remove(const char *key, size_t key_sz) override;
  virtual int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) override;

private:
  hot::rowex::HOTRowex<KV*, KeyExtractor> hot;
};

// static thread_local std::vector<KV> records;

static const uint64_t offset = (1ull << 63ull) - 1ull;

hot_wrapper::hot_wrapper()
{
}

hot_wrapper::~hot_wrapper()
{
}

bool hot_wrapper::find(const char *key, size_t key_sz, char *value_out)
{
  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index
  idx::contenthelpers::OptionalValue<KV*> ret = hot.lookup(k);
  if (ret.mIsValid)
    memcpy(value_out, &ret.mValue->value, key_sz);
  else
  {
#ifdef DEBUG_MSG
    printf("Key not found %llu \n", k);
#endif
  }
  return ret.mIsValid;
}

bool hot_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index
  uint64_t v = *reinterpret_cast<const uint64_t*>(value);
  // records.emplace_back(k, v);
  KV* record = new KV(k, v);
  bool ret = hot.insert(record);
#ifdef DEBUG_MSG
  if (!ret)
    printf("Insert failed, Key %llu  Value %llu \n", k, v);
#endif
  return ret;
}

bool hot_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index
  uint64_t v = *reinterpret_cast<const uint64_t*>(value);
  // records.emplace_back(k, v);
  KV* record = new KV(k, v);
  idx::contenthelpers::OptionalValue<KV*> ret = hot.upsert(record);
#ifdef DEBUG_MSG
  if (!ret.mIsValid)
    printf("Update failed, Key %llu  Value %llu \n", k, v);
#endif
  return ret.mIsValid;
}

bool hot_wrapper::remove(const char *key, size_t key_sz)
{
  // Current version of HOT using a ROWEX synchronization strategy does not support delete
  return true;
}

int hot_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out)
{
  constexpr size_t ONE_MB = 1ULL << 20;
  static thread_local char results[ONE_MB];
  values_out = results;
  char* cur_address = results;
  int scanned = 0;

  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index

  hot::rowex::HOTRowex<KV*, KeyExtractor>::const_iterator iterator = hot.lower_bound(k);

  while (scanned < scan_sz && iterator != hot.end())
  {
    memcpy(cur_address, *iterator, sizeof(KV));
    ++scanned;
    ++iterator;
  }
#ifdef DEBUG_MSG
  if (scanned != 100)
    printf("Scanned: %d\n", scanned);
#endif
  return scanned;
}
