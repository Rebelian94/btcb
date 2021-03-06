#pragma once

#include <boost/thread/thread.hpp>
#include <btcb/node/common.hpp>
#include <btcb/node/lmdb.hpp>
#include <btcb/node/openclwork.hpp>
#include <btcb/secure/blockstore.hpp>
#include <btcb/secure/common.hpp>

#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>

namespace btcb
{
// The fan spreads a key out over the heap to decrease the likelihood of it being recovered by memory inspection
class fan
{
public:
	fan (btcb::uint256_union const &, size_t);
	void value (btcb::raw_key &);
	void value_set (btcb::raw_key const &);
	std::vector<std::unique_ptr<btcb::uint256_union>> values;

private:
	std::mutex mutex;
	void value_get (btcb::raw_key &);
};
class node_config;
class kdf
{
public:
	void phs (btcb::raw_key &, std::string const &, btcb::uint256_union const &);
	std::mutex mutex;
};
enum class key_type
{
	not_a_type,
	unknown,
	adhoc,
	deterministic
};
class wallet_store
{
public:
	wallet_store (bool &, btcb::kdf &, btcb::transaction &, btcb::account, unsigned, std::string const &);
	wallet_store (bool &, btcb::kdf &, btcb::transaction &, btcb::account, unsigned, std::string const &, std::string const &);
	std::vector<btcb::account> accounts (btcb::transaction const &);
	void initialize (btcb::transaction const &, bool &, std::string const &);
	btcb::uint256_union check (btcb::transaction const &);
	bool rekey (btcb::transaction const &, std::string const &);
	bool valid_password (btcb::transaction const &);
	bool attempt_password (btcb::transaction const &, std::string const &);
	void wallet_key (btcb::raw_key &, btcb::transaction const &);
	void seed (btcb::raw_key &, btcb::transaction const &);
	void seed_set (btcb::transaction const &, btcb::raw_key const &);
	btcb::key_type key_type (btcb::wallet_value const &);
	btcb::public_key deterministic_insert (btcb::transaction const &);
	void deterministic_key (btcb::raw_key &, btcb::transaction const &, uint32_t);
	uint32_t deterministic_index_get (btcb::transaction const &);
	void deterministic_index_set (btcb::transaction const &, uint32_t);
	void deterministic_clear (btcb::transaction const &);
	btcb::uint256_union salt (btcb::transaction const &);
	bool is_representative (btcb::transaction const &);
	btcb::account representative (btcb::transaction const &);
	void representative_set (btcb::transaction const &, btcb::account const &);
	btcb::public_key insert_adhoc (btcb::transaction const &, btcb::raw_key const &);
	void insert_watch (btcb::transaction const &, btcb::public_key const &);
	void erase (btcb::transaction const &, btcb::public_key const &);
	btcb::wallet_value entry_get_raw (btcb::transaction const &, btcb::public_key const &);
	void entry_put_raw (btcb::transaction const &, btcb::public_key const &, btcb::wallet_value const &);
	bool fetch (btcb::transaction const &, btcb::public_key const &, btcb::raw_key &);
	bool exists (btcb::transaction const &, btcb::public_key const &);
	void destroy (btcb::transaction const &);
	btcb::store_iterator<btcb::uint256_union, btcb::wallet_value> find (btcb::transaction const &, btcb::uint256_union const &);
	btcb::store_iterator<btcb::uint256_union, btcb::wallet_value> begin (btcb::transaction const &, btcb::uint256_union const &);
	btcb::store_iterator<btcb::uint256_union, btcb::wallet_value> begin (btcb::transaction const &);
	btcb::store_iterator<btcb::uint256_union, btcb::wallet_value> end ();
	void derive_key (btcb::raw_key &, btcb::transaction const &, std::string const &);
	void serialize_json (btcb::transaction const &, std::string &);
	void write_backup (btcb::transaction const &, boost::filesystem::path const &);
	bool move (btcb::transaction const &, btcb::wallet_store &, std::vector<btcb::public_key> const &);
	bool import (btcb::transaction const &, btcb::wallet_store &);
	bool work_get (btcb::transaction const &, btcb::public_key const &, uint64_t &);
	void work_put (btcb::transaction const &, btcb::public_key const &, uint64_t);
	unsigned version (btcb::transaction const &);
	void version_put (btcb::transaction const &, unsigned);
	void upgrade_v1_v2 (btcb::transaction const &);
	void upgrade_v2_v3 (btcb::transaction const &);
	void upgrade_v3_v4 (btcb::transaction const &);
	btcb::fan password;
	btcb::fan wallet_key_mem;
	static unsigned const version_1 = 1;
	static unsigned const version_2 = 2;
	static unsigned const version_3 = 3;
	static unsigned const version_4 = 4;
	unsigned const version_current = version_4;
	static btcb::uint256_union const version_special;
	static btcb::uint256_union const wallet_key_special;
	static btcb::uint256_union const salt_special;
	static btcb::uint256_union const check_special;
	static btcb::uint256_union const representative_special;
	static btcb::uint256_union const seed_special;
	static btcb::uint256_union const deterministic_index_special;
	static size_t const check_iv_index;
	static size_t const seed_iv_index;
	static int const special_count;
	static unsigned const kdf_full_work = 64 * 1024;
	static unsigned const kdf_test_work = 8;
	static unsigned const kdf_work = btcb::btcb_network == btcb::btcb_networks::btcb_test_network ? kdf_test_work : kdf_full_work;
	btcb::kdf & kdf;
	MDB_dbi handle;
	std::recursive_mutex mutex;

private:
	MDB_txn * tx (btcb::transaction const &) const;
};
class wallets;
// A wallet is a set of account keys encrypted by a common encryption key
class wallet : public std::enable_shared_from_this<btcb::wallet>
{
public:
	std::shared_ptr<btcb::block> change_action (btcb::account const &, btcb::account const &, bool = true);
	std::shared_ptr<btcb::block> receive_action (btcb::block const &, btcb::account const &, btcb::uint128_union const &, bool = true);
	std::shared_ptr<btcb::block> send_action (btcb::account const &, btcb::account const &, btcb::uint128_t const &, bool = true, boost::optional<std::string> = {});
	wallet (bool &, btcb::transaction &, btcb::wallets &, std::string const &);
	wallet (bool &, btcb::transaction &, btcb::wallets &, std::string const &, std::string const &);
	void enter_initial_password ();
	bool enter_password (btcb::transaction const &, std::string const &);
	btcb::public_key insert_adhoc (btcb::raw_key const &, bool = true);
	btcb::public_key insert_adhoc (btcb::transaction const &, btcb::raw_key const &, bool = true);
	void insert_watch (btcb::transaction const &, btcb::public_key const &);
	btcb::public_key deterministic_insert (btcb::transaction const &, bool = true);
	btcb::public_key deterministic_insert (bool = true);
	bool exists (btcb::public_key const &);
	bool import (std::string const &, std::string const &);
	void serialize (std::string &);
	bool change_sync (btcb::account const &, btcb::account const &);
	void change_async (btcb::account const &, btcb::account const &, std::function<void(std::shared_ptr<btcb::block>)> const &, bool = true);
	bool receive_sync (std::shared_ptr<btcb::block>, btcb::account const &, btcb::uint128_t const &);
	void receive_async (std::shared_ptr<btcb::block>, btcb::account const &, btcb::uint128_t const &, std::function<void(std::shared_ptr<btcb::block>)> const &, bool = true);
	btcb::block_hash send_sync (btcb::account const &, btcb::account const &, btcb::uint128_t const &);
	void send_async (btcb::account const &, btcb::account const &, btcb::uint128_t const &, std::function<void(std::shared_ptr<btcb::block>)> const &, bool = true, boost::optional<std::string> = {});
	void work_apply (btcb::account const &, std::function<void(uint64_t)>);
	void work_cache_blocking (btcb::account const &, btcb::block_hash const &);
	void work_update (btcb::transaction const &, btcb::account const &, btcb::block_hash const &, uint64_t);
	void work_ensure (btcb::account const &, btcb::block_hash const &);
	bool search_pending ();
	void init_free_accounts (btcb::transaction const &);
	/** Changes the wallet seed and returns the first account */
	btcb::public_key change_seed (btcb::transaction const & transaction_a, btcb::raw_key const & prv_a);
	bool live ();
	std::unordered_set<btcb::account> free_accounts;
	std::function<void(bool, bool)> lock_observer;
	btcb::wallet_store store;
	btcb::wallets & wallets;
};
class node;

/**
 * The wallets set is all the wallets a node controls.
 * A node may contain multiple wallets independently encrypted and operated.
 */
class wallets
{
public:
	wallets (bool &, btcb::node &);
	~wallets ();
	std::shared_ptr<btcb::wallet> open (btcb::uint256_union const &);
	std::shared_ptr<btcb::wallet> create (btcb::uint256_union const &);
	bool search_pending (btcb::uint256_union const &);
	void search_pending_all ();
	void destroy (btcb::uint256_union const &);
	void do_wallet_actions ();
	void queue_wallet_action (btcb::uint128_t const &, std::shared_ptr<btcb::wallet>, std::function<void(btcb::wallet &)> const &);
	void foreach_representative (btcb::transaction const &, std::function<void(btcb::public_key const &, btcb::raw_key const &)> const &);
	bool exists (btcb::transaction const &, btcb::public_key const &);
	void stop ();
	void clear_send_ids (btcb::transaction const &);
	std::function<void(bool)> observer;
	std::unordered_map<btcb::uint256_union, std::shared_ptr<btcb::wallet>> items;
	std::multimap<btcb::uint128_t, std::pair<std::shared_ptr<btcb::wallet>, std::function<void(btcb::wallet &)>>, std::greater<btcb::uint128_t>> actions;
	std::mutex mutex;
	std::condition_variable condition;
	btcb::kdf kdf;
	MDB_dbi handle;
	MDB_dbi send_action_ids;
	btcb::node & node;
	btcb::mdb_env & env;
	bool stopped;
	boost::thread thread;
	static btcb::uint128_t const generate_priority;
	static btcb::uint128_t const high_priority;

	/** Start read-write transaction */
	btcb::transaction tx_begin_write ();

	/** Start read-only transaction */
	btcb::transaction tx_begin_read ();

	/**
	 * Start a read-only or read-write transaction
	 * @param write If true, start a read-write transaction
	 */
	btcb::transaction tx_begin (bool write = false);
};
}
