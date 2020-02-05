//
// Created by florian on 18.11.15.
//

#ifndef ART_LOCK_COUPLING_TREE_H
#define ART_LOCK_COUPLING_TREE_H

#include "N.h"

using namespace ART;

namespace ART_LC {

    class Tree {
    public:
        using LoadKeyFunction = void (*)(TID tid, Key &key);

    private:
        persistent_ptr<N> root;

        TID checkKey(pool_base &pop, const TID tid, const Key &k) const;

        p<LoadKeyFunction> loadKey;

        Epoche epoche{256};

    public:
        enum class CheckPrefixResult : uint8_t {
            Match,
            NoMatch,
            OptimisticMatch
        };

        enum class CheckPrefixPessimisticResult : uint8_t {
            Match,
            NoMatch,
        };

        enum class PCCompareResults : uint8_t {
            Smaller,
            Equal,
            Bigger,
        };
        enum class PCEqualsResults : uint8_t {
            BothMatch,
            Contained,
            NoMatch
        };
        static CheckPrefixResult checkPrefix(N* n, const Key &k, uint32_t &level);

        static CheckPrefixPessimisticResult checkPrefixPessimistic(N *n, const Key &k, uint32_t &level,
                                                                   uint8_t &nonMatchingKey,
                                                                   Prefix &nonMatchingPrefix,
                                                                   LoadKeyFunction loadKey, bool &needRestart);

        static PCCompareResults checkPrefixCompare(N* n, const Key &k, uint8_t fillKey, uint32_t &level, LoadKeyFunction loadKey, bool &needRestart);

        static PCEqualsResults checkPrefixEquals(N* n, uint32_t &level, const Key &start, const Key &end, LoadKeyFunction loadKey, bool &needRestart);

    public:

        Tree(LoadKeyFunction loadKey);

        Tree(const Tree &) = delete;

        Tree(Tree &&t) : root(t.root), loadKey(t.loadKey) { }

        ~Tree();

        ThreadInfo getThreadInfo();

        TID lookup(pool_base &pop, const Key &k, ThreadInfo &threadEpocheInfo) const;

        bool lookupRange(pool_base &pop, const Key &start, const Key &end, Key &continueKey, TID result[], std::size_t resultLen,
                         std::size_t &resultCount, ThreadInfo &threadEpocheInfo) const;

        void insert(pool_base &pop, const Key &k, TID tid, ThreadInfo &epocheInfo);

        void remove(pool_base &pop, const Key &k, TID tid, ThreadInfo &epocheInfo);
    };
}
#endif //ART_LOCK_COUPLING_N_H
