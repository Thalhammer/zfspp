#include "zfspp.h"

#include <libzfs.h>
#include <mutex>
#include <stdexcept>
#include <sys/fs/zfs.h>
#include <sys/stdtypes.h>
#include <zfeature_common.h>

namespace zfspp {

	pool zfs::create_pool(const std::string& name, const nv_list& topology, const nv_list& pool_options,
						  const nv_list& fs_options, bool enable_all_features) {
		auto pool_opts = pool_options;
		if (enable_all_features) {
			for (auto& e : spa_feature_table) {
				std::string fname = "feature@";
				fname += e.fi_uname;
				pool_opts.add_string(fname.c_str(), "enabled");
			}
		}

		std::unique_lock<std::recursive_mutex> lck{m_mutex};
		auto res = zpool_create(m_handle, name.c_str(), topology.raw(), pool_opts.raw(), fs_options.raw());
		if (res != 0) throw std::system_error(libzfs_errno(m_handle), zfs_category());
		auto hdl = zpool_open(m_handle, name.c_str());
		if (hdl == nullptr) throw std::system_error(libzfs_errno(m_handle), zfs_category());
		return pool(*this, hdl);
	}

	pool zfs::open_pool(const std::string& name) {
		std::unique_lock<std::recursive_mutex> lck{m_mutex};
		auto hdl = zpool_open(m_handle, name.c_str());
		if (hdl == nullptr) throw std::system_error(libzfs_errno(m_handle), zfs_category());
		return pool(*this, hdl);
	}

	std::vector<pool> zfs::list_pools() {
		std::unique_lock<std::recursive_mutex> lck{m_mutex};
		struct _state {
			zfs& parent;
			std::vector<pool> result;
			bool alloc_failed = false;
		} state{*this};
		zpool_iter(
			m_handle,
			[](zpool_handle_t* hdl, void* udata) -> int {
				auto ptr = static_cast<_state*>(udata);
				if (ptr->alloc_failed) {
					zpool_close(hdl);
				} else {
					try {
						ptr->result.emplace_back(ptr->parent, hdl);
					} catch (...) {
						ptr->alloc_failed = true;
						zpool_close(hdl);
					}
				}
				return 0;
			},
			&state);
		if (state.alloc_failed) throw std::bad_alloc();
		return std::move(state.result);
	}

	pool& pool::operator=(pool&& other) {
		m_parent = other.m_parent;
		if (m_pool) zpool_close(m_pool);
		m_pool = other.m_pool;
		other.m_parent = nullptr;
		other.m_pool = nullptr;
		return *this;
	}

	pool::~pool() {
		if (m_pool) zpool_close(m_pool);
	}

	std::string pool::name() const noexcept {
		if (m_pool == nullptr || m_parent == nullptr) return "";
		std::unique_lock<zfs> lck{*m_parent};
		return zpool_get_name(m_pool);
	}

	std::string pool::state_str() const noexcept {
		if (m_pool == nullptr || m_parent == nullptr) return "";
		std::unique_lock<zfs> lck{*m_parent};
		return zpool_get_state_str(m_pool);
	}

	int pool::state() const noexcept {
		if (m_pool == nullptr || m_parent == nullptr) return -1;
		std::unique_lock<zfs> lck{*m_parent};
		return zpool_get_state(m_pool);
	}

	pool_status pool::status() const noexcept {
		if (m_pool == nullptr || m_parent == nullptr) return pool_status::corrupt_pool;
		char* msg{};
		std::unique_lock<zfs> lck{*m_parent};
		return static_cast<pool_status>(zpool_get_status(m_pool, &msg, nullptr));
	}

	nv_list pool::config() const {
		if (m_pool == nullptr || m_parent == nullptr) return nv_list();
		std::unique_lock<zfs> lck{*m_parent};
		return zpool_get_config(m_pool, nullptr);
	}

	nv_list pool::features() const {
		if (m_pool == nullptr || m_parent == nullptr) return nv_list();
		std::unique_lock<zfs> lck{*m_parent};
		return zpool_get_features(m_pool);
	}

	void pool::destroy(bool force) {
		if (m_pool == nullptr || m_parent == nullptr) throw std::logic_error("invalid pool handle");
		std::unique_lock<zfs> lck{*m_parent};
		if (zpool_disable_datasets(m_pool, force ? B_TRUE : B_FALSE) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		if (zpool_destroy(m_pool, "destroy") != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void pool::export_(bool force) {
		if (m_pool == nullptr || m_parent == nullptr) throw std::logic_error("invalid pool handle");
		std::unique_lock<zfs> lck{*m_parent};
		if (zpool_export(m_pool, force ? B_TRUE : B_FALSE, "export") != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void pool::checkpoint() {
		if (m_pool == nullptr || m_parent == nullptr) throw std::logic_error("invalid pool handle");
		std::unique_lock<zfs> lck{*m_parent};
		if (zpool_checkpoint(m_pool) != 0) throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void pool::discard_checkpoint() {
		if (m_pool == nullptr || m_parent == nullptr) throw std::logic_error("invalid pool handle");
		std::unique_lock<zfs> lck{*m_parent};
		if (zpool_discard_checkpoint(m_pool) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void pool::upgrade() {
		if (m_pool == nullptr || m_parent == nullptr) throw std::logic_error("invalid pool handle");
		std::unique_lock<zfs> lck{*m_parent};
		if (zpool_upgrade(m_pool, SPA_VERSION) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

} // namespace zfspp