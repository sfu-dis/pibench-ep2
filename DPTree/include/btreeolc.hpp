/*
 * BTreeOLC_child_layout.h - This file contains a modified version that
 *                           uses the key-value pair layout
 *
 * We use this to test whether child node layout will affect performance
 */
#pragma once

#include <immintrin.h>
#include <sched.h>
#include <atomic>
#include <cassert>
#include <cstring>
// std::pair
#include <utility>
#include <functional>

#define PREFETCH
#ifdef PREFETCH
    #include "nodepref.h"
    #define LEAF_LINE_NUM sizeof(BTreeLeaf<Key, Value>)/64
#endif

extern size_t key_size_;
extern thread_local uint64_t vkcmp_time;
#ifdef VAR_KEY

uint64_t rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

int vkcmp(char* a, char* b) {
/*
    auto n = key_size_;
    while(n--)
        if( *a != *b )
            return *a - *b;
        else
            a++,b++;
    return 0;
*/
#ifdef PROFILE
    auto start = rdtsc();
#endif
    auto res = memcmp(a, b, key_size_);
#ifdef PROFILE
    vkcmp_time += (rdtsc() - start);
    vkcmp_time /= 2;
#endif
    return res;
}
#endif

namespace btreeolc
{

void noop() {}
enum class PageType : uint8_t
{
    BTreeInner = 1,
    BTreeLeaf = 2
};

static void prefetch(char *ptr, size_t len) {
    if (ptr == nullptr)
        return;
    for (char *p = ptr; p < ptr + len; p += 64)
    {
        __builtin_prefetch(p);
    }
}

static const uint64_t pageSize = 512;
static const uint64_t leafPageSize = 512;

struct OptLock
{
    std::atomic<uint64_t> typeVersionLockObsolete{0b100};

    bool isLocked(uint64_t version) { return ((version & 0b10) == 0b10); }

    uint64_t readLockOrRestart(bool &needRestart)
    {
        uint64_t version;
        version = typeVersionLockObsolete.load(std::memory_order_relaxed);
        if (isLocked(version))
        {
            needRestart = true;
            _mm_pause();
        }
        return version;
    }

    void writeLockOrRestart(bool &needRestart)
    {
        uint64_t version;
        version = readLockOrRestart(needRestart);
        if (needRestart)
            return;

        upgradeToWriteLockOrRestart(version, needRestart);
        if (needRestart)
            return;
    }

    void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart)
    {
        if (typeVersionLockObsolete.compare_exchange_strong(version,
                                                            version + 0b10))
        {
            version = version + 0b10;
        }
        else
        {
            //_mm_pause();
            needRestart = true;
        }
    }

    void downgradeToReadLock(uint64_t & version) {
        version = typeVersionLockObsolete.fetch_add(0b10);
        version += 0b10;
    }

    void writeUnlock() { typeVersionLockObsolete.fetch_add(0b10); }

    bool isObsolete(uint64_t version) { return (version & 1) == 1; }

    void checkOrRestart(uint64_t startRead, bool &needRestart) const
    {
        readUnlockOrRestart(startRead, needRestart);
    }

    void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const
    {
        needRestart = (startRead != typeVersionLockObsolete.load());
    }

    void writeUnlockObsolete() { typeVersionLockObsolete.fetch_add(0b11); }
};

struct NodeBase : public OptLock
{
    PageType type;
    uint16_t count;
};

struct BTreeLeafBase : public NodeBase
{
    static const PageType typeMarker = PageType::BTreeLeaf;
};

template <class Key, class Payload>
struct BTreeLeaf : public BTreeLeafBase
{
    // This is the element type of the leaf node
    using KeyValueType = std::pair<Key, Payload>;
    static const uint64_t maxEntries =
        (leafPageSize - sizeof(NodeBase)) / (sizeof(KeyValueType));

    // This is the array that we perform search on
    KeyValueType data[maxEntries];

    struct BTreeLeaf<Key, Payload> *next;
    BTreeLeaf()
    {
        count = 0;
        type = typeMarker;
        next = nullptr;
    }

    bool isFull() { return count == maxEntries; };

    unsigned lowerBound(Key k)
    {
        unsigned lower = 0;
        unsigned upper = count;
    #ifdef VAR_KEY
        while (lower < upper && vkcmp((char*)data[lower].first, (char*)k) < 0)
    #else
        while (lower < upper && data[lower].first < k)
    #endif
            ++lower;
        // do
        // {
        //     unsigned mid = ((upper - lower) / 2) + lower;
        //     // This is the key at the pivot position
        //     const Key &middle_key = data[mid].first;

        //     if (k < middle_key)
        //     {
        //         upper = mid;
        //     }
        //     else if (k > middle_key)
        //     {
        //         lower = mid + 1;
        //     }
        //     else
        //     {
        //         return mid;
        //     }
        // } while (lower < upper);
        return lower;
    }

    void update(Key k, Payload p, unsigned pos) {
        assert(data[pos].first == k);
        data[pos].second = p;
    }

    Key max_key() {
        assert(count);
        return data[count - 1].first;
    }

    void insert_at(Key k, Payload p, unsigned pos) {
        assert(data[pos].first == k);
        if (count) {
            memmove(data + pos + 1, data + pos, sizeof(KeyValueType) * (count - pos));
            // memmove(payloads+pos+1,payloads+pos,sizeof(Payload)*(count-pos));
            data[pos].first = k;
            data[pos].second = p;
        } else {
            data[0].first = k;
            data[0].second = p;
        }
        ++count;
    }

    bool insert(Key k, Payload p)
    {
        assert(count < maxEntries);
        if (count)
        {
            unsigned pos = lowerBound(k);
        #ifdef VAR_KEY
            if ((pos < count) && vkcmp((char*)data[pos].first, (char*)k) == 0)
        #else
            if ((pos < count) && (data[pos].first == k))
        #endif
            {
                // Upsert
                data[pos].second = p;
                return true;
            }
            memmove(data + pos + 1, data + pos, sizeof(KeyValueType) * (count - pos));
            // memmove(payloads+pos+1,payloads+pos,sizeof(Payload)*(count-pos));
            data[pos].first = k;
            data[pos].second = p;
        }
        else
        {
            data[0].first = k;
            data[0].second = p;
        }
        count++;
        return false;
    }

    BTreeLeaf *split(Key &sep)
    {
        BTreeLeaf *newLeaf = new BTreeLeaf();
        newLeaf->count = count - (count / 2);
        count = count - newLeaf->count;
        newLeaf->next = next;
        next = newLeaf;
        memcpy(newLeaf->data, data + count, sizeof(KeyValueType) * newLeaf->count);
        // memcpy(newLeaf->payloads, payloads+count,
        // sizeof(Payload)*newLeaf->count);
        sep = data[count - 1].first;
        return newLeaf;
    }
};

struct BTreeInnerBase : public NodeBase
{
    static const PageType typeMarker = PageType::BTreeInner;
};


template <class Key>
struct BTreeInner : public BTreeInnerBase
{
    static const uint64_t maxEntries =
        (pageSize - sizeof(NodeBase)) / (sizeof(Key) + sizeof(NodeBase *));
    Key keys[maxEntries];
    NodeBase *children[maxEntries];

    BTreeInner()
    {
        count = 0;
        type = typeMarker;
    }

    bool isFull() { return count == (maxEntries - 1); };

    unsigned lowerBoundBF(Key k)
    {
        auto base = keys;
        unsigned n = count;
        while (n > 1)
        {
            const unsigned half = n / 2;
            base = (base[half] < k) ? (base + half) : base;
            n -= half;
        }
        return (*base < k) + base - keys;
    }

    unsigned lowerBound(Key k)
    {
        unsigned lower = 0;
        unsigned upper = count;
    #ifdef VAR_KEY
        while (lower < upper && vkcmp((char*)keys[lower], (char*)k) < 0)
    #else
        while (lower < upper && keys[lower] < k)
    #endif
            ++lower;
        return lower;
    }

    BTreeInner *split(Key &sep)
    {
        BTreeInner *newInner = new BTreeInner();
        newInner->count = count - (count / 2);
        count = count - newInner->count - 1;
        sep = keys[count];
        memcpy(newInner->keys, keys + count + 1,
               sizeof(Key) * (newInner->count + 1));
        memcpy(newInner->children, children + count + 1,
               sizeof(NodeBase *) * (newInner->count + 1));
        return newInner;
    }

    Key max_key() {
        assert(count);
        return keys[count - 1];
    }

    void insert(Key k, NodeBase *child)
    {
        assert(count < maxEntries - 1);
        unsigned pos = lowerBound(k);
        memmove(keys + pos + 1, keys + pos, sizeof(Key) * (count - pos + 1));
        memmove(children + pos + 1, children + pos,
                sizeof(NodeBase *) * (count - pos + 1));
        keys[pos] = k;
        children[pos] = child;
        std::swap(children[pos], children[pos + 1]);
        count++;
    }
};

template <class Key, class Value>
struct BTree
{
    std::atomic<NodeBase *> root;

    BTree() { root = new BTreeLeaf<Key, Value>(); }

    void makeRoot(Key k, NodeBase *leftChild, NodeBase *rightChild)
    {
        auto inner = new BTreeInner<Key>();
        inner->count = 1;
        inner->keys[0] = k;
        inner->children[0] = leftChild;
        inner->children[1] = rightChild;
        root = inner;
    }

    void destroy(NodeBase* node) {
        if (node == nullptr) return;
        if (node->type == PageType::BTreeInner) {
            auto inner = static_cast<BTreeInner<Key> *>(node);
            for (int i = 0; i < inner->count; ++i) {
                destroy(inner->children[i]);
            }
            delete inner;
        } else {
            auto leaf = static_cast<BTreeLeaf<Key, Value> *>(node);
            delete leaf;
        }
    }
    ~BTree() {
        destroy(root.load());
    }

    void yield(int count)
    {
        if (count > 3)
            sched_yield();
        else
            _mm_pause();
    }

    

    void insert(Key k, Value v, std::function<void()> insert_func=noop)
    {
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;


        // Current node
        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner)
        {
            auto inner = static_cast<BTreeInner<Key> *>(node);

            // Split eagerly if full
            if (inner->isFull())
            {
                // Lock
                if (parent)
                {
                    parent->upgradeToWriteLockOrRestart(versionParent, needRestart);
                    if (needRestart)
                        goto restart;
                }
                node->upgradeToWriteLockOrRestart(versionNode, needRestart);
                if (needRestart)
                {
                    if (parent)
                        parent->writeUnlock();
                    goto restart;
                }
                if (!parent && (node != root))
                { // there's a new parent
                    node->writeUnlock();
                    goto restart;
                }
                // Split
                Key sep;
                BTreeInner<Key> *newInner = inner->split(sep);
                if (parent)
                    parent->insert(sep, newInner);
                else
                    makeRoot(sep, inner, newInner);

                if (parent) {
                    parent->downgradeToReadLock(versionParent);
                }
            #ifdef VAR_KEY
                if (vkcmp((char*)k, (char*)sep) > 0)
            #else
                if (k > sep) 
            #endif
                {
                    inner->writeUnlock();
                    inner = newInner;
                    versionNode = newInner->readLockOrRestart(needRestart);
                    if (needRestart)
                        goto restart;
                } else {
                    node->downgradeToReadLock(versionNode);
                }
            }

            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            prefetch((char*)node, pageSize);
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        auto leaf = static_cast<BTreeLeaf<Key, Value> *>(node);

        // Split leaf if full
        if (leaf->count == leaf->maxEntries)
        {
            // Lock
            if (parent)
            {
                parent->upgradeToWriteLockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart)
            {
                if (parent)
                    parent->writeUnlock();
                goto restart;
            }
            if (!parent && (node != root))
            { // there's a new parent
                node->writeUnlock();
                goto restart;
            }
            // Split
            Key sep;
            BTreeLeaf<Key, Value> *newLeaf = leaf->split(sep);
        #ifdef VAR_KEY
            if (vkcmp((char*)k, (char*)sep) > 0)
        #else
            if (k > sep) 
        #endif
            {
                newLeaf->insert(k, v);
            } else {
                leaf->insert(k, v);
            }
            insert_func();

            if (parent)
                parent->insert(sep, newLeaf);
            else
                makeRoot(sep, leaf, newLeaf);
            // Unlock and restart
            node->writeUnlock();
            if (parent)
                parent->writeUnlock();
            return; // success
        }
        else
        {
            // only lock leaf node
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                {
                    node->writeUnlock();
                    goto restart;
                }
            }
            leaf->insert(k, v);
            insert_func();
            node->writeUnlock();
            return; // success
        }
    }

    void upsert(Key k, Value v, std::function<bool(Key k)> should_insert_func, std::function<void()> insert_func = noop)
    {
        int restartCount = 0;
        restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        // Current node
        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner)
        {
            auto inner = static_cast<BTreeInner<Key> *>(node);

            // Split eagerly if full
            if (inner->isFull())
            {
                // Lock
                if (parent)
                {
                    parent->upgradeToWriteLockOrRestart(versionParent, needRestart);
                    if (needRestart)
                        goto restart;
                }
                node->upgradeToWriteLockOrRestart(versionNode, needRestart);
                if (needRestart)
                {
                    if (parent)
                        parent->writeUnlock();
                    goto restart;
                }
                if (!parent && (node != root))
                { // there's a new parent
                    node->writeUnlock();
                    goto restart;
                }
                // Split
                Key sep;
                BTreeInner<Key> *newInner = inner->split(sep);
                if (parent)
                    parent->insert(sep, newInner);
                else
                    makeRoot(sep, inner, newInner);
                // Unlock and restart
                node->writeUnlock();
                if (parent)
                    parent->writeUnlock();
                goto restart;
            }

            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        auto leaf = static_cast<BTreeLeaf<Key, Value> *>(node);

        // Split leaf if full
        if (leaf->count == leaf->maxEntries)
        {
            // Lock
            if (parent)
            {
                parent->upgradeToWriteLockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart)
            {
                if (parent)
                    parent->writeUnlock();
                goto restart;
            }
            if (!parent && (node != root))
            { // there's a new parent
                node->writeUnlock();
                goto restart;
            }
            // Split
            Key sep;
            BTreeLeaf<Key, Value> *newLeaf = leaf->split(sep);
            if (parent)
                parent->insert(sep, newLeaf);
            else
                makeRoot(sep, leaf, newLeaf);
            // Unlock and restart
            node->writeUnlock();
            if (parent)
                parent->writeUnlock();
            goto restart;
        }
        else
        {
            // only lock leaf node
            node->upgradeToWriteLockOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                {
                    node->writeUnlock();
                    goto restart;
                }
            }
            unsigned pos = leaf->lowerBound(k);
            bool found = leaf->count == 0 ? false : leaf->data[pos].first == k;
            if (found) {
                leaf->update(k, v, pos);
                insert_func();
            } else {
                bool insert = should_insert_func(k);
                if (insert) {
                    leaf->insert_at(k, v, pos);
                    insert_func();
                }
            }
            node->writeUnlock();
            return; // success
        }
    }

    bool lookup(Key k, Value &result)
    {
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner)
        {
            auto inner = static_cast<BTreeInner<Key> *>(node);

            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

            node = inner->children[inner->lowerBound(k)];
            prefetch((char*)node, pageSize);
            inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }

        BTreeLeaf<Key, Value> *leaf = static_cast<BTreeLeaf<Key, Value> *>(node);
        unsigned pos = leaf->lowerBound(k);
        bool success = false;
    #ifdef VAR_KEY
        if ((pos < leaf->count) && vkcmp((char*)leaf->data[pos].first, (char*)k) == 0)
    #else
        if ((pos < leaf->count) && (leaf->data[pos].first == k))
    #endif
        {
            success = true;
            result = leaf->data[pos].second;
        }
        if (parent)
        {
            parent->readUnlockOrRestart(versionParent, needRestart);
            if (needRestart)
                goto restart;
        }
        node->readUnlockOrRestart(versionNode, needRestart);
        if (needRestart)
            goto restart;

        return success;
    }

    uint64_t scan(Key k, int range, typename BTreeLeaf<Key, Value>::KeyValueType *output)
    {
        int restartCount = 0;
    restart:
        if (restartCount++)
            yield(restartCount);
        bool needRestart = false;

        NodeBase *node = root;
        uint64_t versionNode = node->readLockOrRestart(needRestart);
        if (needRestart || (node != root))
            goto restart;

        // Parent of current node
        BTreeInner<Key> *parent = nullptr;
        uint64_t versionParent;

        while (node->type == PageType::BTreeInner)
        {
            auto inner = static_cast<BTreeInner<Key> *>(node);

            if (parent)
            {
                parent->readUnlockOrRestart(versionParent, needRestart);
                if (needRestart)
                    goto restart;
            }

            parent = inner;
            versionParent = versionNode;

	    int i = inner->lowerBound(k);
            #ifdef PREFETCH
                if (inner->children[i]->type != PageType::BTreeInner) {
                    LEAF_PREF(inner->children[i]);
                    if (i + 1 < node->count)
                        LEAF_PREF(inner->children[i + 1]);
                    if (i + 2 < node->count)
                        LEAF_PREF(inner->children[i + 2]);
                }                
            #endif
            node = inner->children[i];
            
	    inner->checkOrRestart(versionNode, needRestart);
            if (needRestart)
                goto restart;
            versionNode = node->readLockOrRestart(needRestart);
            if (needRestart)
                goto restart;
        }
	
        BTreeLeaf<Key, Value> *leaf = static_cast<BTreeLeaf<Key, Value> *>(node);
        unsigned pos = leaf->lowerBound(k);
        int count = 0;
        for (unsigned i = pos; i < leaf->count; i++)
        {
            if (count == range)
                break;
            output[count++] = leaf->data[i];
        }

        if (parent)
        {
            parent->readUnlockOrRestart(versionParent, needRestart);
            if (needRestart)
                goto restart;
        }
        node->readUnlockOrRestart(versionNode, needRestart);
        if (needRestart)
            goto restart;

        return count;
    }

    struct range_iterator
    {
        BTree<Key, Value> *btree;
        int count;
        int pos;
        static const uint64_t scan_unit_size = BTreeLeaf<Key, Value>::maxEntries;
        typename BTreeLeaf<Key, Value>::KeyValueType kvs[scan_unit_size];

        explicit range_iterator(BTree<Key, Value> *btree, const Key &startKey) : btree(btree), count(0), pos(0)
        {
            fill(startKey);
        }

        void fill(const Key &nextKey)
        {
            count = btree->scan(nextKey, scan_unit_size, kvs);
            pos = 0;
        }

        bool is_end() { return count == 0; }
        range_iterator &operator++()
        {
            if (++pos == count)
            {
                fill(kvs[pos - 1].first + 1);
            }
            return *this;
        }
        bool operator==(const range_iterator &rhs) = delete;
        bool operator!=(const range_iterator &rhs) = delete;
        Key key() { return kvs[pos].first; }
        Value value() { return kvs[pos].second; }
    };

    range_iterator lookup_range(const Key &startKey)
    {
        return range_iterator(this, startKey);
    }
    range_iterator end() {}

    struct unsafe_iterator
    {
        BTreeLeaf<Key, Value> *node;
        int pos;
        explicit unsafe_iterator() : node(nullptr), pos(-1) {}
        explicit unsafe_iterator(BTreeLeaf<Key, Value> *node, int pos = 0) : node(node), pos(pos) {}

        unsafe_iterator &operator++()
        {
            if (++pos == node->count)
            {
                node = node->next;
                pos = 0;
            }
            return *this;
        }
        bool operator==(const unsafe_iterator &rhs) { return node == rhs.node && pos == rhs.pos; }
        bool operator!=(const unsafe_iterator &rhs) { return !(*this == rhs); }
        int next_node(Key & last_key) {
            int ret = node->count - pos;
            last_key = node->data[node->count - 1].first;
            node = node->next;
            pos = 0;
            return ret;
        }
        Key key() { return node->data[pos].first; }
        Value value() { return node->data[pos].second; }
    };

    unsafe_iterator begin_unsafe()
    {
        NodeBase *node = root;
        while (node->type == PageType::BTreeInner)
        {
            auto inner = static_cast<BTreeInner<Key> *>(node);
            node = inner->children[0];
        }
        assert(node->type == PageType::BTreeLeaf);
        return unsafe_iterator((BTreeLeaf<Key, Value>*)node, 0);
    }

    unsafe_iterator end_unsafe()
    {
        return unsafe_iterator(nullptr, 0);
    }
};

} // namespace btreeolc
