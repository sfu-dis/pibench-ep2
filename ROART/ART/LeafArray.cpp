//
// Created by 谢威宇 on 2021/4/3.
//

#include "LeafArray.h"
#include "EpochGuard.h"
#include "threadinfo.h"

using namespace NVMMgr_ns;
namespace PART_ns {
size_t PART_ns::LeafArray::getRightmostSetBit() const {
    auto b = bitmap.load();
    auto pos = b._Find_first();
    assert(pos < LeafArrayLength);
    return pos;
}
void PART_ns::LeafArray::setBit(size_t bit_pos, bool to) {
    auto b = bitmap.load();
    b[bit_pos] = to;
    bitmap.store(b);
}
uint16_t PART_ns::LeafArray::getFingerPrint(size_t pos) const {
    auto x = reinterpret_cast<uint64_t>(leaf[pos].load());
    uint16_t re = x >> FingerPrintShift;
    return re;
}
Leaf *LeafArray::lookup(const Key *k) const {
    uint16_t finger_print = k->getFingerPrint();

    auto b = bitmap.load();

#ifdef FIND_FIRST
    auto i = b[0] ? 0 : 1;
    while (i < LeafArrayLength) {
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            if (finger_print == thisfp && ptr->checkKey(k)) {
                return ptr;
            }
        }
        i = b._Find_next(i);
    }
#else
    for (int i = 0; i < LeafArrayLength; i++) {
        if (b[i] == false)
            continue;
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            if (finger_print == thisfp && ptr->checkKey(k)) {
                return ptr;
            }
        }
    }
#endif

    return nullptr;
}
bool LeafArray::insert(Leaf *l, bool flush) {
    auto b = bitmap.load();
    b.flip();
    auto pos = b._Find_first();
    if (pos < LeafArrayLength) {
        b.flip();
        b[pos] = true;
        bitmap.store(b);
        auto s =
            (static_cast<uintptr_t>(l->getFingerPrint()) << FingerPrintShift) |
            (reinterpret_cast<uintptr_t>(l));
        leaf[pos].store(s);
        if (flush)
            flush_data((void *)&leaf[pos], sizeof(std::atomic<uintptr_t>));

        return true;
    } else {
        return false;
    }
}
bool LeafArray::remove(const Key *k) {
    uint16_t finger_print = k->getFingerPrint();
    auto b = bitmap.load();
    auto i = b[0] ? 0 : 1;
    while (i < LeafArrayLength) {
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            if (finger_print == thisfp && ptr->checkKey(k)) {
                leaf[i].store(0);
                flush_data(&leaf[i], sizeof(std::atomic<uintptr_t>));
            #ifdef MEMORY_FOOTPRINT
                pmem_deallocated += sizeof(PART_ns::NTypes::Leaf);
            #endif
                EpochGuard::DeleteNode(ptr);
                b[i] = false;
                bitmap.store(b);
                return true;
            }
        }
        i = b._Find_next(i);
    }
    return false;
}
void LeafArray::reload() {
    auto b = bitmap.load();
    for (int i = 0; i < LeafArrayLength; i++) {
        if (leaf[i].load() != 0) {
            b[i] = true;
        } else {
            b[i] = false;
        }
    }
    bitmap.store(b);
}
// void LeafArray::graphviz_debug(std::ofstream &f) {
//     char buf[1000] = {};
//     sprintf(buf + strlen(buf), "node%lx [label=\"",
//             reinterpret_cast<uintptr_t>(this));
//     sprintf(buf + strlen(buf), "LeafArray\n");
//     sprintf(buf + strlen(buf), "count: %zu\n", bitmap.load().count());
//     sprintf(buf + strlen(buf), "\"]\n");

//     auto b = bitmap.load();
//     auto i = b[0] ? 0 : 1;
//     while (i < LeafArrayLength) {
//         auto fingerprint_ptr = this->leaf[i].load();
//         if (fingerprint_ptr != 0) {
//             uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
//             auto ptr = reinterpret_cast<Leaf *>(
//                 fingerprint_ptr ^
//                 (static_cast<uintptr_t>(thisfp) << FingerPrintShift));

//             sprintf(buf + strlen(buf), "node%lx -- node%lx \n",
//                     reinterpret_cast<uintptr_t>(this),
//                     reinterpret_cast<uintptr_t>(ptr));
//         }
//         i = b._Find_next(i);
//     }

//     f << buf;

//     b = bitmap.load();
//     i = b[0] ? 0 : 1;
//     while (i < LeafArrayLength) {
//         auto fingerprint_ptr = this->leaf[i].load();
//         if (fingerprint_ptr != 0) {
//             uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
//             auto ptr = reinterpret_cast<Leaf *>(
//                 fingerprint_ptr ^
//                 (static_cast<uintptr_t>(thisfp) << FingerPrintShift));

//             ptr->graphviz_debug(f);
//         }
//         i = b._Find_next(i);
//     }
// }

void LeafArray::splitAndUnlock(N *parentNode, uint8_t parentKey,
                               bool &need_restart) {

    parentNode->writeLockOrRestart(need_restart);

    if (need_restart) {
        this->writeUnlock();
        return;
    }

    auto b = bitmap.load();
    auto leaf_count = b.count();
    // printf("# of leaves: %d\n", leaf_count);
    std::vector<char *> keys;
    //    char **keys = new char *[leaf_count];
    std::vector<int> lens;
    //    int *lens = new int[leaf_count];
    int level = 0;
    level = parentNode->getLevel() + 1;

    auto i = b[0] ? 0 : 1;
    while (i < LeafArrayLength) {
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            // keys.push_back(ptr->GetKey());
            // lens.push_back(ptr->key_len);
            if (ptr->key_len > level) {
                keys.push_back(ptr->GetKey());
                lens.push_back(ptr->key_len);
            }
            else {
                // printf("replace prefix key at index: %d!\n", i);
                this->leaf[i].store(leaf[leaf_count-1].load());
                bitmap.store(b.reset(leaf_count-1));
                leaf_count--;
                // b = bitmap.load();
                if (i != leaf_count-1) {
                    continue;
                }
            } 
        }
        i = b._Find_next(i);
    }
    //    printf("spliting\n");

    std::vector<char> common_prefix;
    
    // assume keys are not substring of another key

    // todo: get common prefix can be optimized by binary search
    while (true) {
        bool out = false;
        for (i = 0; i < leaf_count; i++) {
            if (level < lens[i]) {
                if (i == 0) {
                    common_prefix.push_back(keys[i][level]);
                } else {
                    if (keys[i][level] != common_prefix.back()) {

                        common_prefix.pop_back();

                        out = true;
                        break;
                    }
                }
            } else {
                // assume keys are not substring of another key
                //printf("assert0\n");
                // assert(0);
            }
        }
        if (out)
            break;
        level++;
    }
    // printf("while loop finished!\n");
    std::map<char, LeafArray *> split_array;
    for (i = 0; i < leaf_count; i++) {
        // printf("accessing leaf at idx: %d\n", i);
        if (split_array.count(keys[i][level]) == 0) {
            split_array[keys[i][level]] =
                new (alloc_new_node_from_type(NTypes::LeafArray))
                    LeafArray(level);
        }
        split_array.at(keys[i][level])->insert(getLeafAt(i), false);
    }
    // printf("for loop finished!\n");
    N *n;
    uint8_t *prefix_start = reinterpret_cast<uint8_t *>(common_prefix.data());
    auto prefix_len = common_prefix.size();
    auto leaf_array_count = split_array.size();
    if (leaf_array_count <= 4) {
        n = new (alloc_new_node_from_type(NTypes::N4))
            N4(level, prefix_start, prefix_len);
    } else if (leaf_array_count > 4 && leaf_array_count <= 16) {
        n = new (alloc_new_node_from_type(NTypes::N16))
            N16(level, prefix_start, prefix_len);
    } else if (leaf_array_count > 16 && leaf_array_count <= 48) {
        n = new (alloc_new_node_from_type(NTypes::N48))
            N48(level, prefix_start, prefix_len);
    } else if (leaf_array_count > 48 && leaf_array_count <= 256) {
        n = new (alloc_new_node_from_type(NTypes::N256))
            N256(level, prefix_start, prefix_len);
    } else {
        assert(0);
    }
    for (const auto &p : split_array) {
        unchecked_insert(n, p.first, setLeafArray(p.second), true);
        flush_data(p.second, sizeof(LeafArray));
    }
    // printf("ifs finished!\n");
    N::change(parentNode, parentKey, n);
    parentNode->writeUnlock();

    this->writeUnlockObsolete();
#ifdef MEMORY_FOOTPRINT
    pmem_deallocated += sizeof(PART_ns::NTypes::LeafArray);
#endif
    EpochGuard::DeleteNode(this);
    // printf("split finished\n");
}
Leaf *LeafArray::getLeafAt(size_t pos) const {
    auto t = reinterpret_cast<uintptr_t>(this->leaf[pos].load());
    t = (t << 16) >> 16;
    return reinterpret_cast<Leaf *>(t);
}
uint32_t LeafArray::getCount() const { return bitmap.load().count(); }
bool LeafArray::isFull() const { return getCount() == LeafArrayLength; }
std::vector<Leaf *> LeafArray::getSortedLeaf(const Key *start, const Key *end,
                                             int start_level,
                                             bool compare_start,
                                             bool compare_end) {
    std::vector<Leaf *> leaves;
    auto b = bitmap.load();
    auto i = b[0] ? 0 : 1;

    while (i < LeafArrayLength) {
        auto ptr = getLeafAt(i);
        i = b._Find_next(i);
        // start <= ptr < end
        if (compare_start) {
            auto lt_start = leaf_key_lt(ptr, start, start_level);
            if (lt_start == true) {
                continue;
            }
        }
        if (compare_end) {
            auto lt_end = leaf_key_lt(ptr, end, start_level);
            if (lt_end == false) {
                continue;
            }
        }
        leaves.push_back(ptr);
    }
#ifdef SORT_LEAVES
    std::sort(leaves.begin(), leaves.end(),
              [start_level](Leaf *a, Leaf *b) -> bool {
                  leaf_lt(a, b, start_level);
              });
#endif
    return leaves;
}
bool LeafArray::update(const Key *k, Leaf *l) {
    uint16_t finger_print = k->getFingerPrint();
    auto b = bitmap.load();

#ifdef FIND_FIRST
    auto i = b[0] ? 0 : 1;
    while (i < LeafArrayLength) {
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            if (finger_print == thisfp && ptr->checkKey(k)) {
                auto news = fingerPrintLeaf(finger_print, l);
                leaf[i].store(news);
                flush_data(&leaf[i], sizeof(std::atomic<uintptr_t>));
                return true;
            }
        }
        i = b._Find_next(i);
    }
#else
    for (int i = 0; i < LeafArrayLength; i++) {
        if (b[i] == false)
            continue;
        auto fingerprint_ptr = this->leaf[i].load();
        if (fingerprint_ptr != 0) {
            uint16_t thisfp = fingerprint_ptr >> FingerPrintShift;
            auto ptr = reinterpret_cast<Leaf *>(
                fingerprint_ptr ^
                (static_cast<uintptr_t>(thisfp) << FingerPrintShift));
            if (finger_print == thisfp && ptr->checkKey(k)) {
                auto news = fingerPrintLeaf(finger_print, l);
                leaf[i].store(news);
                flush_data(&leaf[i], sizeof(std::atomic<uintptr_t>));
                return true;
            }
        }
    }
#endif
    return false;
}
uintptr_t LeafArray::fingerPrintLeaf(uint16_t fingerPrint, Leaf *l) {
    uintptr_t mask = (1LL << FingerPrintShift) - 1;
    auto f = uintptr_t(fingerPrint);
    return (reinterpret_cast<uintptr_t>(l) & mask) | (f << FingerPrintShift);
}
N *LeafArray::getAnyChild() const {
    auto b = bitmap.load();
    auto i = b._Find_first();
    if (i == LeafArrayLength) {
        return nullptr;
    } else {
        return N::setLeaf(getLeafAt(i));
    }
}

} // namespace PART_ns
