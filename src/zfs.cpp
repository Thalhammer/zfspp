#include <fcntl.h>
#include <stdexcept>
#include <sys/stdtypes.h>
#include <system_error>
#include <zfspp.h>

#include <libzfs.h>
#include <sys/fs/zfs.h>

namespace zfspp {

	class zfs_error_category : public std::error_category {
	public:
		const char* name() const noexcept override { return "zfs"; }
		std::string message(int code) const override {
			switch (code) {
			case EZFS_SUCCESS: return "no error -- success";
			case EZFS_NOMEM: return "out of memory";
			case EZFS_BADPROP: return "invalid property value";
			case EZFS_PROPREADONLY: return "cannot set readonly property";
			case EZFS_PROPTYPE: return "property does not apply to dataset type";
			case EZFS_PROPNONINHERIT: return "property is not inheritable";
			case EZFS_PROPSPACE: return "bad quota or reservation";
			case EZFS_BADTYPE: return "dataset is not of appropriate type";
			case EZFS_BUSY: return "pool or dataset is busy";
			case EZFS_EXISTS: return "pool or dataset already exists";
			case EZFS_NOENT: return "no such pool or dataset";
			case EZFS_BADSTREAM: return "bad backup stream";
			case EZFS_DSREADONLY: return "dataset is readonly";
			case EZFS_VOLTOOBIG: return "volume is too large for 32-bit system";
			case EZFS_INVALIDNAME: return "invalid dataset name";
			case EZFS_BADRESTORE: return "unable to restore to destination";
			case EZFS_BADBACKUP: return "backup failed";
			case EZFS_BADTARGET: return "bad attach/detach/replace target";
			case EZFS_NODEVICE: return "no such device in pool";
			case EZFS_BADDEV: return "invalid device to add";
			case EZFS_NOREPLICAS: return "no valid replicas";
			case EZFS_RESILVERING: return "resilvering (healing reconstruction)";
			case EZFS_BADVERSION: return "unsupported version";
			case EZFS_POOLUNAVAIL: return "pool is currently unavailable";
			case EZFS_DEVOVERFLOW: return "too many devices in one vdev";
			case EZFS_BADPATH: return "must be an absolute path";
			case EZFS_CROSSTARGET: return "rename or clone across pool or dataset";
			case EZFS_ZONED: return "used improperly in local zone";
			case EZFS_MOUNTFAILED: return "failed to mount dataset";
			case EZFS_UMOUNTFAILED: return "failed to unmount dataset";
			case EZFS_UNSHARENFSFAILED: return "failed to unshare over nfs";
			case EZFS_SHARENFSFAILED: return "failed to share over nfs";
			case EZFS_PERM: return "permission denied";
			case EZFS_NOSPC: return "out of space";
			case EZFS_FAULT: return "bad address";
			case EZFS_IO: return "I/O error";
			case EZFS_INTR: return "signal received";
			case EZFS_ISSPARE: return "device is a hot spare";
			case EZFS_INVALCONFIG: return "invalid vdev configuration";
			case EZFS_RECURSIVE: return "recursive dependency";
			case EZFS_NOHISTORY: return "no history object";
			case EZFS_POOLPROPS: return "couldn't retrieve pool props";
			case EZFS_POOL_NOTSUP: return "ops not supported for this type of pool";
			case EZFS_POOL_INVALARG: return "invalid argument for this pool operation";
			case EZFS_NAMETOOLONG: return "dataset name is too long";
			case EZFS_OPENFAILED: return "open of device failed";
			case EZFS_NOCAP: return "couldn't get capacity";
			case EZFS_LABELFAILED: return "write of label failed";
			case EZFS_BADWHO: return "invalid permission who";
			case EZFS_BADPERM: return "invalid permission";
			case EZFS_BADPERMSET: return "invalid permission set name";
			case EZFS_NODELEGATION: return "delegated administration is disabled";
			case EZFS_UNSHARESMBFAILED: return "failed to unshare over smb";
			case EZFS_SHARESMBFAILED: return "failed to share over smb";
			case EZFS_BADCACHE: return "bad cache file";
			case EZFS_ISL2CACHE: return "device is for the level 2 ARC";
			case EZFS_VDEVNOTSUP: return "unsupported vdev type";
			case EZFS_NOTSUP: return "ops not supported on this dataset";
			case EZFS_ACTIVE_SPARE: return "pool has active shared spare devices";
			case EZFS_UNPLAYED_LOGS: return "log device has unplayed logs";
			case EZFS_REFTAG_RELE: return "snapshot release: tag not found";
			case EZFS_REFTAG_HOLD: return "snapshot hold: tag already exists";
			case EZFS_TAGTOOLONG: return "snapshot hold/rele: tag too long";
			case EZFS_PIPEFAILED: return "pipe create failed";
			case EZFS_THREADCREATEFAILED: return "thread create failed";
			case EZFS_POSTSPLIT_ONLINE: return "onlining a disk after splitting it";
			case EZFS_SCRUBBING: return "currently scrubbing";
			case EZFS_NO_SCRUB: return "no active scrub";
			case EZFS_DIFF: return "general failure of zfs diff";
			case EZFS_DIFFDATA: return "bad zfs diff data";
			case EZFS_POOLREADONLY: return "pool is in read-only mode";
			case EZFS_SCRUB_PAUSED: return "scrub currently paused";
			case EZFS_ACTIVE_POOL: return "pool is imported on a different system";
			case EZFS_CRYPTOFAILED: return "failed to setup encryption";
			case EZFS_NO_PENDING: return "cannot cancel, no operation is pending";
			case EZFS_CHECKPOINT_EXISTS: return "checkpoint exists";
			case EZFS_DISCARDING_CHECKPOINT: return "currently discarding a checkpoint";
			case EZFS_NO_CHECKPOINT: return "pool has no checkpoint";
			case EZFS_DEVRM_IN_PROGRESS: return "a device is currently being removed";
			case EZFS_VDEV_TOO_BIG: return "a device is too big to be used";
			case EZFS_IOC_NOTSUPPORTED: return "operation not supported by zfs module";
			case EZFS_TOOMANY: return "argument list too long";
			case EZFS_INITIALIZING: return "currently initializing";
			case EZFS_NO_INITIALIZE: return "no active initialize";
			case EZFS_WRONG_PARENT: return "invalid parent dataset (e.g ZVOL)";
			case EZFS_TRIMMING: return "currently trimming";
			case EZFS_NO_TRIM: return "no active trim";
			case EZFS_TRIM_NOTSUP: return "device does not support trim";
			case EZFS_NO_RESILVER_DEFER: return "pool doesn't support resilver_defer";
			case EZFS_EXPORT_IN_PROGRESS: return "currently exporting the pool";
			case EZFS_REBUILDING: return "resilvering (sequential reconstrution)";
			default: return "unknown error";
			}
		}
	};

	const std::error_category& zfs_category() noexcept {
		constinit static zfs_error_category instance;
		return instance;
	}

	zfs::zfs() : m_handle(libzfs_init()) {
		if(m_handle == nullptr)
			throw std::runtime_error("failed to initialize libzfs");
		m_eventfd = open("/dev/zfs", O_RDWR | O_CLOEXEC | O_NONBLOCK);
		if(m_eventfd < 0) {
			auto error = errno;
			libzfs_fini(m_handle);
			throw std::system_error(error, std::system_category());
		}
	}

	zfs::~zfs() {
		std::unique_lock lck{m_mutex};
		if (m_handle) libzfs_fini(m_handle);
		if(m_eventfd >= 0)
			::close(m_eventfd);
	}

	bool zfs::next_event(nv_list& data, size_t* n_dropped, bool block) {
		std::unique_lock lck{m_mutex};
		nvlist_t* nvl{};
        int drop{};
        auto res = zpool_events_next(m_handle, &nvl, &drop, block ? B_FALSE : B_TRUE, m_eventfd);
		data = nv_list(nvl, nv_list::adopt_list{});
		if(res != 0) {
			if(libzfs_errno(m_handle) == EZFS_INTR) return false;
			throw std::system_error(libzfs_errno(m_handle), zfs_category());
		}
		if(n_dropped) *n_dropped = drop;
		return nvl != nullptr;
	}

	bool validate_dataset_name(const char* name, dataset_type dt) noexcept {
		return zfs_name_valid(name, static_cast<zfs_type_t>(dt)) == B_TRUE;
	}

} // namespace zfspp