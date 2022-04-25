#include "Tree.h"
#include "EpochGuard.h"
#include "N.h"
#include "nvm_mgr.h"
#include "threadinfo.h"
// #include "timer.h"
#include <algorithm>
#include <assert.h>
#include <fstream>
#include <functional>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace NVMMgr_ns;

namespace PART_ns {

#ifdef CHECK_COUNT
__thread int checkcount = 0;
#endif

#ifdef COUNT_ALLOC
__thread cpuCycleTimer *alloc_time = nullptr;
double getalloctime() { return alloc_time->duration(); }
#endif

#ifndef DRAM_MODE
POBJ_LAYOUT_BEGIN(DLART);
POBJ_LAYOUT_TOID(DLART, char);
POBJ_LAYOUT_END(DLART);
PMEMobjpool *pmem_pool;
#endif
void *allocate_size(size_t size) {
#ifdef COUNT_ALLOC
    if (alloc_time == nullptr)
        alloc_time = new cpuCycleTimer();
    alloc_time->start();
#endif

#ifdef MEMORY_FOOTPRINT
    pmem_allocated += size;
#endif

#ifdef DRAM_MODE
    void *addr = new (std::align_val_t(64)) char[size];
#else
    PMEMoid ptr;
    // pmemobj_zalloc(pmem_pool, &ptr, size, TOID_TYPE_NUM(char));
    pmemobj_alloc(pmem_pool, &ptr, size, 0, NULL, NULL);
    void *addr = (void *)pmemobj_direct(ptr);
#endif

#ifdef COUNT_ALLOC
    alloc_time->end();
#endif
    return addr;
}
//#endif

// Tree::Tree() {
//     std::cout << "[P-ART]\tnew P-ART\n";

//     init_nvm_mgr();
//     register_threadinfo();
//     NVMMgr *mgr = get_nvm_mgr();
//     //    Epoch_Mgr * epoch_mgr = new Epoch_Mgr();
// #ifdef ARTPMDK
//     const char *pool_name = "/mnt/pmem0/matianmao/dlartpmdk.data";
//     const char *layout_name = "DLART";
//     size_t pool_size = 64LL * 1024 * 1024 * 1024; // 16GB

//     if (access(pool_name, 0)) {
//         pmem_pool = pmemobj_create(pool_name, layout_name, pool_size, 0666);
//         if (pmem_pool == nullptr) {
//             std::cout << "[DLART]\tcreate fail\n";
//             assert(0);
//         }
//         std::cout << "[DLART]\tcreate\n";
//     } else {
//         pmem_pool = pmemobj_open(pool_name, layout_name);
//         std::cout << "[DLART]\topen\n";
//     }
//     std::cout << "[DLART]\topen pmem pool successfully\n";

//     root = new (allocate_size(sizeof(N256))) N256(0, {});
//     flush_data((void *)root, sizeof(N256));

// #else

//     if (mgr->first_created) {
//         // first open
//         root = new (mgr->alloc_tree_root()) N256(0, {});
//         flush_data((void *)root, sizeof(N256));
//         //        N::clflush((char *)root, sizeof(N256), true, true);
//         std::cout << "[P-ART]\tfirst create a P-ART\n";
//     } else {
//         // recovery
//         root = reinterpret_cast<N256 *>(mgr->alloc_tree_root());
// #ifdef INSTANT_RESTART
//         root->check_generation();
// #endif
//         std::cout << "[RECOVERY]\trecovery P-ART and reclaim the memory, root "
//                      "addr is "
//                   << (uint64_t)root << "\n";
//         //        rebuild(mgr->recovery_set);
//         //#ifdef RECLAIM_MEMORY
//         //        mgr->recovery_free_memory();
//         //#endif
//     }

// #endif
// }

Tree::Tree() {
    std::cout << "[P-ART]\tnew P-ART\n";
//    Epoch_Mgr * epoch_mgr = new Epoch_Mgr();
#ifdef DRAM_MODE
    root = new (std::align_val_t(64)) N256(0, {});
#elif defined(ARTPMDK) 
    const char *layout_name = "DLART";
    if (pool_size_ == 0)
        std::cout << "[DLART]\tpool size is 0\n";

    if (access(pool_path_, 0)) {
        pmem_pool = pmemobj_create(pool_path_, layout_name, pool_size_, 0666);
        if (pmem_pool == nullptr) {
            std::cout << "[DLART]\tcreate fail\n";
            assert(0);
        }
        std::cout << "[DLART]\tcreate\n";
    } else {
        pmem_pool = pmemobj_open(pool_path_, layout_name);
        std::cout << "[DLART]\topen\n";
    }
    std::cout << "[DLART]\topen pmem pool successfully!\n";
    root = new (allocate_size(sizeof(N256))) N256(0, {});
    flush_data((void *)root, sizeof(N256));

#else
    init_nvm_mgr();
    register_threadinfo();
    NVMMgr *mgr = get_nvm_mgr();
    if (mgr->first_created) {
        // first open
        root = new (mgr->alloc_tree_root()) N256(0, {});
        flush_data((void *)root, sizeof(N256));
        //        N::clflush((char *)root, sizeof(N256), true, true);
        std::cout << "[P-ART]\tfirst create a P-ART\n";
    } else {
        // recovery
        root = reinterpret_cast<N256 *>(mgr->alloc_tree_root());
    #ifdef INSTANT_RESTART
        root->check_generation();
    #endif
        std::cout << "[RECOVERY]\trecovery P-ART and reclaim the memory, root "
                     "addr is "
                  << (uint64_t)root << "\n";
        //        rebuild(mgr->recovery_set);
        //#ifdef RECLAIM_MEMORY
        //        mgr->recovery_free_memory();
        //#endif
    }

#endif
    std::cout << "[DLART]\tP-ART Initialization Complete\n";

#ifdef MEMORY_FOOTPRINT
    printf("N4: %ld bytes\n", sizeof(N4));
    printf("N16: %ld bytes\n", sizeof(N16));
    printf("N48: %ld bytes\n", sizeof(N48));
    printf("N256: %ld bytes\n", sizeof(N256));
    printf("LeafArray: %ld bytes\n", sizeof(LeafArray));
    printf("Leaf: %ld bytes\n", sizeof(Leaf));
#endif
}

Tree::~Tree() {
    // TODO: reclaim the memory of PM
    //    N::deleteChildren(root);
    //    N::deleteNode(root);
    std::cout << "[P-ART]\tshut down, free the tree\n";
    unregister_threadinfo();
    close_nvm_mgr();
}

// allocate a leaf and persist it
Leaf *Tree::allocLeaf(const Key *k) const {
#ifdef KEY_INLINE

    #ifdef ARTPMDK
        Leaf *newLeaf =
            new (allocate_size(sizeof(Leaf) + k->key_len + k->val_len)) Leaf(k);
        flush_data((void *)newLeaf, sizeof(Leaf) + k->key_len + k->val_len);
    #else

        Leaf *newLeaf =
            new (alloc_new_node_from_size(sizeof(Leaf) + k->key_len + k->val_len))
                Leaf(k);
        flush_data((void *)newLeaf, sizeof(Leaf) + k->key_len + k->val_len);
    #endif
        return newLeaf;
#else
    Leaf *newLeaf =
        new (alloc_new_node_from_type(NTypes::Leaf)) Leaf(k); // not persist
    flush_data((void *)newLeaf, sizeof(Leaf));
    return newLeaf;
#endif
}
#ifdef LEAF_ARRAY
Leaf *Tree::lookup(const Key *k) const {
    // enter a new epoch
    EpochGuard NewEpoch;
    bool need_restart;
    int restart_cnt = 0;
restart:
    need_restart = false;
    N *node = root;

    uint32_t level = 0;
    bool optimisticPrefixMatch = false;

    while (true) {
#ifdef INSTANT_RESTART
        node->check_generation();
#endif

#ifdef CHECK_COUNT
        int pre = level;
#endif
        switch (checkPrefix(node, k, level)) { // increases level
        case CheckPrefixResult::NoMatch:
            return nullptr;
        case CheckPrefixResult::OptimisticMatch:
            optimisticPrefixMatch = true;
            // fallthrough
        case CheckPrefixResult::Match: {
            if (k->getKeyLen() <= level) {
                return nullptr;
            }
            node = N::getChild(k->fkey[level], node);

#ifdef CHECK_COUNT
            checkcount += std::min(4, (int)level - pre);
#endif

            if (node == nullptr) {
                return nullptr;
            }
            
            if (N::isLeafArray(node)) {

                auto la = N::getLeafArray(node);
                //                auto v = la->getVersion();
                auto ret = la->lookup(k);
                //                if (la->isObsolete(v) ||
                //                !la->readVersionOrRestart(v)) {
                //                    printf("read restart\n");
                //                    goto restart;
                //                }
                if (ret == nullptr && restart_cnt < 0) {
                    restart_cnt++;
                    goto restart;
                }
                return ret;
            }
        }
        }
        level++;
    }
}
#else
Leaf *Tree::lookup(const Key *k) const {
    // enter a new epoch
    EpochGuard NewEpoch;

    N *node = root;

    uint32_t level = 0;
    bool optimisticPrefixMatch = false;

    while (true) {
#ifdef INSTANT_RESTART
        node->check_generation();
#endif

#ifdef CHECK_COUNT
        int pre = level;
#endif
        switch (checkPrefix(node, k, level)) { // increases level
        case CheckPrefixResult::NoMatch:
            return nullptr;
        case CheckPrefixResult::OptimisticMatch:
            optimisticPrefixMatch = true;
            // fallthrough
        case CheckPrefixResult::Match: {
            if (k->getKeyLen() <= level) {
                return nullptr;
            }
            node = N::getChild(k->fkey[level], node);

#ifdef CHECK_COUNT
            checkcount += std::min(4, (int)level - pre);
#endif

            if (node == nullptr) {
                return nullptr;
            }
            if (N::isLeaf(node)) {
                Leaf *ret = N::getLeaf(node);
                if (level < k->getKeyLen() - 1 || optimisticPrefixMatch) {
#ifdef CHECK_COUNT
                    checkcount += k->getKeyLen();
#endif
                    if (ret->checkKey(k)) {
                        return ret;
                    } else {
                        return nullptr;
                    }
                } else {
                    return ret;
                }
            }
        }
        }
        level++;
    }
}
#endif
#ifdef CHECK_COUNT
int get_count() { return checkcount; }
#endif

typename Tree::OperationResults Tree::update(const Key *k) const {
    EpochGuard NewEpoch;
restart:
    bool needRestart = false;

    N *node = nullptr;
    N *nextNode = root;
    uint8_t nodeKey = 0;
    uint32_t level = 0;
    // bool optimisticPrefixMatch = false;

    while (true) {
        node = nextNode;
#ifdef INSTANT_RESTART
        node->check_generation();
#endif
        auto v = node->getVersion(); // check version

        switch (checkPrefix(node, k, level)) { // increases level
        case CheckPrefixResult::NoMatch:
            if (N::isObsolete(v) || !node->readVersionOrRestart(v)) {
                goto restart;
            }
            return OperationResults::NotFound;
        case CheckPrefixResult::OptimisticMatch:
            // fallthrough
        case CheckPrefixResult::Match: {
            // if (level >= k->getKeyLen()) {
            //     // key is too short
            //     // but it next fkey is 0
            //     return OperationResults::NotFound;
            // }
            nodeKey = k->fkey[level];

            nextNode = N::getChild(nodeKey, node);

            if (nextNode == nullptr) {
                if (N::isObsolete(v) || !node->readVersionOrRestart(v)) {
                    //                        std::cout<<"retry\n";
                    goto restart;
                }
                return OperationResults::NotFound;
            }
#ifdef LEAF_ARRAY
            if (N::isLeafArray(nextNode)) {
                node->lockVersionOrRestart(v, needRestart);
                if (needRestart) {
                    //                        std::cout<<"retry\n";
                    goto restart;
                }

                auto *leaf_array = N::getLeafArray(nextNode);
                auto leaf = allocLeaf(k);
                auto result = leaf_array->update(k, leaf);
                node->writeUnlock();
                if (!result) {
                #ifdef MEMORY_FOOTPRINT
                    pmem_deallocated += sizeof(Leaf);
                #endif
                    EpochGuard::DeleteNode(leaf);
                    return OperationResults::NotFound;
                } else {
                    return OperationResults::Success;
                }
            }
#else
            if (N::isLeaf(nextNode)) {
                node->lockVersionOrRestart(v, needRestart);
                if (needRestart) {
                    //                        std::cout<<"retry\n";
                    goto restart;
                }

                Leaf *leaf = N::getLeaf(nextNode);
                if (!leaf->checkKey(k)) {
                    node->writeUnlock();
                    return OperationResults::NotFound;
                }
                //
                Leaf *newleaf = allocLeaf(k);
                //
                N::change(node, nodeKey, N::setLeaf(newleaf));
                node->writeUnlock();
                return OperationResults::Success;
            }
#endif
            level++;
        }
        }
    }
}
#ifdef LEAF_ARRAY
bool Tree::lookupRange(const Key *start, const Key *end, const Key *continueKey,
                       Leaf *result[], std::size_t resultSize,
                       std::size_t &resultsFound) const {
    if (!N::key_key_lt(start, end)) {
        printf("Start key: %llu should be less than end key: %llu!\n", start->key, end->key);
        resultsFound = 0;
        return false;
    }
    //    for (uint32_t i = 0; i < std::min(start->getKeyLen(),
    //    end->getKeyLen());
    //         ++i) {
    //        if (start->fkey[i] > end->fkey[i]) {
    //            resultsFound = 0;
    //            return false;
    //        } else if (start->fkey[i] < end->fkey[i]) {
    //            break;
    //        }
    //    }
    char scan_value[100];
    EpochGuard NewEpoch;

    Leaf *toContinue = nullptr;
    bool restart;
    std::function<void(N *, int, bool, bool)> copy =
        [&result, &resultSize, &resultsFound, &toContinue, &copy, &scan_value,
         &start, &end](N *node, int compare_level, bool compare_start,
                       bool compare_end) {
            if (N::isLeafArray(node)) {

                auto la = N::getLeafArray(node);

                auto leaves = la->getSortedLeaf(start, end, compare_level,
                                                compare_start, compare_end);

                for (auto leaf : leaves) {
                    if (resultsFound == resultSize) {
                        toContinue = N::getLeaf(node);
                        return;
                    }

                    result[resultsFound] = leaf;
                    // memcmp((char*)&result[resultsFound*2], leaf->kv, 16); // assuming key + val = 16 bytes
                    resultsFound++;
                }
            } else {
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, 0u, 255u, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    N *n = std::get<1>(children[i]);
                    copy(n, node->getLevel() + 1, compare_start, compare_end);
                    if (toContinue != nullptr) {
                        break;
                    }
                }
            }
        };
    std::function<void(N *, uint32_t)> findStart =
        [&copy, &start, &findStart, &toContinue, &restart,
         this](N *node, uint32_t level) {
            if (N::isLeafArray(node)) {
                copy(node, level, true, false);
                return;
            }

            PCCompareResults prefixResult;
            prefixResult = checkPrefixCompare(node, start, level);
            switch (prefixResult) {
            case PCCompareResults::Bigger:
                copy(node, level, false, false);
                break;
            case PCCompareResults::Equal: {
                uint8_t startLevel = (start->getKeyLen() > level)
                                         ? start->fkey[level]
                                         : (uint8_t)0;
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, startLevel, 255, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);
                    if (k == startLevel) {
                        findStart(n, level + 1);
                    } else if (k > startLevel) {
                        copy(n, level + 1, false, false);
                    }
                    if (toContinue != nullptr || restart) {
                        break;
                    }
                }
                break;
            }
            case PCCompareResults::SkippedLevel:
                restart = true;
                break;
            case PCCompareResults::Smaller:
                break;
            }
        };
    std::function<void(N *, uint32_t)> findEnd =
        [&copy, &end, &toContinue, &restart, &findEnd, this](N *node,
                                                             uint32_t level) {
            if (N::isLeafArray(node)) {
                // there might be some leaves less than end
                copy(node, level, false, true);
                return;
            }

            PCCompareResults prefixResult;
            prefixResult = checkPrefixCompare(node, end, level);

            switch (prefixResult) {
            case PCCompareResults::Smaller:
                copy(node, level, false, false);
                break;
            case PCCompareResults::Equal: {
                uint8_t endLevel = (end->getKeyLen() > level) ? end->fkey[level]
                                                              : (uint8_t)255;
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, 0, endLevel, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);
                    if (k == endLevel) {
                        findEnd(n, level + 1);
                    } else if (k < endLevel) {
                        copy(n, level + 1, false, false);
                    }
                    if (toContinue != nullptr || restart) {
                        break;
                    }
                }
                break;
            }
            case PCCompareResults::Bigger:
                break;
            case PCCompareResults::SkippedLevel:
                restart = true;
                break;
            }
        };

restart:
    restart = false;
    resultsFound = 0;

    uint32_t level = 0;
    N *node = nullptr;
    N *nextNode = root;

    while (true) {
        if (!(node = nextNode) || toContinue)
            break;
        if (N::isLeafArray(node)) {
            copy(node, level, true, true);
            break;
        }

        PCEqualsResults prefixResult;
        prefixResult = checkPrefixEquals(node, level, start, end);
        switch (prefixResult) {
        case PCEqualsResults::SkippedLevel:
            goto restart;
        case PCEqualsResults::NoMatch: {
            return false;
        }
        case PCEqualsResults::Contained: {
            copy(node, level + 1, false, false);
            break;
        }
        case PCEqualsResults::BothMatch: {
            uint8_t startLevel =
                (start->getKeyLen() > level) ? start->fkey[level] : (uint8_t)0;
            uint8_t endLevel =
                (end->getKeyLen() > level) ? end->fkey[level] : (uint8_t)255;
            if (startLevel != endLevel) {
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, startLevel, endLevel, children,
                               childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);

                    if (k == startLevel) {
                        findStart(n, level + 1);
                    } else if (k > startLevel && k < endLevel) {
                        copy(n, level + 1, false, false);
                    } else if (k == endLevel) {
                        findEnd(n, level + 1);
                    }
                    if (restart) {
                        goto restart;
                    }
                    if (toContinue) {
                        break;
                    }
                }
            } else {

                nextNode = N::getChild(startLevel, node);

                level++;
                continue;
            }
            break;
        }
        }
        break;
    }

    if (toContinue != nullptr) {
        Key *newkey = new Key();
#ifdef KEY_INLINE
        newkey->Init((char *)toContinue->GetKey(), toContinue->key_len,
                     toContinue->GetValue(), toContinue->val_len);
#else
        newkey->Init((char *)toContinue->fkey, toContinue->key_len,
                     toContinue->value, toContinue->val_len);
#endif
        continueKey = newkey;
        return true;
    } else {
        return false;
    }
}

#else
bool Tree::lookupRange(const Key *start, const Key *end, const Key *continueKey,
                       Leaf *result[], std::size_t resultSize,
                       std::size_t &resultsFound) const {
    if (!N::key_key_lt(start, end)) {
        resultsFound = 0;
        return false;
    }
    //    for (uint32_t i = 0; i < std::min(start->getKeyLen(),
    //    end->getKeyLen());
    //         ++i) {
    //        if (start->fkey[i] > end->fkey[i]) {
    //            resultsFound = 0;
    //            return false;
    //        } else if (start->fkey[i] < end->fkey[i]) {
    //            break;
    //        }
    //    }
    char scan_value[100];
    // enter a new epoch
    EpochGuard NewEpoch;

    Leaf *toContinue = nullptr;
    bool restart;
    std::function<void(N *)> copy = [&result, &resultSize, &resultsFound,
                                     &toContinue, &copy, &scan_value,
                                     start](N *node) {
        if (N::isLeaf(node)) {
            if (resultsFound == resultSize) {
                toContinue = N::getLeaf(node);
                return;
            }
            Leaf *leaf = N::getLeaf(node);
            result[resultsFound] = N::getLeaf(node);
            resultsFound++;
        } else {
            std::tuple<uint8_t, N *> children[256];
            uint32_t childrenCount = 0;
            N::getChildren(node, 0u, 255u, children, childrenCount);
            for (uint32_t i = 0; i < childrenCount; ++i) {
                N *n = std::get<1>(children[i]);
                copy(n);
                if (toContinue != nullptr) {
                    break;
                }
            }
        }
    };
    std::function<void(N *, uint32_t)> findStart =
        [&copy, &start, &findStart, &toContinue, &restart,
         this](N *node, uint32_t level) {
            if (N::isLeaf(node)) {
                // correct the bug
                if (N::leaf_key_lt(N::getLeaf(node), start, level) == false) {
                    copy(node);
                }
                return;
            }

            PCCompareResults prefixResult;
            prefixResult = checkPrefixCompare(node, start, level);
            switch (prefixResult) {
            case PCCompareResults::Bigger:
                copy(node);
                break;
            case PCCompareResults::Equal: {
                uint8_t startLevel = (start->getKeyLen() > level)
                                         ? start->fkey[level]
                                         : (uint8_t)0;
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, startLevel, 255, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);
                    if (k == startLevel) {
                        findStart(n, level + 1);
                    } else if (k > startLevel) {
                        copy(n);
                    }
                    if (toContinue != nullptr || restart) {
                        break;
                    }
                }
                break;
            }
            case PCCompareResults::SkippedLevel:
                restart = true;
                break;
            case PCCompareResults::Smaller:
                break;
            }
        };
    std::function<void(N *, uint32_t)> findEnd =
        [&copy, &end, &toContinue, &restart, &findEnd, this](N *node,
                                                             uint32_t level) {
            if (N::isLeaf(node)) {
                if (N::leaf_key_lt(N::getLeaf(node), end, level)) {
                    copy(node);
                }
                return;
            }

            PCCompareResults prefixResult;
            prefixResult = checkPrefixCompare(node, end, level);

            switch (prefixResult) {
            case PCCompareResults::Smaller:
                copy(node);
                break;
            case PCCompareResults::Equal: {
                uint8_t endLevel = (end->getKeyLen() > level) ? end->fkey[level]
                                                              : (uint8_t)255;
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, 0, endLevel, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);
                    if (k == endLevel) {
                        findEnd(n, level + 1);
                    } else if (k < endLevel) {
                        copy(n);
                    }
                    if (toContinue != nullptr || restart) {
                        break;
                    }
                }
                break;
            }
            case PCCompareResults::Bigger:
                break;
            case PCCompareResults::SkippedLevel:
                restart = true;
                break;
            }
        };

restart:
    restart = false;
    resultsFound = 0;

    uint32_t level = 0;
    N *node = nullptr;
    N *nextNode = root;

    while (true) {
        if (!(node = nextNode) || toContinue)
            break;
        PCEqualsResults prefixResult;
        prefixResult = checkPrefixEquals(node, level, start, end);
        switch (prefixResult) {
        case PCEqualsResults::SkippedLevel:
            goto restart;
        case PCEqualsResults::NoMatch: {
            return false;
        }
        case PCEqualsResults::Contained: {
            copy(node);
            break;
        }
        case PCEqualsResults::BothMatch: {
            uint8_t startLevel =
                (start->getKeyLen() > level) ? start->fkey[level] : (uint8_t)0;
            uint8_t endLevel =
                (end->getKeyLen() > level) ? end->fkey[level] : (uint8_t)255;
            if (startLevel != endLevel) {
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, startLevel, endLevel, children,
                               childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const uint8_t k = std::get<0>(children[i]);
                    N *n = std::get<1>(children[i]);
                    if (k == startLevel) {
                        findStart(n, level + 1);
                    } else if (k > startLevel && k < endLevel) {
                        copy(n);
                    } else if (k == endLevel) {
                        findEnd(n, level + 1);
                    }
                    if (restart) {
                        goto restart;
                    }
                    if (toContinue) {
                        break;
                    }
                }
            } else {

                nextNode = N::getChild(startLevel, node);

                level++;
                continue;
            }
            break;
        }
        }
        break;
    }

    if (toContinue != nullptr) {
        Key *newkey = new Key();
#ifdef KEY_INLINE
        newkey->Init((char *)toContinue->GetKey(), toContinue->key_len,
                     toContinue->GetValue(), toContinue->val_len);
#else
        newkey->Init((char *)toContinue->fkey, toContinue->key_len,
                     toContinue->value, toContinue->val_len);
#endif
        continueKey = newkey;
        return true;
    } else {
        return false;
    }
}
#endif
bool Tree::checkKey(const Key *ret, const Key *k) const {
    return ret->getKeyLen() == k->getKeyLen() &&
           memcmp(ret->fkey, k->fkey, k->getKeyLen()) == 0;
}

typename Tree::OperationResults Tree::insert(const Key *k) {
    EpochGuard NewEpoch;
restart:
    bool needRestart = false;
    N *node = nullptr;
    N *nextNode = root;
    N *parentNode = nullptr;
    uint8_t parentKey, nodeKey = 0;
    uint32_t level = 0;
    // printf("Begin of insert\n"); //segfault
    while (true) {
        // printf("Begin of while\n"); //segfault
        parentNode = node;
        parentKey = nodeKey;
        node = nextNode;
#ifdef INSTANT_RESTART
        node->check_generation();
#endif
        auto v = node->getVersion();
        uint32_t nextLevel = level;

        uint8_t nonMatchingKey;
        Prefix remainingPrefix;
        //printf("Begin of switch\n"); //segfault
        switch (
            checkPrefixPessimistic(node, k, nextLevel, nonMatchingKey,
                                   remainingPrefix)) { // increases nextLevel
        case CheckPrefixPessimisticResult::SkippedLevel:
            // printf("restart on SkippedLevel!\n"); //segfault
            goto restart;
        case CheckPrefixPessimisticResult::NoMatch: {
	    if (nextLevel >= k->getKeyLen())
		return OperationResults::Existed;
            //assert(nextLevel < k->getKeyLen()); // prevent duplicate key
            node->lockVersionOrRestart(v, needRestart);
            if (needRestart) {
                // printf("restart on first needRestart!\n"); //segfault
                goto restart;
            }

            // 1) Create new node which will be parent of node, Set common
            // prefix, level to this node
            Prefix prefi = node->getPrefi();
            prefi.prefixCount = nextLevel - level;
#ifdef ARTPMDK
            N4 *newNode = new (allocate_size(sizeof(N4))) N4(nextLevel, prefi);
#else
            auto newNode = new (alloc_new_node_from_type(NTypes::N4))
                N4(nextLevel, prefi); // not persist
#endif

            // 2)  add node and (tid, *k) as children

            auto *newLeaf = allocLeaf(k);

#ifdef LEAF_ARRAY
            auto newLeafArray =
                new (alloc_new_node_from_type(NTypes::LeafArray)) LeafArray();
            newLeafArray->insert(newLeaf, true);
            newNode->insert(k->fkey[nextLevel], N::setLeafArray(newLeafArray),
                            false);
#else
            newNode->insert(k->fkey[nextLevel], N::setLeaf(newLeaf), false);
#endif
            // not persist
            newNode->insert(nonMatchingKey, node, false);
            // persist the new node
            flush_data((void *)newNode, sizeof(N4));

            // 3) lockVersionOrRestart, update parentNode to point to the
            // new node, unlock
            parentNode->writeLockOrRestart(needRestart);
            if (needRestart) {
            #ifdef MEMORY_FOOTPRINT
                pmem_deallocated += sizeof(N4);
            #endif
                EpochGuard::DeleteNode((void *)newNode);
#ifdef LEAF_ARRAY
            #ifdef MEMORY_FOOTPRINT
                pmem_deallocated += sizeof(LeafArray);
            #endif
                EpochGuard::DeleteNode(newLeafArray);
#endif
            #ifdef MEMORY_FOOTPRINT
                pmem_deallocated += sizeof(Leaf);
            #endif
                EpochGuard::DeleteNode((void *)newLeaf);

                node->writeUnlock();
                // printf("restart on second needRestart!\n"); //segfault
                goto restart;
            }

            N::change(parentNode, parentKey, newNode);
            parentNode->writeUnlock();

            // 4) update prefix of node, unlock
            node->setPrefix(
                remainingPrefix.prefix,
                node->getPrefi().prefixCount - ((nextLevel - level) + 1), true);
            //            std::cout<<"insert success\n";

            node->writeUnlock();
            // printf("end of NoMatch, return Success!\n"); //segfault
            return OperationResults::Success;

        } // end case  NoMatch
        case CheckPrefixPessimisticResult::Match:
            //printf("Match, break switch!\n"); //segfault
            break;
        }
        // printf("End of switch\n"); //segfault
        //assert(nextLevel < k->getKeyLen()); // prevent duplicate key
        if (nextLevel >= k->getKeyLen()){
	    //printf("Key is: %s    key len: %d       nextlevel: %d\n", k->fkey, k->key_len, nextLevel);
	    //exit(1);
	    return OperationResults::Existed;
	}

	// TODO: maybe one string is substring of another, so it fkey[level]
        // will be 0 solve problem of substring
        level = nextLevel;
        nodeKey = k->fkey[level];

        // segfault
        // switch (node->getType()) {
        // case NTypes::N4: {
        //     printf("N4\n");
        //     break;
        // }
        // case NTypes::N16: {
        //     printf("N16\n");
        //     break;
        // }
        // case NTypes::N48: {
        //     printf("N48\n");
        //     break;
        // }
        // case NTypes::N256: {
        //     printf("N256\n");
        //     break;
        // }
        // case NTypes::LeafArray: {
        //     printf("LeafArray\n");
        //     break;
        // }
        // default: {
        //     assert(false);
        // }
        // }
        nextNode = N::getChild(nodeKey, node);
        // printf("%p\n", nextNode); //segfault
        // printf("NextLevel: %d, KeyLen: %d\n", nextLevel, k->getKeyLen()); //segfault
        if (nextNode == nullptr) { //  || nextNode == (void*)0x303030303030) {
            // printf("nextNode == nullptr\n"); //segfault
            node->lockVersionOrRestart(v, needRestart);
            if (needRestart)
                goto restart;
            Leaf *newLeaf = allocLeaf(k);
#ifdef LEAF_ARRAY
            auto newLeafArray =
                new (alloc_new_node_from_type(NTypes::LeafArray)) LeafArray();
            newLeafArray->insert(newLeaf, true);
            // printf("New leaf ptr: %p\n", newLeaf); //segfault
            // printf("Newly leaf array ptr: %p\n", newLeafArray); //segfault
            N::insertAndUnlock(node, parentNode, parentKey, nodeKey,
                               N::setLeafArray(newLeafArray), needRestart);
#else
            N::insertAndUnlock(node, parentNode, parentKey, nodeKey,
                               N::setLeaf(newLeaf), needRestart);
#endif
            if (needRestart)
                goto restart;

            return OperationResults::Success;
        }
        // printf("Keep going\n"); //segfault
#ifdef LEAF_ARRAY
        if (N::isLeafArray(nextNode)) {
            // printf("nextNode is LeafArray\n"); //segfault
            auto leaf_array = N::getLeafArray(nextNode);
            if (leaf_array->lookup(k) != nullptr) {
                return OperationResults::Existed;
            } else {
                auto lav = leaf_array->getVersion();
                leaf_array->lockVersionOrRestart(lav, needRestart);
                if (needRestart) {
                    goto restart;
                }
                if (leaf_array->isFull()) {
                    leaf_array->splitAndUnlock(node, nodeKey, needRestart);
                    if (needRestart) {
                        goto restart;
                    }
                    nextNode = N::getChild(nodeKey, node);
                    // insert at the next iteration
                } else {
                    auto leaf = allocLeaf(k);
                    leaf_array->insert(leaf, true);
                    leaf_array->writeUnlock();
                    return OperationResults::Success;
                }
            }
        }
#else
        if (N::isLeaf(nextNode)) {
            node->lockVersionOrRestart(v, needRestart);
            if (needRestart)
                goto restart;
            Leaf *leaf = N::getLeaf(nextNode);

            level++;
            // assert(level < leaf->getKeyLen());
            // prevent inserting when
            // prefix of leaf exists already
            // but if I want to insert a prefix of this leaf, i also need to
            // insert successfully
            uint32_t prefixLength = 0;
#ifdef KEY_INLINE
            while (level + prefixLength <
                       std::min(k->getKeyLen(), leaf->getKeyLen()) &&
                   leaf->kv[level + prefixLength] ==
                       k->fkey[level + prefixLength]) {
                prefixLength++;
            }
#else
            while (level + prefixLength <
                       std::min(k->getKeyLen(), leaf->getKeyLen()) &&
                   leaf->fkey[level + prefixLength] ==
                       k->fkey[level + prefixLength]) {
                prefixLength++;
            }
#endif
            // equal
            if (k->getKeyLen() == leaf->getKeyLen() &&
                level + prefixLength == k->getKeyLen()) {
                // duplicate leaf
                node->writeUnlock();
                //                std::cout<<"ohfinish\n";
                return OperationResults::Existed;
            }
            // substring
#ifdef ARTPMDK
            N4 *n4 = new (allocate_size(sizeof(N4)))
                N4(level + prefixLength, &k->fkey[level],
                   prefixLength); // not persist
#else
            auto n4 = new (alloc_new_node_from_type(NTypes::N4))
                N4(level + prefixLength, &k->fkey[level],
                   prefixLength); // not persist
#endif
            Leaf *newLeaf = allocLeaf(k);
            n4->insert(k->fkey[level + prefixLength], N::setLeaf(newLeaf),
                       false);
#ifdef KEY_INLINE
            n4->insert(leaf->kv[level + prefixLength], nextNode, false);
#else
            n4->insert(leaf->fkey[level + prefixLength], nextNode, false);
#endif
            flush_data((void *)n4, sizeof(N4));

            N::change(node, k->fkey[level - 1], n4);
            node->writeUnlock();
            return OperationResults::Success;
        }
#endif
        // printf("level ++!\n"); //segfault
        level++;
    }
    // printf("Insert finished!\n"); //segfault
    //    std::cout<<"ohfinish\n";
}

typename Tree::OperationResults Tree::remove(const Key *k) {
    EpochGuard NewEpoch;
restart:
    bool needRestart = false;

    N *node = nullptr;
    N *nextNode = root;
    N *parentNode = nullptr;
    uint8_t parentKey, nodeKey = 0;
    uint32_t level = 0;
    // bool optimisticPrefixMatch = false;

    while (true) {
        parentNode = node;
        parentKey = nodeKey;
        node = nextNode;
#ifdef INSTANT_RESTART
        node->check_generation();
#endif
        auto v = node->getVersion();

        switch (checkPrefix(node, k, level)) { // increases level
        case CheckPrefixResult::NoMatch:
            if (N::isObsolete(v) || !node->readVersionOrRestart(v)) {
                goto restart;
            }
            return OperationResults::NotFound;
        case CheckPrefixResult::OptimisticMatch:
            // fallthrough
        case CheckPrefixResult::Match: {
            // if (level >= k->getKeyLen()) {
            //     // key is too short
            //     // but it next fkey is 0
            //     return OperationResults::NotFound;
            // }
            nodeKey = k->fkey[level];

            nextNode = N::getChild(nodeKey, node);

            if (nextNode == nullptr) {
                if (N::isObsolete(v) ||
                    !node->readVersionOrRestart(v)) { // TODO
                    goto restart;
                }
                return OperationResults::NotFound;
            }
#ifdef LEAF_ARRAY
            if (N::isLeafArray(nextNode)) {
                auto *leaf_array = N::getLeafArray(nextNode);
                auto lav = leaf_array->getVersion();
                leaf_array->lockVersionOrRestart(lav, needRestart);
                if (needRestart) {
                    goto restart;
                }
                auto result = leaf_array->remove(k);
                leaf_array->writeUnlock();
                if (!result) {
                    return OperationResults::NotFound;
                } else {
                    return OperationResults::Success;
                }
            }
#else
            if (N::isLeaf(nextNode)) {
                node->lockVersionOrRestart(v, needRestart);
                if (needRestart)
                    goto restart;

                Leaf *leaf = N::getLeaf(nextNode);
                if (!leaf->checkKey(k)) {
                    node->writeUnlock();
                    return OperationResults::NotFound;
                }
                assert(parentNode == nullptr || N::getCount(node) != 1);
                if (N::getCount(node) == 2 && node != root) {
                    // 1. check remaining entries
                    N *secondNodeN;
                    uint8_t secondNodeK;
                    std::tie(secondNodeN, secondNodeK) =
                        N::getSecondChild(node, nodeKey);
                    if (N::isLeaf(secondNodeN)) {
                        parentNode->writeLockOrRestart(needRestart);
                        if (needRestart) {
                            node->writeUnlock();
                            goto restart;
                        }

                        // N::remove(node, k[level]); not necessary
                        N::change(parentNode, parentKey, secondNodeN);

                        parentNode->writeUnlock();
                        node->writeUnlockObsolete();

                        // remove the node
                        EpochGuard::DeleteNode((void *)node);
                    } else {
                        uint64_t vChild = secondNodeN->getVersion();
                        secondNodeN->lockVersionOrRestart(vChild, needRestart);
                        if (needRestart) {
                            node->writeUnlock();
                            goto restart;
                        }
                        parentNode->writeLockOrRestart(needRestart);
                        if (needRestart) {
                            node->writeUnlock();
                            secondNodeN->writeUnlock();
                            goto restart;
                        }

                        // N::remove(node, k[level]); not necessary
                        N::change(parentNode, parentKey, secondNodeN);

                        secondNodeN->addPrefixBefore(node, secondNodeK);

                        parentNode->writeUnlock();
                        node->writeUnlockObsolete();

                        // remove the node
                        EpochGuard::DeleteNode((void *)node);

                        secondNodeN->writeUnlock();
                    }
                } else {
                    N::removeAndUnlock(node, k->fkey[level], parentNode,
                                       parentKey, needRestart);
                    if (needRestart)
                        goto restart;
                }
                // remove the leaf
                EpochGuard::DeleteNode((void *)leaf);

                return OperationResults::Success;
            }
#endif
            level++;
        }
        }
    }
}

void Tree::rebuild(std::vector<std::pair<uint64_t, size_t>> &rs,
                   uint64_t start_addr, uint64_t end_addr, int thread_id) {
    // rebuild meta data count/compactcount
    // record all used memory (offset, size) into rs set
    N::rebuild_node(root, rs, start_addr, end_addr, thread_id);
}

typename Tree::CheckPrefixResult Tree::checkPrefix(N *n, const Key *k,
                                                   uint32_t &level) {
    if (k->getKeyLen() <= n->getLevel()) {
        return CheckPrefixResult::NoMatch;
    }
    Prefix p = n->getPrefi();
    if (p.prefixCount + level < n->getLevel()) {
        level = n->getLevel();
        return CheckPrefixResult::OptimisticMatch;
    }
    if (p.prefixCount > 0) {
        for (uint32_t i = ((level + p.prefixCount) - n->getLevel());
             i < std::min(p.prefixCount, maxStoredPrefixLength); ++i) {
            if (p.prefix[i] != k->fkey[level]) {
                return CheckPrefixResult::NoMatch;
            }
            ++level;
        }
        if (p.prefixCount > maxStoredPrefixLength) {
            // level += p.prefixCount - maxStoredPrefixLength;
            level = n->getLevel();
            return CheckPrefixResult::OptimisticMatch;
        }
    }
    return CheckPrefixResult::Match;
}

typename Tree::CheckPrefixPessimisticResult
Tree::checkPrefixPessimistic(N *n, const Key *k, uint32_t &level,
                             uint8_t &nonMatchingKey,
                             Prefix &nonMatchingPrefix) {
    Prefix p = n->getPrefi();
    if (p.prefixCount + level != n->getLevel()) {
        // printf("Shoud not be called during normal operations\n"); //segfault
        // exit(1);
        // Intermediate or inconsistent state from path compression
        // "splitAndUnlock" or "merge" is detected Inconsistent path compressed
        // prefix should be recovered in here
        bool needRecover = false;
        auto v = n->getVersion();
        n->lockVersionOrRestart(v, needRecover);
        if (!needRecover) {
            // Inconsistent state due to prior system crash is suspected --> Do
            // recovery

            // 1) Picking up arbitrary two leaf nodes and then 2) rebuilding
            // correct compressed prefix
            uint32_t discrimination =
                (n->getLevel() > level ? n->getLevel() - level
                                       : level - n->getLevel());
            Leaf *kr = N::getAnyChildTid(n);
            p.prefixCount = discrimination;
            for (uint32_t i = 0;
                 i < std::min(discrimination, maxStoredPrefixLength); i++) {
#ifdef KEY_INLINE
                p.prefix[i] = kr->kv[level + i];
#else
                p.prefix[i] = kr->fkey[level + i];
#endif
            }
            n->setPrefix(p.prefix, p.prefixCount, true);
            n->writeUnlock();
        }

        // path compression merge is in progress --> restart from root
        // path compression splitAndUnlock is in progress --> skipping an
        // intermediate compressed prefix by using level (invariant)
        if (p.prefixCount + level < n->getLevel()) {
            return CheckPrefixPessimisticResult::SkippedLevel;
        }
    }
    if (p.prefixCount > 0) {
        uint32_t prevLevel = level;
        Leaf *kt = nullptr;
        bool load_flag = false;
        for (uint32_t i = ((level + p.prefixCount) - n->getLevel());
             i < p.prefixCount; ++i) {
            if (i >= maxStoredPrefixLength && !load_flag) {
                //            if (i == maxStoredPrefixLength) {
                // Optimistic path compression
                kt = N::getAnyChildTid(n);
                load_flag = true;
            }
#ifdef KEY_INLINE
            uint8_t curKey = i >= maxStoredPrefixLength ? (uint8_t)kt->kv[level]
                                                        : p.prefix[i];
#else
            uint8_t curKey =
                i >= maxStoredPrefixLength ? kt->fkey[level] : p.prefix[i];
#endif
            if (curKey != k->fkey[level]) {
                nonMatchingKey = curKey;
                if (p.prefixCount > maxStoredPrefixLength) {
                    if (i < maxStoredPrefixLength) {
                        kt = N::getAnyChildTid(n);
                    }
                    for (uint32_t j = 0;
                         j < std::min((p.prefixCount - (level - prevLevel) - 1),
                                      maxStoredPrefixLength);
                         ++j) {
#ifdef KEY_INLINE
                        nonMatchingPrefix.prefix[j] =
                            (uint8_t)kt->kv[level + j + 1];
#else
                        nonMatchingPrefix.prefix[j] = kt->fkey[level + j + 1];
#endif
                    }
                } else {
                    for (uint32_t j = 0; j < p.prefixCount - i - 1; ++j) {
                        nonMatchingPrefix.prefix[j] = p.prefix[i + j + 1];
                    }
                }
                return CheckPrefixPessimisticResult::NoMatch;
            }
            ++level;
        }
    }
    return CheckPrefixPessimisticResult::Match;
}

typename Tree::PCCompareResults
Tree::checkPrefixCompare(const N *n, const Key *k, uint32_t &level) {
    Prefix p = n->getPrefi();
    if (p.prefixCount + level < n->getLevel()) {
        return PCCompareResults::SkippedLevel;
    }
    if (p.prefixCount > 0) {
        Leaf *kt = nullptr;
        bool load_flag = false;
        for (uint32_t i = ((level + p.prefixCount) - n->getLevel());
             i < p.prefixCount; ++i) {
            if (i >= maxStoredPrefixLength && !load_flag) {
                // loadKey(N::getAnyChildTid(n), kt);
                kt = N::getAnyChildTid(n);
                load_flag = true;
            }
            uint8_t kLevel =
                (k->getKeyLen() > level) ? k->fkey[level] : (uint8_t)0;

#ifdef KEY_INLINE
            uint8_t curKey = i >= maxStoredPrefixLength ? (uint8_t)kt->kv[level]
                                                        : p.prefix[i];
#else
            uint8_t curKey =
                i >= maxStoredPrefixLength ? kt->fkey[level] : p.prefix[i];
#endif
            if (curKey < kLevel) {
                return PCCompareResults::Smaller;
            } else if (curKey > kLevel) {
                return PCCompareResults::Bigger;
            }
            ++level;
        }
    }
    return PCCompareResults::Equal;
}

typename Tree::PCEqualsResults Tree::checkPrefixEquals(const N *n,
                                                       uint32_t &level,
                                                       const Key *start,
                                                       const Key *end) {
    Prefix p = n->getPrefi();
    if (p.prefixCount + level < n->getLevel()) {
        return PCEqualsResults::SkippedLevel;
    }
    if (p.prefixCount > 0) {
        Leaf *kt = nullptr;
        bool load_flag = false;
        for (uint32_t i = ((level + p.prefixCount) - n->getLevel());
             i < p.prefixCount; ++i) {
            if (i >= maxStoredPrefixLength && !load_flag) {
                // loadKey(N::getAnyChildTid(n), kt);
                kt = N::getAnyChildTid(n);
                load_flag = true;
            }
            uint8_t startLevel =
                (start->getKeyLen() > level) ? start->fkey[level] : (uint8_t)0;
            uint8_t endLevel =
                (end->getKeyLen() > level) ? end->fkey[level] : (uint8_t)0;

#ifdef KEY_INLINE
            uint8_t curKey = i >= maxStoredPrefixLength ? (uint8_t)kt->kv[level]
                                                        : p.prefix[i];
#else
            uint8_t curKey =
                i >= maxStoredPrefixLength ? kt->fkey[level] : p.prefix[i];
#endif
            if (curKey > startLevel && curKey < endLevel) {
                return PCEqualsResults::Contained;
            } else if (curKey < startLevel || curKey > endLevel) {
                return PCEqualsResults::NoMatch;
            }
            ++level;
        }
    }
    return PCEqualsResults::BothMatch;
}
// void Tree::graphviz_debug() {
//     std::ofstream f("../dot/tree-view.dot");

//     f << "graph tree\n"
//          "{\n"
//          "    graph[dpi = 400];\n"
//          "    label=\"Tree View\"\n"
//          "    node []\n";
//     N::graphviz_debug(f, root);
//     f << "}";
//     f.close();
//     //    printf("ok2\n");
// }

} // namespace PART_ns
