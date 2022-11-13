#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

struct libzfs_handle;
struct zpool_handle;
struct zfs_handle;
struct nvlist;
struct nvpair;
namespace zfspp {

	enum class nv_type;
	class nv_pair;
	class nv_list;
	enum class dataset_type;
	class zfs;
	enum class pool_status;
	class pool;
	class dataset;
	class event_watcher;

	enum class nv_type {
		unknown = 0,
		boolean,
		byte,
		int16,
		uint16,
		int32,
		uint32,
		int64,
		uint64,
		string,
		byte_array,
		int16_array,
		uint16_array,
		int32_array,
		uint32_array,
		int64_array,
		uint64_array,
		string_array,
		hrtime,
		nvlist,
		nvlist_array,
		boolean_value,
		int8,
		uint8,
		boolean_array,
		int8_array,
		uint88_array,
	};

	class nv_pair {
		::nvlist* m_list;
		::nvpair* m_pair;
		friend class nv_list;

		nv_pair(::nvlist* list, ::nvpair* pair) : m_list(list), m_pair(pair) {}

	public:
		nv_pair() : m_list(nullptr), m_pair(nullptr) {}
		::nvpair* raw() const noexcept { return m_pair; }
		::nvlist* raw_list() const noexcept { return m_list; }

		std::string_view key() const noexcept;
		nv_type type() const noexcept;

		bool as_boolean() const;
		uchar_t as_byte() const;
		int8_t as_int8() const;
		uint8_t as_uint8() const;
		int16_t as_int16() const;
		uint16_t as_uint16() const;
		int32_t as_int32() const;
		uint32_t as_uint32() const;
		int64_t as_int64() const;
		uint64_t as_uint64() const;
		std::string_view as_string() const;
		nv_list as_nvlist() const;

		std::vector<bool> as_boolean_array() const;
		std::vector<uchar_t> as_byte_array() const;
		std::vector<int8_t> as_int8_array() const;
		std::vector<uint8_t> as_uint8_array() const;
		std::vector<int16_t> as_int16_array() const;
		std::vector<uint16_t> as_uint16_array() const;
		std::vector<int32_t> as_int32_array() const;
		std::vector<uint32_t> as_uint32_array() const;
		std::vector<int64_t> as_int64_array() const;
		std::vector<uint64_t> as_uint64_array() const;
		std::vector<std::string_view> as_string_array() const;
		std::vector<nv_list> as_nvlist_array() const;

		nv_pair& operator*() noexcept { return *this; }
		nv_pair& operator++() noexcept;
		nv_pair operator++(int) noexcept {
			auto tmp = *this;
			++(*this);
			return tmp;
		}
		bool operator==(const nv_pair& rhs) const noexcept { return m_pair == rhs.m_pair; }
		bool operator!=(const nv_pair& rhs) const noexcept { return m_pair != rhs.m_pair; }
	};

	class nv_list {
		::nvlist* m_handle{};

		void ensure_allocated();

	public:
		struct adopt_list {};
		nv_list();
		nv_list(::nvlist* list);
		nv_list(::nvlist* list, adopt_list) : m_handle(list) {}
		nv_list(const nv_list& other);
		nv_list(nv_list&& other);
		nv_list& operator=(const nv_list& other);
		nv_list& operator=(nv_list&& other);
		~nv_list();

		::nvlist* raw() const noexcept { return m_handle; }

		void clear();
		size_t size() const noexcept;
		bool empty() const noexcept { return begin() == end(); }
		std::set<std::string_view> keys() const;

		nv_pair begin() const noexcept;
		nv_pair cbegin() const noexcept;
		nv_pair end() const noexcept { return nv_pair(); }
		nv_pair cend() const noexcept { return nv_pair(); }
		nv_pair find(const char* key) const noexcept;

		nv_pair at(const char* key) const {
			auto pair = find(key);
			if (pair == end()) throw std::out_of_range(key);
			return pair;
		}

		bool erase(const char* key);

		void add_boolean(const char* key);
		void add_boolean_value(const char* key, bool val);
		void add_byte(const char* key, uchar_t val);
		void add_int8(const char* key, int8_t val);
		void add_uint8(const char* key, uint8_t val);
		void add_int16(const char* key, int16_t val);
		void add_uint16(const char* key, uint16_t val);
		void add_int32(const char* key, int32_t val);
		void add_uint32(const char* key, uint32_t val);
		void add_int64(const char* key, int64_t val);
		void add_uint64(const char* key, uint64_t val);
		void add_string(const char* key, const char* val);
		void add_nvlist(const char* key, const nv_list& val);
		void add_boolean_array(const char* key, std::span<const bool> val);
		void add_byte_array(const char* key, std::span<const uchar_t> val);
		void add_int8_array(const char* key, std::span<const int8_t> val);
		void add_uint8_array(const char* key, std::span<const uint8_t> val);
		void add_int16_array(const char* key, std::span<const int16_t> val);
		void add_uint16_array(const char* key, std::span<const uint16_t> val);
		void add_int32_array(const char* key, std::span<const int32_t> val);
		void add_uint32_array(const char* key, std::span<const uint32_t> val);
		void add_int64_array(const char* key, std::span<const int64_t> val);
		void add_uint64_array(const char* key, std::span<const uint64_t> val);
		void add_string_array(const char* key, std::span<const char* const> val);
		void add_nvlist_array(const char* key, std::span<const nv_list> val);
		void add_hrtime(const char* key, hrtime_t);

		std::string to_json(bool with_types = false) const;
	};

	enum class dataset_type {
		filesystem = (1 << 0),
		snapshot = (1 << 1),
		volume = (1 << 2),
		pool = (1 << 3),
		bookmark = (1 << 4),
		any = 0x1f,
	};

	class zfs {
		std::recursive_mutex m_mutex;
		libzfs_handle* m_handle{};
		int m_eventfd{-1};

	public:
		zfs();
		zfs(const zfs&) = delete;
		zfs(zfs&&) = delete;
		zfs& operator=(const zfs&) = delete;
		zfs& operator=(zfs&&) = delete;
		~zfs();

		void lock() { return m_mutex.lock(); }
		bool try_lock() { return m_mutex.try_lock(); }
		void unlock() { return m_mutex.unlock(); }

		libzfs_handle* raw() const noexcept { return m_handle; }

		std::vector<dataset> root_datasets();
		dataset open_dataset(const std::string& name, dataset_type dt = dataset_type::any);
		dataset open_dataset_from_fs_path(const std::string& path, dataset_type dt = dataset_type::any);

		pool create_pool(const std::string& name, const nv_list& topology, const nv_list& pool_options,
						 const nv_list& fs_options, bool enable_all_features = true);
		pool open_pool(const std::string& name);
		std::vector<pool> list_pools();

		bool next_event(nv_list& data, size_t* n_dropped = nullptr, bool block = false);

		bool validate_dataset_name(const char* name, dataset_type dt, std::string* reason = nullptr);
	};

	enum class pool_status {
		corrupt_cache,		 /* corrupt /kernel/drv/zpool.cache */
		missing_dev_r,		 /* missing device with replicas */
		missing_dev_nr,		 /* missing device with no replicas */
		corrupt_label_r,	 /* bad device label with replicas */
		corrupt_label_nr,	 /* bad device label with no replicas */
		bad_guid_sum,		 /* sum of device guids didn't match */
		corrupt_pool,		 /* pool metadata is corrupted */
		corrupt_data,		 /* data errors in user (meta)data */
		failing_dev,		 /* device experiencing errors */
		version_newer,		 /* newer on-disk version */
		hostid_mismatch,	 /* last accessed by another system */
		hostid_active,		 /* currently active on another system */
		hostid_required,	 /* multihost=on and hostid=0 */
		io_failure_wait,	 /* failed i/o, failmode 'wait' */
		io_failure_continue, /* failed i/o, failmode 'continue' */
		io_failure_mmp,		 /* failed mmp, failmode not 'panic' */
		bad_log,			 /* cannot read log chain(s) */
		errata,				 /* informational errata available */
		unsup_feat_read,	 /* unsupported features for read */
		unsup_feat_write,	 /* unsupported features for write */
		faulted_dev_r,		 /* faulted device with replicas */
		faulted_dev_nr,		 /* faulted device with no replicas */
		version_older,		 /* older legacy on-disk version */
		feat_disabled,		 /* supported features are disabled */
		resilvering,		 /* device being resilvered */
		offline_dev,		 /* device offline */
		removed_dev,		 /* removed device */
		rebuilding,			 /* device being rebuilt */
		rebuild_scrub,		 /* recommend scrubbing the pool */
		non_native_ashift,	 /* (e.g. 512e dev with ashift of 9) */
		compatibility_err,	 /* bad 'compatibility' property */
		incompatible_feat,	 /* feature set outside compatibility */
		ok
	};

	class pool {
		zfs* m_parent{};
		zpool_handle* m_pool{};

	public:
		pool(zfs& parent, zpool_handle* pool) : m_parent(&parent), m_pool(pool) {}
		pool(pool&& other) : m_parent(other.m_parent), m_pool(other.m_pool) {
			other.m_parent = nullptr;
			other.m_pool = nullptr;
		}
		pool& operator=(pool&& other);
		pool(const pool&) = delete;
		pool& operator=(const pool&) = delete;
		~pool();

		bool valid() const noexcept { return m_pool != nullptr; }
		operator bool() const noexcept { return valid(); }
		bool operator!() const noexcept { return !valid(); }

		zpool_handle* raw() const noexcept { return m_pool; }

		zfs& client() noexcept { return *m_parent; }
		const zfs& client() const noexcept { return *m_parent; }

		std::string_view name() const noexcept;
		std::string_view state_str() const noexcept;
		int state() const noexcept;
		pool_status status() const noexcept;

		nv_list config() const;
		nv_list features() const;

		void destroy(bool force = false);
		void export_(bool force = false);

		void checkpoint();
		void discard_checkpoint();
		void upgrade();
	};

	class dataset {
		zfs* m_parent{};
		zfs_handle* m_hdl{};

	public:
		dataset(zfs& parent, zfs_handle* hdl) : m_parent(&parent), m_hdl(hdl) {}
		dataset(dataset&& other) : m_parent(other.m_parent), m_hdl(other.m_hdl) {
			other.m_parent = nullptr;
			other.m_hdl = nullptr;
		}
		dataset& operator=(dataset&& other);
		dataset(const dataset&);
		dataset& operator=(const dataset&);
		~dataset();

		bool valid() const noexcept { return m_hdl != nullptr; }
		operator bool() const noexcept { return valid(); }
		bool operator!() const noexcept { return !valid(); }

		zfs_handle* raw() const noexcept { return m_hdl; }

		zfs& client() noexcept { return *m_parent; }
		const zfs& client() const noexcept { return *m_parent; }

		std::string_view name() const noexcept;
		std::string_view relative_name() const noexcept;
		pool pool() const noexcept;
		std::string_view pool_name() const noexcept;
		dataset_type type() const noexcept;
		std::string mountpoint() const noexcept;

		std::vector<dataset> children() const;
		std::vector<dataset> filesystems() const;
		std::vector<dataset> snapshots() const;
		std::vector<dataset> snapshots_sorted() const;
		std::vector<dataset> bookmarks() const;
		std::vector<dataset> mounted_children() const;

		nv_list properties() const;
		nv_list user_properties() const;
		void set_property(const char* name, const char* value);

		dataset create_snapshot(const char* name, bool recursive = false, const nv_list& opts = {});
		dataset create_child(const char* name, dataset_type type = dataset_type::filesystem, const nv_list& opts = {});
		dataset clone(const char* name, const nv_list& opts = {});
		void destroy(bool defer = false);
		void mount(const std::string& options = {}, int flags = 0);
		void mount_at(const std::string& mountpoint, const std::string& options = {}, int flags = 0);
		void unmount(bool force = false);
	};

	class event_watcher {
		zfs* m_parent{};
		std::thread m_watcher_thread;
		mutable std::mutex m_mtx;
		std::array<uint64_t, 3> m_checkpoint{};
		std::function<void(const nv_list&)> m_on_event;
		std::function<void(size_t)> m_on_drop;
		std::function<void()> m_on_error;
		bool m_should_stop{};
		bool m_is_started{false};

		void thread_fn();

	public:
		event_watcher(zfs& parent);
		~event_watcher();

		void set_checkpoint(std::span<uint64_t, 3> checkpoint);
		std::array<uint64_t, 3> checkpoint() const noexcept;

		void set_on_event(std::function<void(const nv_list&)> cb);
		void set_on_drop(std::function<void(size_t)> cb);
		void set_on_error(std::function<void()> cb);

		void start();
		void stop();
	};

	const std::error_category& zfs_category() noexcept;

	constexpr inline dataset_type operator|(dataset_type lhs, dataset_type rhs) noexcept {
		return static_cast<dataset_type>(static_cast<size_t>(lhs) | static_cast<size_t>(rhs));
	}

	const char* nv_type_name(nv_type dt) noexcept;
} // namespace zfspp