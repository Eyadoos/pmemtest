#include <assert.h>
#include <algorithm>
#include "Include/N.h"

namespace ART_LC {

    bool N256::isFull() const {
        return false;
    }

    bool N256::isUnderfull() const {
        return count == 37;
    }

    void N256::deleteChildren() {
        for (uint64_t i = 0; i < 256; ++i) {
            if (children[i] != nullptr) {
                N::deleteChildren(children[i]);
                N::deleteNode(children[i]);
            }
        }
    }

    void N256::insert(pool_base &pop, uint8_t key, N *val) {
	transaction::run(pop, [&]{
	        children[key] = val;
        	count++;
	});
    }

    template<class NODE>
    void N256::copyTo(NODE n) const {
        for (int i = 0; i < 256; ++i) {
            if (children[i] != nullptr) {
                n->insert(i, children[i]);
            }
        }
    }

    bool N256::change(pool_base &pop, uint8_t key, N *n) {
	transaction::run(pop, [&]{
	        children[key] = n;
	});
        return true;
    }

    N *N256::getChild(const uint8_t k) const {
        return children[k];
    }

    void N256::remove(pool_base &pop, uint8_t k) {
	transaction::run(pop, [&]{
	        children[k] = nullptr;
        	count--;
	});
    }

    N *N256::getAnyChild() {
        N *anyChild = nullptr;
        for (uint64_t i = 0; i < 256; ++i) {
            if (children[i] != nullptr) {
                if (N::isLeaf(children[i])) {
                    return children[i];
                } else {
                    anyChild = children[i];
                }
            }
        }
        return anyChild;
    }

    uint64_t N256::getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                           uint32_t &childrenCount) {
        restart:
        bool needRestart = false;
        uint64_t v;
        v = readLockOrRestart(needRestart);
        if (needRestart) goto restart;
        childrenCount = 0;
        for (unsigned i = start; i <= end; i++) {
            if (this->children[i] != nullptr) {
                children[childrenCount] = std::make_tuple(i, this->children[i]);
                childrenCount++;
            }
        }
        readUnlockOrRestart(v, needRestart);
        if (needRestart) goto restart;
        return v;
    }
}
