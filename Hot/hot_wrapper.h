#include <vector>
#include <omp.h>
#include <hot/rowex/HOTRowex.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>
#include "tree_api.hpp"
#include <atomic>
#include <unordered_map>


// #define DEBUG_MSG

std::atomic<uint64_t> dram_fp(0);
std::unordered_map<uint64_t, uint64_t> map;

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

struct InsertHelper
{
  InsertHelper()
  {
    int id = omp_get_thread_num();
    // if (id == 0)
    //   records.reserve(150000000u); // load thread needs more memory to hold 100M index + 50M insert ops
    // else
    //   records.reserve(50000000u);
  }
  ~InsertHelper() {}
};

hot_wrapper::hot_wrapper()
{
}

hot_wrapper::~hot_wrapper()
{
  printf("DRAM Footprint: %llu\n", dram_fp.load());
  for( const auto& [key, value] : u ) {
    printf("Chunk size: %llu    Request time: %llu \n", key, value);
  }
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
  thread_local InsertHelper i;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index
  uint64_t v = *reinterpret_cast<const uint64_t*>(value);
  // records.emplace_back(k, v);
  KV* record = new KV(k, v);
#ifdef MEMORY_FOOTPRINT
  dram_fp += sizeof(KV);
#endif
  bool ret = hot.insert(record);
#ifdef DEBUG_MSG
  if (!ret)
    printf("Insert failed, Key %llu  Value %llu \n", k, v);
#endif
  return ret;
}

bool hot_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz)
{
  thread_local InsertHelper i;
  uint64_t k = *reinterpret_cast<const uint64_t*>(key) & offset; // at most 63 bits can be embedded into the index
  uint64_t v = *reinterpret_cast<const uint64_t*>(value);
  // records.emplace_back(k, v);
  KV* record = new KV(k, v);
#ifdef MEMORY_FOOTPRINT
  dram_fp += sizeof(KV);
#endif
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
