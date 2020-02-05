//
// Created by florian on 05.08.15.
//

#ifndef ART_LOCK_COUPLING_N_H
#define ART_LOCK_COUPLING_N_H
//#define ART_NOREADLOCK
//#define ART_NOWRITELOCK
#include <stdint.h>
#include <atomic>
#include <string.h>
#include "../../Include/Key.h"
#include "../../Include/Epoche.h"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#define LAYOUT "LOCK"
using namespace pmem;
using namespace pmem::obj;

using TID = uint64_t;

using namespace ART ;
namespace ART_LC {
/*
 * SynchronizedTree
 * LockCouplingTree
 * LockCheckFreeReadTree
 * UnsynchronizedTree
 */

    enum class NTypes : uint8_t {
        N4 = 0,
        N16 = 1,
        N48 = 2,
        N256 = 3
    };

    static constexpr uint32_t maxStoredPrefixLength = 11;

    using Prefix = uint8_t[maxStoredPrefixLength];

    class N {
    protected:
        N(NTypes type, const uint8_t *prefix, uint32_t prefixLength) {
            setType(type);
            setPrefix(prefix, prefixLength);
        }

        N(const N &) = delete;

        N(N &&) = delete;

        //2b type 60b version 1b lock 1b obsolete
        p<std::atomic<uint64_t>> typeVersionLockObsolete{0b100};
        p<std::atomic<uint8_t>> mutex{0b00};
        p<int> readerCount = 0;
        // version 1, unlocked, not obsolete
        p<uint32_t> prefixCount = 0;

        p<uint8_t> count = 0;
        p<Prefix> prefix;


        void setType(pool_base &pop, NTypes type);

        static uint64_t convertTypeToVersion(NTypes type);

    public:

        NTypes getType() const;

        uint32_t getCount() const;

        bool isLocked(uint64_t version) const;

        bool mutexLocked(uint8_t mutexVal) const;

        void writeLockOrRestart(bool &needRestart);

        void upgradeToWriteLockOrRestart(pool_base &pop, uint64_t &version, bool &needRestart);

        void writeUnlock(pool_base &pop);

/*        void increment();

        void decrement();

        void lockMutex();

        void unlockMutex();
*/
        uint64_t readLockOrRestart(pool_base &pop, bool &needRestart); 

        /**
         * returns true if node hasn't been changed in between
         */
        void checkOrRestart(uint64_t startRead, bool &needRestart);
        void readUnlockOrRestart(pool_base &pop, uint64_t startRead, bool &needRestart);

        static bool isObsolete(uint64_t version);

        /**
         * can only be called when node is locked
         */
        void writeUnlockObsolete(pool_base &pop) {
		transaction::run(pop, [&]{
	            typeVersionLockObsolete.fetch_add(0b11);
		});
        }

        static N *getChild(const uint8_t k, const N *node);

        static void insertAndUnlock(pool_base &pop, N *node, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, N *val, bool &needRestart,
                                    ThreadInfo &threadInfo);

        static bool change(N *node, uint8_t key, N *val);

        static void removeAndUnlock(N *node, uint64_t v, uint8_t key, N *parentNode, uint64_t parentVersion, uint8_t keyParent, bool &needRestart, ThreadInfo &threadInfo);

        bool hasPrefix() const;

        const uint8_t *getPrefix() const;

        void setPrefix(pool_base &pop, const uint8_t *prefix, uint32_t length);

        void addPrefixBefore(pool_base &pop, N *node, uint8_t key);

        uint32_t getPrefixLength() const;

        static TID getLeaf(const N *n);

        static bool isLeaf(const N *n);

        static N *setLeaf(TID tid);

        static N *getAnyChild(N *n);

        static TID getAnyChildTid(N *n, bool &needRestart);

        static void deleteChildren(N *node);

        static void deleteNode(pool_base &pop, N *node);

        static std::tuple<N *, uint8_t> getSecondChild(N *node, const uint8_t k);

        template<typename curN, typename biggerN>
        static void insertGrow(pool_base &pop, curN *n, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, N *val, bool &needRestart, ThreadInfo &threadInfo);

        template<typename curN, typename smallerN>
        static void removeAndShrink(curN *n, uint64_t v, N *parentNode, uint64_t parentVersion, uint8_t keyParent, uint8_t key, bool &needRestart, ThreadInfo &threadInfo);

        static uint64_t getChildren(N *node, uint8_t start, uint8_t end, std::tuple<uint8_t, N *> children[],
                                uint32_t &childrenCount);
    };

    class N4 : public N {
    public:
        p<uint8_t> keys[4];
        persistent_ptr<N> children[4] = {nullptr, nullptr, nullptr, nullptr};

    public:
        N4(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N4, prefix,
                                                                             prefixLength) { }

        void insert(pool_base &pop, uint8_t key, N *n);

        template<class NODE>
        void copyTo(pool_base &pop, NODE n) const;

        bool change(pool_base &pop, uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(pool_base &pop, uint8_t k);

        N *getAnyChild() ;

        bool isFull() const;

        bool isUnderfull() const;

        std::tuple<N *, uint8_t> getSecondChild(const uint8_t key) const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) ;
    };

    class N16 : public N {
    public:
        p<uint8_t> keys[16];
        persistent_ptr<N> children[16];

        static uint8_t flipSign(uint8_t keyByte) {
            // Flip the sign bit, enables signed SSE comparison of unsigned values, used by Node16
            return keyByte ^ 128;
        }

        static inline unsigned ctz(uint16_t x) {
            // Count trailing zeros, only defined for x>0
#ifdef __GNUC__
            return __builtin_ctz(x);
#else
            // Adapted from Hacker's Delight
   unsigned n=1;
   if ((x&0xFF)==0) {n+=8; x=x>>8;}
   if ((x&0x0F)==0) {n+=4; x=x>>4;}
   if ((x&0x03)==0) {n+=2; x=x>>2;}
   return n-(x&1);
#endif
        }

        N *const *getChildPos(const uint8_t k) const;

    public:
        N16(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N16, prefix,
                                                                              prefixLength) {
            memset(keys, 0, sizeof(keys));
            memset(children, 0, sizeof(children));
        }

        void insert(pool_base &pop, uint8_t key, N *n);

        template<class NODE>
        void copyTo(NODE n) const;

        bool change(uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(pool_base &pop, uint8_t k);

        N *getAnyChild() ;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) ;
    };

    class N48 : public N {
        p<uint8_t> childIndex[256];
        persistent_ptr<N> children[48];
    public:
        static const uint8_t emptyMarker = 48;

        N48(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N48, prefix,
                                                                              prefixLength) {
            memset(childIndex, emptyMarker, sizeof(childIndex));
            memset(children, 0, sizeof(children));
        }

        void insert(pool_base &pop, uint8_t key, N *n);

        template<class NODE>
        void copyTo(NODE n) const;

        bool change(pool_base &pop, uint8_t key, N *val);

        N *getChild(const uint8_t k) const;

        void remove(pool_base &pop, uint8_t k);

        N *getAnyChild() ;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) ;
    };

    class N256 : public N {
        persistent_ptr<N> children[256];

    public:
        N256(const uint8_t *prefix, uint32_t prefixLength) : N(NTypes::N256, prefix,
                                                                               prefixLength) {
            memset(children, '\0', sizeof(children));
        }

        void insert(pool_base &pop, uint8_t key, N *val);

        template<class NODE>
        void copyTo(NODE n) const;

        bool change(pool_base &pop, uint8_t key, N *n);

        N *getChild(const uint8_t k) const;

        void remove(pool_base &pop, uint8_t k);

        N *getAnyChild() ;

        bool isFull() const;

        bool isUnderfull() const;

        void deleteChildren();

        uint64_t getChildren(uint8_t start, uint8_t end, std::tuple<uint8_t, N *> *&children,
                         uint32_t &childrenCount) ;
    };
}
#endif //ART_OPTIMISTIC_LOCK_COUPLING_N_H
