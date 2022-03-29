#ifndef __FPTREE_WRAPPER_HPP__
#define __FPTREE_WRAPPER_HPP__

#include "tree_api.hpp"
#include "fptree.cpp"

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
    fptree_t * tree_;
};

thread_local char k[128];

fptree_wrapper::fptree_wrapper()
{
    tree_ = fptree_create();
    openPmemobjPool(pool_path_);
}

fptree_wrapper::~fptree_wrapper()
{
    fptree_destroy(tree_);
}

bool fptree_wrapper::find(const char* key, size_t key_sz, char* value_out)
{
    bool found = fptree_get(tree_, *reinterpret_cast<uint64_t*>(const_cast<char*>(key)), (void*)(value_out));
    if (!found) 
        printf("Key %llu not found!", *reinterpret_cast<uint64_t*>(const_cast<char*>(key)));
    return found;
}


bool fptree_wrapper::insert(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
    if (!fptree_put(tree_, *reinterpret_cast<uint64_t*>(const_cast<char*>(key)), *reinterpret_cast<uint64_t*>(const_cast<char*>(value))))
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
    return true;
}

bool fptree_wrapper::remove(const char* key, size_t key_sz)
{
    return true;
}

int fptree_wrapper::scan(const char* key, size_t key_sz, int scan_sz, char*& values_out)
{
	// For now only support 8 bytes key and value (uint64_t)
    constexpr size_t ONE_MB = 1ULL << 20;
    static thread_local char results[ONE_MB];
#ifdef DEBUG_MSG
    printf("%d records scanned\n", scanned);
#endif
    return 0;
}
#endif
