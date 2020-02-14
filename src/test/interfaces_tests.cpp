// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

using interfaces::FoundBlock;

BOOST_FIXTURE_TEST_SUITE(interfaces_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(findBlock)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    int64_t time_mtp = -1;
    BOOST_CHECK(chain->findBlock(active[20]->GetBlockHash(), FoundBlock().mtpTime(time_mtp)));
    BOOST_CHECK_EQUAL(time_mtp, active[20]->GetMedianTimePast());
}

BOOST_AUTO_TEST_CASE(findFirstBlockWithTimeAndHeight)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    uint256 hash;
    int height;
    BOOST_CHECK(chain->findFirstBlockWithTimeAndHeight(/* min_time= */ 0, /* min_height= */ 5, FoundBlock().hash(hash).height(height)));
    BOOST_CHECK_EQUAL(hash, active[5]->GetBlockHash());
    BOOST_CHECK_EQUAL(height, 5);
    BOOST_CHECK(!chain->findFirstBlockWithTimeAndHeight(/* min_time= */ active.Tip()->GetBlockTimeMax() + 1, /* min_height= */ 0));
}

BOOST_AUTO_TEST_CASE(findNextBlock)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    bool reorg;
    uint256 hash;
    BOOST_CHECK(chain->findNextBlock(active[20]->GetBlockHash(), 20, FoundBlock().hash(hash), &reorg));
    BOOST_CHECK_EQUAL(hash, active[21]->GetBlockHash());
    BOOST_CHECK_EQUAL(reorg, false);
    BOOST_CHECK(!chain->findNextBlock(uint256(), 20, {}, &reorg));
    BOOST_CHECK_EQUAL(reorg, true);
    BOOST_CHECK(!chain->findNextBlock(active.Tip()->GetBlockHash(), active.Height(), {}, &reorg));
    BOOST_CHECK_EQUAL(reorg, false);
}

BOOST_AUTO_TEST_CASE(findAncestorByHeight)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    uint256 hash;
    BOOST_CHECK(chain->findAncestorByHeight(active[20]->GetBlockHash(), 10, FoundBlock().hash(hash)));
    BOOST_CHECK_EQUAL(hash, active[10]->GetBlockHash());
    BOOST_CHECK(!chain->findAncestorByHeight(active[10]->GetBlockHash(), 20));
}

BOOST_AUTO_TEST_CASE(findAncestorByHash)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    int height = -1;
    BOOST_CHECK(chain->findAncestorByHash(active[20]->GetBlockHash(), active[10]->GetBlockHash(), FoundBlock().height(height)));
    BOOST_CHECK_EQUAL(height, 10);
    BOOST_CHECK(!chain->findAncestorByHash(active[10]->GetBlockHash(), active[20]->GetBlockHash()));
}

BOOST_AUTO_TEST_CASE(findCommonAncestor)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    auto* orig_tip = active.Tip();
    for (int i = 0; i < 10; ++i) {
        BlockValidationState state;
        ChainstateActive().InvalidateBlock(state, Params(), active.Tip());
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight - 10);
    coinbaseKey.MakeNewKey(true);
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight + 10);
    uint256 fork_hash;
    int fork_height;
    int orig_height;
    BOOST_CHECK(chain->findCommonAncestor(orig_tip->GetBlockHash(), active.Tip()->GetBlockHash(), FoundBlock().height(fork_height).hash(fork_hash), FoundBlock().height(orig_height)));
    BOOST_CHECK_EQUAL(orig_height, orig_tip->nHeight);
    BOOST_CHECK_EQUAL(fork_height, orig_tip->nHeight - 10);
    BOOST_CHECK_EQUAL(fork_hash, active[fork_height]->GetBlockHash());
}

BOOST_AUTO_TEST_CASE(hasBlocks)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ChainActive();
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[5]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[95]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[50]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
}

BOOST_AUTO_TEST_SUITE_END()
