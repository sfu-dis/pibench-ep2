#ifndef __DPTREE_WRAPPER_HPP__
#define __DPTREE_WRAPPER_HPP__

#include "tree_api.hpp"
#include <btreeolc.hpp>
#include <concur_dptree.hpp>

#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <libpmemobj.h>

// #define DEBUG_MSG

extern int parallel_merge_worker_num;

class dptree_wrapper : public tree_api
{
public:
    dptree_wrapper();
    virtual ~dptree_wrapper();
    
    virtual bool find(const char* key, size_t key_sz, char* value_out) override;
    virtual bool insert(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool update(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool remove(const char* key, size_t key_sz) override;
    virtual int scan(const char* key, size_t key_sz, int scan_sz, char*& values_out) override;

private:
    dptree::concur_dptree<uint64_t, uint64_t> dptree;
};

struct KV {
    uint64_t k;
    uint64_t v;
};

dptree_wrapper::dptree_wrapper()
{
}

dptree_wrapper::~dptree_wrapper()
{
}

bool dptree_wrapper::find(const char* key, size_t key_sz, char* value_out)
{
    uint64_t v, k = *reinterpret_cast<uint64_t*>(const_cast<char*>(key));
    if (!dptree.lookup(k, v))
    {
#ifdef DEBUG_MSG
        printf("Key not found!\n");
#endif
        return false;
    }
    v = v >> 1;
    memcpy(value_out, &v, sizeof(v));
    return true;
}


bool dptree_wrapper::insert(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
    uint64_t k = *reinterpret_cast<uint64_t*>(const_cast<char*>(key));
    uint64_t v = *reinterpret_cast<uint64_t*>(const_cast<char*>(value));
    dptree.insert(k, v);
    return true;
}

bool dptree_wrapper::update(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
    uint64_t k = *reinterpret_cast<uint64_t*>(const_cast<char*>(key));
    uint64_t v = *reinterpret_cast<uint64_t*>(const_cast<char*>(value));
    dptree.upsert(k, v);
    return true;
}

bool dptree_wrapper::remove(const char* key, size_t key_sz)
{
    // current dptree code does not implement delete
    return true;
}

int dptree_wrapper::scan(const char* key, size_t key_sz, int scan_sz, char*& values_out)
{
    uint64_t k = *reinterpret_cast<uint64_t*>(const_cast<char*>(key));
    static thread_local std::vector<uint64_t> v(scan_sz*2);
    v.clear();
    dptree.scan(k, scan_sz, v);
    values_out = (char*)v.data();
    scan_sz = v.size() / 2;
#ifdef DEBUG_MSG
    if (scan_sz != 100)
        printf("%d records scanned!\n", scan_sz);
#endif
    return scan_sz;
}

#endif