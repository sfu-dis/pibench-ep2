#pragma once

#include "Epoch.h"
#include "Key.h"
#include "util.h"
#include <atomic>
#include <fstream>
#include <set>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace PART_ns {

int gethelpcount();

enum class NTypes : uint8_t {
    N4 = 1,
    N16 = 2,
    N48 = 3,
    N256 = 4,
    Leaf = 5,
    LeafArray = 6
};

class LeafArray;

class BaseNode {
  public:
    NTypes type;
    BaseNode(NTypes type_) : type(type_) {}
    virtual ~BaseNode() {}
};

class Leaf : public BaseNode {
  public:
    size_t key_len;
    size_t val_len;
//    uint64_t key;
// variable key
#ifdef KEY_INLINE
    char kv[0]; // append key and value
#else
    uint8_t *fkey;
    char *value;
#endif

  public:
    Leaf(const Key *k);
    Leaf(uint8_t *key_, size_t key_len_, char *value_,
         size_t val_len_); // used for update
    // use for test
    Leaf() : BaseNode(NTypes::Leaf) {}

    virtual ~Leaf() {}

    bool checkKey(const Key *k) const {
#ifdef KEY_INLINE
        if (key_len == k->getKeyLen() && memcmp(kv, k->fkey, key_len) == 0)
            return true;
        return false;
#else
        if (key_len == k->getKeyLen() && memcmp(fkey, k->fkey, key_len) == 0)
            return true;
        return false;
#endif
    }
    size_t getKeyLen() const { return key_len; }
    char *GetKey() {
#ifdef KEY_INLINE
        return kv;
#else
        return (char *)fkey;
#endif
    }
    char *GetValue() {
#ifdef KEY_INLINE
        return kv + key_len;
#else
        return value;
#endif
    }

    uint16_t getFingerPrint();

    // void graphviz_debug(std::ofstream &f);

} __attribute__((aligned(64)));

static constexpr uint32_t maxStoredPrefixLength = 4;
struct Prefix {
    uint32_t prefixCount = 0;
    uint8_t prefix[maxStoredPrefixLength];
};
static_assert(sizeof(Prefix) == 8, "Prefix should be 64 bit long");

class N : public BaseNode {
  protected:
    N(NTypes type, uint32_t level, const uint8_t *prefix, uint32_t prefixLength)
        : BaseNode(type), level(level) {
        type_version_lock_obsolete = new std::atomic<uint64_t>;
        type_version_lock_obsolete->store(0b100);
        recovery_latch.store(0, std::memory_order_seq_cst);
        setType(type);
        setPrefix(prefix, prefixLength, false);
    }

    N(NTypes type, uint32_t level, const Prefix &prefi)
        : BaseNode(type), prefix(prefi), level(level) {
        type_version_lock_obsolete = new std::atomic<uint64_t>;
        type_version_lock_obsolete->store(0b100);
        recovery_latch.store(0, std::memory_order_seq_cst);
        setType(type);
    }

    N(const N &) = delete;

    N(N &&) = delete;

    virtual ~N() {}

  public:
    // 3b type 59b version 1b lock 1b obsolete
    // obsolete means this node has been deleted
    std::atomic<uint64_t> *type_version_lock_obsolete;

    alignas(64) std::atomic<Prefix> prefix;
    const uint32_t level;
    uint16_t count = 0;
    uint16_t compactCount = 0;

    uint64_t generation_version = 0;
    std::atomic<uint64_t> recovery_latch;

    static const uint64_t dirty_bit = ((uint64_t)1 << 60);

    void setType(NTypes type);

    static uint64_t convertTypeToVersion(NTypes type);

    static inline N *setDirty(N *val) {
        return (N *)((uint64_t)val | dirty_bit);
    }
    static inline N *clearDirty(N *val) {
        return (N *)((uint64_t)val & (~dirty_bit));
    }
    static inline bool isDirty(N *val) { return (uint64_t)val & dirty_bit; }

    static void helpFlush(std::atomic<N *> *n);

    void set_generation();

    uint64_t get_generation();

    void check_generation();

    NTypes getType() const;

    uint32_t getLevel() const;

    static uint32_t getCount(N *node);

    void setCount(uint16_t count_, uint16_t compactCount_);

    bool isLocked(uint64_t version) const;

    void writeLockOrRestart(bool &needRestart);

    void lockVersionOrRestart(uint64_t &version, bool &needRestart);

    void writeUnlock();

    uint64_t getVersion() const;

    /**
     * returns true if node hasn't been changed in between
     */
    bool checkOrRestart(uint64_t startRead) const;
    bool readVersionOrRestart(uint64_t startRead) const;

    static bool isObsolete(uint64_t version);

    /**
     * can only be called when node is locked
     */
    void writeUnlockObsolete() { type_version_lock_obsolete->fetch_add(0b11); }

    static N *getChild(const uint8_t k, N *node);

    static void insertAndUnlock(N *node, N *parentNode, uint8_t keyParent,
                                uint8_t key, N *val, bool &needRestart);

    static void change(N *node, uint8_t key, N *val);

    static void removeAndUnlock(N *node, uint8_t key, N *parentNode,
                                uint8_t keyParent, bool &needRestart);

    Prefix getPrefi() const;

    void setPrefix(const uint8_t *prefix, uint32_t length, bool flush);

    void addPrefixBefore(N *node, uint8_t key);

    static Leaf *getLeaf(const N *n);

    static bool isLeaf(const N *n);

    static N *setLeaf(const Leaf *k);

    static LeafArray *getLeafArray(const N *n);

    static bool isLeafArray(const N *n);

    static N *setLeafArray(const LeafArray *la);

    static N *getAnyChild(N *n);

    static Leaf *getAnyChildTid(const N *n);

    static void deleteChildren(N *node);

    static void deleteNode(N *node);

    static std::tuple<N *, uint8_t> getSecondChild(N *node, const uint8_t k);

    template <typename curN, typename biggerN>
    static void tryInsertOrGrowAndUnlock(curN *n, N *parentNode,
                                         uint8_t keyParent, uint8_t key, N *val,
                                         NTypes type, bool &needRestart);

    template <typename curN>
    static void compactAndInsertAndUnlock(curN *n, N *parentNode,
                                          uint8_t keyParent, uint8_t key,
                                          N *val, NTypes type,
                                          bool &needRestart);

    template <typename curN, typename smallerN>
    static void removeAndShrink(curN *n, N *parentNode, uint8_t keyParent,
                                uint8_t key, NTypes type, bool &needRestart);

    static void getChildren(N *node, uint8_t start, uint8_t end,
                            std::tuple<uint8_t, N *> children[],
                            uint32_t &childrenCount);

    static void rebuild_node(N *node,
                             std::vector<std::pair<uint64_t, size_t>> &rs,
                             uint64_t start_addr, uint64_t end_addr,
                             int thread_id);

    // static void graphviz_debug(std::ofstream &f, N *node);

    // do insert without checking anything
    static void unchecked_insert(N *node, uint8_t key_byte, N *child,
                                 bool flush);

    static bool key_keylen_lt(char *a, const int alen, char *b, const int blen,
                              const int compare_level);

    static bool leaf_lt(Leaf *a, Leaf *b, int compare_level);
    static bool leaf_key_lt(Leaf *a, const Key *b, const int compare_level);
    static bool key_leaf_lt(const Key *a, Leaf *b, const int compare_level);
    static bool key_key_lt(const Key *a, const Key *b);

    static const int ZentryKeyShift = 48;
    static uint8_t getZentryKey(uintptr_t zentry);
    static N *getZentryPtr(uintptr_t zentry);
    static std::pair<uint8_t, N *> getZentryKeyPtr(uintptr_t zentry);
    static uintptr_t makeZentry(uint8_t key,N* node);

} __attribute__((aligned(64)));

} // namespace PART_ns
