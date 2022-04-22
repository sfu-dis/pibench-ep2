#include "N256.h"
#include "LeafArray.h"
#include "N.h"
#include <algorithm>
#include <assert.h>

namespace PART_ns {

void N256::deleteChildren() {
#ifdef MEMORY_FOOTPRINT
    pmem_deallocated += sizeof(N256);
#endif
    for (uint64_t i = 0; i < 256; ++i) {
        N *child = N::clearDirty(children[i].load());
        if (child != nullptr) {
            N::deleteChildren(child);
            N::deleteNode(child);
        }
    }
}

bool N256::insert(uint8_t key, N *val, bool flush) {
    if (flush) {
        uint64_t oldp = (1ull << 56) | ((uint64_t)key << 48);
    }
    if (val == (void*)0x303030303030) {
        printf("0x303030303030 inserted!\n"); //segfault
        exit(1);
    }
    children[key].store(val, std::memory_order_seq_cst);
    if (flush) {
        flush_data((void *)&children[key], sizeof(std::atomic<N *>));
    }

    count++;
    return true;
}

void N256::change(uint8_t key, N *n) {

    children[key].store(n, std::memory_order_seq_cst);
    flush_data((void *)&children[key], sizeof(std::atomic<N *>));
}

N *N256::getChild(const uint8_t k) {
    N *child = children[k].load();
    return child;
}

bool N256::remove(uint8_t k, bool force, bool flush) {
    if (count <= 37 && !force) {
        return false;
    }

    children[k].store(nullptr, std::memory_order_seq_cst);
    flush_data((void *)&children[k], sizeof(std::atomic<N *>));
    count--;
    return true;
}

N *N256::getAnyChild() const {
    N *anyChild = nullptr;
    for (uint64_t i = 0; i < 256; ++i) {
        N *child = children[i].load();

        if (child != nullptr) {
            if (N::isLeaf(child)) {
                return child;
            }
            anyChild = child;
        }
    }
    return anyChild;
}

void N256::getChildren(uint8_t start, uint8_t end,
                       std::tuple<uint8_t, N *> children[],
                       uint32_t &childrenCount) {
    childrenCount = 0;
    for (unsigned i = start; i <= end; i++) {
        N *child = this->children[i].load();

        if (child != nullptr) {
            children[childrenCount] = std::make_tuple(i, child);
            childrenCount++;
        }
    }
}

uint32_t N256::getCount() const {
    uint32_t cnt = 0;
    for (uint32_t i = 0; i < 256 && cnt < 3; i++) {
        N *child = children[i].load();
        if (child != nullptr)
            cnt++;
    }
    return cnt;
}
// void N256::graphviz_debug(std::ofstream &f) {
//     char buf[10000] = {};
//     sprintf(buf + strlen(buf), "node%lx [label=\"",
//             reinterpret_cast<uintptr_t>(this));
//     sprintf(buf + strlen(buf), "N256 %d\n",level);
//     auto pre = this->getPrefi();
//     sprintf(buf + strlen(buf), "Prefix Len: %d\n", pre.prefixCount);
//     sprintf(buf + strlen(buf), "Prefix: ");
//     for (int i = 0; i < std::min(pre.prefixCount, maxStoredPrefixLength); i++) {
//         sprintf(buf + strlen(buf), "%c ", pre.prefix[i]);
//     }
//     sprintf(buf + strlen(buf), "\n");
//     sprintf(buf + strlen(buf), "count: %d\n", count);
//     sprintf(buf + strlen(buf), "compact: %d\n", compactCount);
//     sprintf(buf + strlen(buf), "\"]\n");

//     for (auto i = 0; i < 256; i++) {
//         auto p = children[i].load();
//         if (p != nullptr) {
//             auto x = i;
//             auto addr = reinterpret_cast<uintptr_t>(p);
//             if (isLeaf(p)) {
//                 addr = reinterpret_cast<uintptr_t>(getLeaf(p));
//             }
//             sprintf(buf + strlen(buf), "node%lx -- node%lx [label=\"%c\"]\n",
//                     reinterpret_cast<uintptr_t>(this), addr, x);
//         }
//     }
//     f << buf;

//     for (auto &i : children) {
//         auto p = i.load();
//         if (p != nullptr) {
//             if (isLeaf(p)) {
// #ifdef LEAF_ARRAY
//                 auto la = getLeafArray(p);
//                 la->graphviz_debug(f);
// #else
//                 auto l = getLeaf(p);
//                 l->graphviz_debug(f);
// #endif
//             } else {
//                 N::graphviz_debug(f, p);
//             }
//         }
//     }
// }
} // namespace PART_ns
