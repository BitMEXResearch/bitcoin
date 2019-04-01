// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CHAIN_H
#define BITCOIN_INTERFACES_CHAIN_H

#include <optional.h>               // For Optional and nullopt
#include <primitives/transaction.h> // For CTransactionRef

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class CBlock;
class CFeeRate;
class CRPCCommand;
class CScheduler;
class Coin;
class uint256;
enum class RBFTransactionState;
struct CBlockLocator;
struct FeeCalculation;
struct NodeContext;

namespace interfaces {

class Handler;
class Wallet;

//! Interface giving clients (wallet processes, maybe other analysis tools in
//! the future) ability to access to the chain state, receive notifications,
//! estimate fees, and submit transactions.
//!
//! TODO: Current chain methods are too low level, exposing too much of the
//! internal workings of the bitcoin node, and not being very convenient to use.
//! Chain methods should be cleaned up and simplified over time. Examples:
//!
//! * The Chain::lock() method, which lets clients delay chain tip updates
//!   should be removed when clients are able to respond to updates
//!   asynchronously
//!   (https://github.com/bitcoin/bitcoin/pull/10973#issuecomment-380101269).
//!
//! * The initMessage() and showProgress() methods which the wallet uses to send
//!   notifications to the GUI should go away when GUI and wallet can directly
//!   communicate with each other without going through the node
//!   (https://github.com/bitcoin/bitcoin/pull/15288#discussion_r253321096).
//!
//! * The handleRpc, registerRpcs, rpcEnableDeprecated methods and other RPC
//!   methods can go away if wallets listen for HTTP requests on their own
//!   ports instead of registering to handle requests on the node HTTP port.
class Chain
{
public:
    virtual ~Chain() {}

    //! Interface for querying locked chain state, used by legacy code that
    //! assumes state won't change between calls. New code should avoid using
    //! the Lock interface and instead call higher-level Chain methods
    //! that return more information so the chain doesn't need to stay locked
    //! between calls.
    class Lock
    {
    public:
        virtual ~Lock() {}

        //! Get current chain height, not including genesis block (returns 0 if
        //! chain only contains genesis block, nullopt if chain does not contain
        //! any blocks).
        virtual Optional<int> getHeight() = 0;

        //! Get block height above genesis block. Returns 0 for genesis block,
        //! 1 for following block, and so on. Returns nullopt for a block not
        //! included in the current chain.
        virtual Optional<int> getBlockHeight(const uint256& hash) = 0;

        //! Get locator for the current chain tip.
        virtual CBlockLocator getTipLocator() = 0;

        //! Check if transaction will be final given chain height current time.
        virtual bool checkFinalTx(const CTransaction& tx) = 0;
    };

    //! Return Lock interface. Chain is locked when this is called, and
    //! unlocked when the returned interface is freed.
    virtual std::unique_ptr<Lock> lock(bool try_lock = false) = 0;

    //! Return whether node has the block and optionally return block metadata
    //! or contents.
    //!
    //! If a block pointer is provided to retrieve the block contents, and the
    //! block exists but doesn't have data (for example due to pruning), the
    //! block will be empty and all fields set to null.
    virtual bool findBlock(const uint256& hash,
        CBlock* block = nullptr,
        int64_t* time = nullptr,
        int64_t* max_time = nullptr,
        int64_t* mtp_time = nullptr) = 0;

    //! Return hash of first block in the chain with timestamp >= the given time
    //! and height >= than the given height, or nullopt if there is no block
    //! with a high enough timestamp and height. Also return the block height as
    //! an optional output parameter (to avoid the cost of a second lookup in
    //! case this information is needed.)
    virtual Optional<uint256> findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, int* height = nullptr) = 0;

    //! Get hash of next block if block is part of current chain. Also flag if
    //! there was a reorg and the specified block hash is not even on the
    //! current chain.
    virtual Optional<uint256> findNextBlock(const uint256& block_hash, int block_height, bool& reorg) = 0;

    //! Find ancestor of block at specified height and return its hash.
    virtual uint256 findAncestorByHeight(const uint256& block_hash, int ancestor_height) = 0;

    //! Return whether block descends from a specified ancestor, and
    //! optionally return height of the ancestor.
    virtual bool findAncestorByHash(const uint256& block_hash,
        const uint256& ancestor_hash,
        int* height = 0) = 0;

    //! Find most recent common ancestor between two blocks and optionally
    //! return its hash and/or height. Also return height of first block. Return
    //! nullopt if either block is unknown or there is no common ancestor.
    virtual Optional<int> findFork(const uint256& block_hash1,
        const uint256& block_hash2,
        uint256* ancestor_hash,
        int* ancestor_height) = 0;

    //! Look up unspent output information. Returns coins in the mempool and in
    //! the current chain UTXO set. Iterates through all the keys in the map and
    //! populates the values.
    virtual void findCoins(std::map<COutPoint, Coin>& coins) = 0;

    //! Estimate fraction of total transactions verified if blocks up to
    //! the specified block hash are verified.
    virtual double guessVerificationProgress(const uint256& block_hash) = 0;

    //! Return true if data is available for the specified blocks.
    virtual bool hasBlocks(const uint256& block_hash, Optional<int> min_height, Optional<int> max_height) = 0;

    //! Check if transaction is RBF opt in.
    virtual RBFTransactionState isRBFOptIn(const CTransaction& tx) = 0;

    //! Check if transaction has descendants in mempool.
    virtual bool hasDescendantsInMempool(const uint256& txid) = 0;

    //! Transaction is added to memory pool, if the transaction fee is below the
    //! amount specified by max_tx_fee, and broadcast to all peers if relay is set to true.
    //! Return false if the transaction could not be added due to the fee or for another reason.
    virtual bool broadcastTransaction(const CTransactionRef& tx, std::string& err_string, const CAmount& max_tx_fee, bool relay) = 0;

    //! Calculate mempool ancestor and descendant counts for the given transaction.
    virtual void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants) = 0;

    //! Get the node's package limits.
    //! Currently only returns the ancestor and descendant count limits, but could be enhanced to
    //! return more policy settings.
    virtual void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) = 0;

    //! Check if transaction will pass the mempool's chain limits.
    virtual bool checkChainLimits(const CTransactionRef& tx) = 0;

    //! Estimate smart fee.
    virtual CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc = nullptr) = 0;

    //! Fee estimator max target.
    virtual unsigned int estimateMaxBlocks() = 0;

    //! Mempool minimum fee.
    virtual CFeeRate mempoolMinFee() = 0;

    //! Relay current minimum fee (from -minrelaytxfee and -incrementalrelayfee settings).
    virtual CFeeRate relayMinFee() = 0;

    //! Relay incremental fee setting (-incrementalrelayfee), reflecting cost of relay.
    virtual CFeeRate relayIncrementalFee() = 0;

    //! Relay dust fee setting (-dustrelayfee), reflecting lowest rate it's economical to spend.
    virtual CFeeRate relayDustFee() = 0;

    //! Check if any block has been pruned.
    virtual bool havePruned() = 0;

    //! Check if the node is ready to broadcast transactions.
    virtual bool isReadyToBroadcast() = 0;

    //! Check if in IBD.
    virtual bool isInitialBlockDownload() = 0;

    //! Check if shutdown requested.
    virtual bool shutdownRequested() = 0;

    //! Get adjusted time.
    virtual int64_t getAdjustedTime() = 0;

    //! Send init message.
    virtual void initMessage(const std::string& message) = 0;

    //! Send init warning.
    virtual void initWarning(const std::string& message) = 0;

    //! Send init error.
    virtual void initError(const std::string& message) = 0;

    //! Send progress indicator.
    virtual void showProgress(const std::string& title, int progress, bool resume_possible) = 0;

    //! Chain notifications.
    class Notifications
    {
    public:
        virtual ~Notifications() {}
        virtual void TransactionAddedToMempool(const CTransactionRef& tx) {}
        virtual void TransactionRemovedFromMempool(const CTransactionRef& ptx) {}
        virtual void BlockConnected(const CBlock& block, const std::vector<CTransactionRef>& tx_conflicted, int height) {}
        virtual void BlockDisconnected(const CBlock& block, int height) {}
        virtual void UpdatedBlockTip() {}
        virtual void ChainStateFlushed(const CBlockLocator& locator) {}
    };

    using ScanFn = std::function<Optional<uint256>(const uint256& start_hash, int start_height, const uint256& tip_hash, int tip_height)>;
    using MempoolFn = std::function<void(std::vector<CTransactionRef>)>;

    //! Register handler for notifications. Before returning and before sending
    //! the first notification, call ScanFn with the hash of the first block
    //! after the provided previous sync location, and then call MempoolFn
    //! with a snapshot of transactions in the mempool the first
    //! TransactionAddedToMempool/TransactionRemovedFromMempool notifications.
    //!
    //! @param[in] notify        object to send notifications to after returning
    //! @param[in] scan_fn       callback invoked to scan any new blocks that
    //!                          were added before sending new notifications.
    //!                          This should return the hash of the last block
    //!                          scanned, and will be called more than once if
    //!                          the tip changes the call and the last block
    //!                          scanned is behind the chain tip at the point
    //!                          notifications would be attached.
    //! @param[in] mempool_fn    callback invoked with snapshot of the mempool at
    //!                          the point notifications are attached
    //! @param[in] scan_locator  location of last block already scanned. scan_fn
    //!                          will be only be called for new blocks after
    //!                          this point. If null scan may begin at the
    //!                          genesis block, depending on scan_time
    //! @param[in] scan_time     minimum block timestamp for beginning the scan.
    //!                          The first block with timestamp greater or equal
    //!                          to this time is eligible to be scanned,
    //!                          depending on scan_locator
    //! @param[out] tip_hash     hash of the tip at the point notifications will
    //!                          begin. The first block Connected / Disconnected
    //!                          notifications will be relative to this tip
    //! @param[out] tip_height   height of block at tip_hash
    //! @param[out] tip_locator  locator for tip_hash
    virtual std::unique_ptr<Handler> handleNotifications(Notifications& notifications,
        ScanFn scan_fn,
        MempoolFn mempool_fn,
        const CBlockLocator* scan_locator,
        int64_t scan_time,
        uint256& tip_hash,
        int& tip_height,
        CBlockLocator& tip_locator,
        bool& missing_block_data) = 0;

    //! Wait for pending notifications to be processed unless block hash points to the current
    //! chain tip.
    virtual void waitForNotificationsIfTipChanged(const uint256& old_tip) = 0;

    //! Register handler for RPC. Command is not copied, so reference
    //! needs to remain valid until Handler is disconnected.
    virtual std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) = 0;

    //! Check if deprecated RPC is enabled.
    virtual bool rpcEnableDeprecated(const std::string& method) = 0;

    //! Run function after given number of seconds. Cancel any previous calls with same name.
    virtual void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) = 0;

    //! Current RPC serialization flags.
    virtual int rpcSerializationFlags() = 0;
};

//! Interface to let node manage chain clients (wallets, or maybe tools for
//! monitoring and analysis in the future).
class ChainClient
{
public:
    virtual ~ChainClient() {}

    //! Register rpcs.
    virtual void registerRpcs() = 0;

    //! Check for errors before loading.
    virtual bool verify() = 0;

    //! Load saved state.
    virtual bool load() = 0;

    //! Start client execution and provide a scheduler.
    virtual void start(CScheduler& scheduler) = 0;

    //! Save state to disk.
    virtual void flush() = 0;

    //! Shut down client.
    virtual void stop() = 0;
};

//! Return implementation of Chain interface.
std::unique_ptr<Chain> MakeChain(NodeContext& node);

//! Return implementation of ChainClient interface for a wallet client. This
//! function will be undefined in builds where ENABLE_WALLET is false.
//!
//! Currently, wallets are the only chain clients. But in the future, other
//! types of chain clients could be added, such as tools for monitoring,
//! analysis, or fee estimation. These clients need to expose their own
//! MakeXXXClient functions returning their implementations of the ChainClient
//! interface.
std::unique_ptr<ChainClient> MakeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_CHAIN_H
