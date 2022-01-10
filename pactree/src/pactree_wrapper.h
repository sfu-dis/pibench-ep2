#pragma once

#include "tree_api.hpp"
#include "pactree.h"
#include <numa-config.h>
#include "pactreeImpl.h"

#include <cstring>
#include <mutex>
#include <vector>
#include <shared_mutex>
#include <atomic>
#include <omp.h>

std::atomic<uint64_t> dram_allocated(0);
std::atomic<uint64_t> pmem_allocated(0);
std::atomic<uint64_t> dram_freed(0);
std::atomic<uint64_t> pmem_freed(0);


class pactree_wrapper : public tree_api
{
public:
    pactree_wrapper();
    virtual ~pactree_wrapper();

    virtual bool find(const char* key, size_t key_sz, char* value_out) override;
    virtual bool insert(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool update(const char* key, size_t key_sz, const char* value, size_t value_sz) override;
    virtual bool remove(const char* key, size_t key_sz) override;
    virtual int scan(const char* key, size_t key_sz, int scan_sz, char*& values_out) override;

private:
    pactree *tree_ = nullptr;
    thread_local static bool thread_init;
};
//static std::atomic<uint64_t> i_(0);
thread_local bool pactree_wrapper::thread_init = false;
struct ThreadHelper
{
    ThreadHelper(pactree* t){
        t->registerThread();
	// int id = omp_get_thread_num();
        // printf("Thread ID: %d\n", id);
    }
    ~ThreadHelper(){}
    
};

pactree_wrapper::pactree_wrapper()
{
    tree_ = new pactree(1);
}

pactree_wrapper::~pactree_wrapper()
{
#ifdef MEMORY_FOOTPRINT
    // printf("DRAM Allocated: %llu\n", dram_allocated.load());
    // printf("DRAM Freed: %llu\n", dram_freed.load());
    printf("PMEM Allocated: %llu\n", pmem_allocated.load());
    // printf("PMEM Freed: %llu\n", pmem_freed.load());
#endif
    if (tree_ != nullptr)
        delete tree_;
    //tree_ = nullptr;
}


bool pactree_wrapper::find(const char* key, size_t key_sz, char* value_out)
{
    thread_local ThreadHelper t(tree_);
    Val_t value = tree_->lookup(*reinterpret_cast<Key_t*>(const_cast<char*>(key)));
    if (value == 0)
    {
        return false;
    }
    memcpy(value_out, &value, sizeof(value));
    return true;
}

bool pactree_wrapper::insert(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
    thread_local ThreadHelper t(tree_);
    if (!tree_->insert(*reinterpret_cast<Key_t*>(const_cast<char *>(key)), *reinterpret_cast<Val_t*>(const_cast<char *>(value))))
    {
        return false;
    }
    return true;
}

bool pactree_wrapper::update(const char* key, size_t key_sz, const char* value, size_t value_sz)
{
    thread_local ThreadHelper t(tree_);
    if (!tree_->update(*reinterpret_cast<Key_t*>(const_cast<char*>(key)), *reinterpret_cast<Val_t*>(const_cast<char*>(value))))
    {
        return false;
    }
    return true;
}

bool pactree_wrapper::remove(const char* key, size_t key_sz) {
    thread_local ThreadHelper t(tree_);
    if (!tree_->remove(*reinterpret_cast<Key_t*>(const_cast<char*>(key))))
    {
        return false;
    }
    return true;
}

int pactree_wrapper::scan(const char* key, size_t key_sz, int scan_sz, char*& values_out)
{
    thread_local ThreadHelper t(tree_);
    constexpr size_t ONE_MB = 1ULL << 20;
    //static thread_local char results[ONE_MB];
    thread_local std::vector<Val_t> results;
    results.reserve(scan_sz);
    int scanned = tree_->scan(*reinterpret_cast<Key_t*>(const_cast<char*>(key)), (uint64_t)scan_sz, results);
//    if (scanned < 100)
//	printf("%d records scanned!\n", scanned);
    return scanned;
}
