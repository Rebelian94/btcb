#pragma once

#include <rai/secure.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>

namespace rai
{
class work_pool
{
public:
	work_pool ();
	~work_pool ();
	void loop (uint64_t);
	void stop ();
	uint64_t generate (rai::uint256_union const &);
	void generate (rai::block &);
	uint64_t work_value (rai::block_hash const &, uint64_t);
	bool work_validate (rai::block &);
	bool work_validate (rai::block_hash const &, uint64_t);
	rai::uint256_union current;
	std::atomic <int> ticket;
	bool done;
	std::vector <std::thread> threads;
	std::unordered_map <rai::uint256_union, uint64_t> completed;
	std::queue <rai::uint256_union> pending;
	std::mutex mutex;
	std::condition_variable consumer_condition;
	std::condition_variable producer_condition;
	// Local work threshold for rate-limiting publishing blocks. ~5 seconds of work.
	static uint64_t const publish_test_threshold = 0xf000000000000000;
	static uint64_t const publish_full_threshold = 0xfff0000000000000;
	static uint64_t const publish_threshold = rai::rai_network == rai::rai_networks::rai_test_network ? publish_test_threshold : publish_full_threshold;
    static unsigned const block_full_work = 1024;
    static unsigned const block_test_work = 8;
    static unsigned const block_work = rai::rai_network == rai::rai_networks::rai_test_network ? block_test_work : block_full_work;
};
// The fan spreads a key out over the heap to decrease the likelyhood of it being recovered by memory inspection
class fan
{
public:
    fan (rai::uint256_union const &, size_t);
    rai::uint256_union value ();
    void value_set (rai::uint256_union const &);
    std::vector <std::unique_ptr <rai::uint256_union>> values;
};
class wallet_value
{
public:
	wallet_value () = default;
	wallet_value (MDB_val const &);
	wallet_value (rai::uint256_union const &);
	rai::mdb_val val () const;
	rai::private_key key;
	uint64_t work;
};
class node_config;
class wallet_store
{
public:
    wallet_store (bool &, rai::transaction &, rai::account, std::string const &);
    wallet_store (bool &, rai::transaction &, rai::account, std::string const &, std::string const &);
	std::vector <rai::account> accounts (MDB_txn *);
    void initialize (MDB_txn *, bool &, std::string const &);
    rai::uint256_union check (MDB_txn *);
    bool rekey (MDB_txn *, std::string const &);
    bool valid_password (MDB_txn *);
    void enter_password (MDB_txn *, std::string const &);
    rai::uint256_union wallet_key (MDB_txn *);
    rai::uint256_union salt (MDB_txn *);
    bool is_representative (MDB_txn *);
    rai::account representative (MDB_txn *);
    void representative_set (MDB_txn *, rai::account const &);
    rai::public_key insert (MDB_txn *, rai::private_key const &);
    void erase (MDB_txn *, rai::public_key const &);
	rai::wallet_value entry_get_raw (MDB_txn *, rai::public_key const &);
	void entry_put_raw (MDB_txn *, rai::public_key const &, rai::wallet_value const &);
    bool fetch (MDB_txn *, rai::public_key const &, rai::private_key &);
    bool exists (MDB_txn *, rai::public_key const &);
	void destroy (MDB_txn *);
    rai::store_iterator find (MDB_txn *, rai::uint256_union const &);
    rai::store_iterator begin (MDB_txn *);
    rai::store_iterator end ();
    rai::uint256_union derive_key (MDB_txn *, std::string const &);
    void serialize_json (MDB_txn *, std::string &);
	void write_backup (MDB_txn *, boost::filesystem::path const &);
    bool move (MDB_txn *, rai::wallet_store &, std::vector <rai::public_key> const &);
	bool import (MDB_txn *, rai::wallet_store &);
	bool work_get (MDB_txn *, rai::public_key const &, uint64_t &);
	void work_put (MDB_txn *, rai::public_key const &, uint64_t);
    rai::fan password;
    static rai::uint256_union const version_1;
    static rai::uint256_union const version_current;
    static rai::uint256_union const version_special;
    static rai::uint256_union const wallet_key_special;
    static rai::uint256_union const salt_special;
    static rai::uint256_union const check_special;
    static rai::uint256_union const representative_special;
    static int const special_count;
    static unsigned const kdf_full_work = 1 * 1024 * 1024;
    static unsigned const kdf_test_work = 8;
    static unsigned const kdf_work = rai::rai_network == rai::rai_networks::rai_test_network ? kdf_test_work : kdf_full_work;
	rai::mdb_env & environment;
    MDB_dbi handle;
};
class node;
// A wallet is a set of account keys encrypted by a common encryption key
class wallet : public std::enable_shared_from_this <rai::wallet>
{
public:
    wallet (bool &, rai::transaction &, rai::node &, std::string const &);
    wallet (bool &, rai::transaction &, rai::node &, std::string const &, std::string const &);
	void enter_initial_password (MDB_txn *);
	rai::public_key insert (rai::private_key const &);
    bool exists (rai::public_key const &);
	bool import (std::string const &, std::string const &);
	void serialize (std::string &);
	bool change_action (rai::account const &, rai::account const &);
    bool receive_action (rai::send_block const &, rai::private_key const &, rai::account const &);
	bool send_action (rai::account const &, rai::account const &, rai::uint128_t const &);
	bool change_sync (rai::account const &, rai::account const &);
    bool receive_sync (rai::send_block const &, rai::private_key const &, rai::account const &);
	bool send_sync (rai::account const &, rai::account const &, rai::uint128_t const &);
    void work_generate (rai::account const &, rai::block_hash const &);
    void work_update (MDB_txn *, rai::account const &, rai::block_hash const &, uint64_t);
    uint64_t work_fetch (MDB_txn *, rai::account const &, rai::block_hash const &);
	bool search_pending ();
    rai::wallet_store store;
    rai::node & node;
};
// The wallets set is all the wallets a node controls.  A node may contain multiple wallets independently encrypted and operated.
class wallets
{
public:
	wallets (bool &, rai::node &);
	std::shared_ptr <rai::wallet> open (rai::uint256_union const &);
	std::shared_ptr <rai::wallet> create (rai::uint256_union const &);
    bool search_pending (rai::uint256_union const &);
	void destroy (rai::uint256_union const &);
	void queue_wallet_action (rai::account const &, std::function <void ()> const &);
	void foreach_representative (std::function <void (rai::public_key const &, rai::private_key const &)> const &);
	std::unordered_map <rai::uint256_union, std::shared_ptr <rai::wallet>> items;
	std::unordered_multimap <rai::account, std::function <void ()>> pending_actions;
	std::unordered_set <rai::account> current_actions;
	std::mutex action_mutex;
	MDB_dbi handle;
	rai::node & node;
};
}