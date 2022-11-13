#include "zfspp.h"
#include <libzfs.h>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <sys/fs/zfs.h>
#include <sys/mount.h>
#include <sys/stdtypes.h>

namespace zfspp {

	struct iterate_data {
		zfs& parent;
		std::vector<dataset> result;
		bool alloc_failed = false;

		static int cb(zfs_handle* hdl, void* udata) {
			auto ptr = static_cast<iterate_data*>(udata);
			if (ptr->alloc_failed) {
				zfs_close(hdl);
			} else {
				try {
					ptr->result.emplace_back(ptr->parent, hdl);
				} catch (...) {
					ptr->alloc_failed = true;
					zfs_close(hdl);
				}
			}
			return 0;
		}
	};

	std::vector<dataset> zfs::root_datasets() {
		std::unique_lock lck{m_mutex};
		iterate_data data{*this};
		zfs_iter_root(m_handle, iterate_data::cb, &data);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	dataset zfs::open_dataset(const std::string& name, dataset_type dt) {
		std::unique_lock lck{m_mutex};
		auto hdl = zfs_open(m_handle, name.c_str(), static_cast<zfs_type_t>(dt));
		if (hdl == nullptr) throw std::system_error(libzfs_errno(m_handle), zfs_category());
		return dataset(*this, hdl);
	}

	dataset zfs::open_dataset_from_fs_path(const std::string& path, dataset_type dt) {
		std::unique_lock lck{*this};
		auto hdl = zfs_path_to_zhandle(m_handle, path.c_str(), static_cast<zfs_type_t>(dt));
		if (hdl == nullptr) throw std::system_error(libzfs_errno(m_handle), zfs_category());
		return dataset(*this, hdl);
	}

	dataset& dataset::operator=(dataset&& other) {
		if (m_hdl) zfs_close(m_hdl);
		m_parent = other.m_parent;
		m_hdl = other.m_hdl;
		other.m_parent = nullptr;
		other.m_hdl = nullptr;
		return *this;
	}

	dataset::dataset(const dataset& other) : m_parent(other.m_parent), m_hdl(nullptr) {
		if (other.m_hdl) m_hdl = zfs_handle_dup(other.m_hdl);
	}

	dataset& dataset::operator=(const dataset& other) {
		if (m_hdl) zfs_close(m_hdl);
		m_hdl = nullptr;
		m_parent = other.m_parent;
		if (other.m_hdl) m_hdl = zfs_handle_dup(other.m_hdl);
		return *this;
	}

	dataset::~dataset() {
		if (m_hdl) zfs_close(m_hdl);
	}

	std::string_view dataset::name() const noexcept { return zfs_get_name(m_hdl); }

	std::string_view dataset::relative_name() const noexcept {
		auto name = this->name();
		auto pos = name.rfind('/');
		if (pos == std::string::npos)
			pos = 0;
		else
			pos++;
		return name.substr(pos);
	}

	pool dataset::pool() const noexcept { return {*m_parent, zfs_get_pool_handle(m_hdl)}; }

	std::string_view dataset::pool_name() const noexcept { return zfs_get_pool_name(m_hdl); }

	dataset_type dataset::type() const noexcept { return static_cast<dataset_type>(zfs_get_type(m_hdl)); }

	std::string dataset::mountpoint() const noexcept {
		char* ptr{nullptr};
		if (zfs_is_mounted(m_hdl, &ptr) == B_FALSE) return "";
		std::string res{ptr};
		free(ptr);
		return res;
	}

	std::vector<dataset> dataset::children() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_children(m_hdl, iterate_data::cb, &data);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	std::vector<dataset> dataset::filesystems() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_filesystems(m_hdl, iterate_data::cb, &data);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	std::vector<dataset> dataset::snapshots() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_snapshots(m_hdl, B_FALSE, iterate_data::cb, &data, 0, 0);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	std::vector<dataset> dataset::snapshots_sorted() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_snapshots_sorted(m_hdl, iterate_data::cb, &data, 0, 0);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	std::vector<dataset> dataset::bookmarks() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_bookmarks(m_hdl, iterate_data::cb, &data);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	std::vector<dataset> dataset::mounted_children() const {
		std::unique_lock lck{*m_parent};
		iterate_data data{*m_parent};
		zfs_iter_mounted(m_hdl, iterate_data::cb, &data);
		if (data.alloc_failed) throw std::bad_alloc();
		return std::move(data.result);
	}

	nv_list dataset::properties() const {
		std::unique_lock lck{*m_parent};
		auto res = zfs_get_all_props(m_hdl);
		if (res == nullptr) throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		return nv_list{res};
	}

	nv_list dataset::user_properties() const {
		std::unique_lock lck{*m_parent};
		auto res = zfs_get_user_props(m_hdl);
		if (res == nullptr) throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		return nv_list{res};
	}

	void dataset::set_property(const char* name, const char* value) {
		std::unique_lock lck{*m_parent};
		if (zfs_prop_set(m_hdl, name, value) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	dataset dataset::create_snapshot(const char* name, bool recursive, const nv_list& opts) {
		std::string fullname{this->name()};
		fullname += "@";
		fullname += name;
		std::unique_lock lck{*m_parent};
		if (zfs_snapshot(m_parent->raw(), fullname.c_str(), recursive ? B_TRUE : B_FALSE, opts.raw()) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		return m_parent->open_dataset(fullname, dataset_type::snapshot);
	}

	dataset dataset::create_child(const char* name, dataset_type type, const nv_list& opts) {
		if (m_hdl == nullptr) throw std::logic_error("invalid dataset handle");
		std::string fullname{this->name()};
		if (type == dataset_type::snapshot)
			fullname += "@";
		else
			fullname += "/";
		fullname += name;
		std::unique_lock lck{*m_parent};
		if (zfs_create(m_parent->raw(), fullname.c_str(), static_cast<zfs_type_t>(type), opts.raw()) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		return m_parent->open_dataset(fullname, type);
	}

	dataset dataset::clone(const char* name, const nv_list& opts) {
		std::unique_lock lck{*m_parent};
		if (zfs_clone(m_hdl, name, opts.raw()) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
		return m_parent->open_dataset(name, type());
	}

	void dataset::destroy(bool defer) {
		if (m_hdl == nullptr) throw std::logic_error("invalid dataset handle");
		std::unique_lock lck{*m_parent};
		if (zfs_destroy(m_hdl, defer ? B_TRUE : B_FALSE) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void dataset::mount(const std::string& options, int flags) {
		if (m_hdl == nullptr) throw std::logic_error("invalid dataset handle");
		std::unique_lock lck{*m_parent};
		if (zfs_mount(m_hdl, options.c_str(), flags) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void dataset::mount_at(const std::string& mountpoint, const std::string& options, int flags) {
		if (m_hdl == nullptr) throw std::logic_error("invalid dataset handle");
		std::unique_lock lck{*m_parent};
		if (zfs_mount_at(m_hdl, options.c_str(), flags, mountpoint.c_str()) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

	void dataset::unmount(bool force) {
		if (m_hdl == nullptr) throw std::logic_error("invalid dataset handle");
		std::unique_lock lck{*m_parent};
		if (zfs_unmountall(m_hdl, force ? MS_FORCE : 0) != 0)
			throw std::system_error(libzfs_errno(m_parent->raw()), zfs_category());
	}

} // namespace zfspp