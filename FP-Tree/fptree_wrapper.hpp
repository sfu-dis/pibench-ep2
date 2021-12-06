#ifndef __FPTREE_WRAPPER_HPP__
#define __FPTREE_WRAPPER_HPP__

#include "tree_api.hpp"
#include "fptree.h"

#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <libpmemobj.h>

// #define DEBUG_MSG

class fptree_wrapper : public tree_api
{
public:
    fptree_wrapper();
    virtual ~fptree_wrapper();
    
    virtual bool find(const char* key, size_t key_sz, char* value_out) override;
    virtual bool insert(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool update(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool remove(const char* key, size_t key_sz) override;
    virtual int scan(const char* key, size_t key_sz, int scan_sz, char*& values_out) override;

private:
    FPtree tree_;
};


fptree_wrapper::fptree_wrapper()
{
}

fptree_wrapper::~fptree_wrapper()
{
}

bool fptree_wrapper::find(const char* key, size_t key_sz, char* value_out)
{
    uint64_t value = tree_.find(*reinterpret_cast<uint64_t*>(const_cast<char*>(key)));
    if (value == 0)
    {
	#ifdef DEBUG_MSG
	    printf("Search key not found!\n");
	#endif
        return false;
    }
    memcpy(value_out, &value, sizeof(value));
    return true;
}


bool fptree_wrapper::insert(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
#ifdef VAR_KEY // key size > 8
	#ifdef PMEM
		TOID(char)* dst;
		POBJ_ZALLOC(pop, dst, char, key_sz);
		memcpy(pmemobj_direct(dst->oid), key, key_sz);
		pmemobj_persist(pop, dst, key_sz);
		KV kv = KV((uint64_t)pmemobj_direct(dst->oid), *reinterpret_cast<uint64_t*>(const_cast<char*>(value)));
	#else
		char* k = new char[key_sz];
		memcpy(k, key, key_sz);
		KV kv = KV((uint64_t)k, *reinterpret_cast<uint64_t*>(const_cast<char*>(value)));
	#endif
#else
    KV kv = KV(*reinterpret_cast<uint64_t*>(const_cast<char*>(key)), *reinterpret_cast<uint64_t*>(const_cast<char*>(value)));
#endif
    if (!tree_.insert(kv))
    {
	#ifdef DEBUG_MSG
	    printf("Insert failed\n");
	#endif
        return false;
    }
    return true;
}

bool fptree_wrapper::update(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
	// For now only support 8 bytes key and value (uint64_t)
    KV kv = KV(*reinterpret_cast<uint64_t*>(const_cast<char*>(key)), *reinterpret_cast<uint64_t*>(const_cast<char*>(value)));
    if (!tree_.update(kv))
    {
#ifdef DEBUG_MSG
        printf("Update failed\n");
#endif
        return false;
    }
    return true;
}

bool fptree_wrapper::remove(const char* key, size_t key_sz)
{
	// For now only support 8 bytes key and value (uint64_t)
    if (!tree_.deleteKey(*reinterpret_cast<uint64_t*>(const_cast<char*>(key))))
    {
#ifdef DEBUG_MSG
        printf("Remove failed\n");
#endif
        return false;
    }
    return true;
}

int fptree_wrapper::scan(const char* key, size_t key_sz, int scan_sz, char*& values_out)
{
	// For now only support 8 bytes key and value (uint64_t)
    constexpr size_t ONE_MB = 1ULL << 20;
    static thread_local char results[ONE_MB];
    int scanned = tree_.rangeScan(*reinterpret_cast<uint64_t*>(const_cast<char*>(key)), (uint64_t)scan_sz, results);
#ifdef DEBUG_MSG
    printf("%d records scanned\n", scanned);
#endif
    return scanned;
}

#endif