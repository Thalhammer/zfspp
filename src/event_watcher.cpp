#include <cstdint>
#include <stdexcept>
#include <zfspp.h>

#include <csignal>
#include <mutex>

namespace zfspp {

	event_watcher::event_watcher(zfs& parent)
        : m_parent(&parent)
    {}
	event_watcher::~event_watcher() {
        this->stop();
    }

	void event_watcher::set_checkpoint(std::array<uint64_t, 3> checkpoint) {
        std::unique_lock<std::mutex> lck{m_mtx};
        std::copy(checkpoint.begin(), checkpoint.end(), m_checkpoint.begin());
    }

	std::array<uint64_t, 3> event_watcher::checkpoint() const noexcept {
        std::unique_lock<std::mutex> lck{m_mtx};
        return m_checkpoint;
    }

	void event_watcher::set_on_event(std::function<void(const nv_list&)> cb) {
        std::unique_lock<std::mutex> lck{m_mtx};
        m_on_event = cb;
    }

	void event_watcher::set_on_drop(std::function<void(size_t)> cb) {
        std::unique_lock<std::mutex> lck{m_mtx};
        m_on_drop = cb;
    }

	void event_watcher::set_on_error(std::function<void()> cb) {
        std::unique_lock<std::mutex> lck{m_mtx};
        m_on_error = cb;
    }


	void event_watcher::start() {
        std::unique_lock<std::mutex> lck{m_mtx};
        m_should_stop = false;
        m_watcher_thread = std::thread([this](){
		    signal(SIGUSR1, [](int) {});
            try {
                this->thread_fn();
            } catch(...) {
                std::unique_lock<std::mutex> lck{m_mtx};
                if(m_on_error) m_on_error();
            }
            m_is_started = false;
        });
        m_is_started = true;
    }

	void event_watcher::stop() {
        std::unique_lock<std::mutex> lck{m_mtx};
        m_should_stop = true;
        if(m_watcher_thread.joinable()) {
            pthread_kill(m_watcher_thread.native_handle(), SIGUSR1);
            m_watcher_thread.join();
        }
        if(m_is_started != false) throw std::logic_error("ASSERT_FAILED");
    }

    namespace {
        std::array<uint64_t, 3> parse_checkpoint(const nv_list& info) {
            auto time = info.find("time");
            auto eid = info.find("eid");
            if(time == info.end() || eid == info.end()) return {};
            if(time.type() != nv_type::int64_array || eid.type() != nv_type::uint64) return {};
            auto time_val = time.as_int64_array();
            auto eid_val = eid.as_uint64();
            if(time_val.size() != 2) return {};
            return {eid_val, static_cast<uint64_t>(time_val[0]), static_cast<uint64_t>(time_val[1])};
        }
    }

    void event_watcher::thread_fn() {
		zfspp::nv_list info{};
		size_t n_dropped{};
        while(m_parent->next_event(info, &n_dropped, false)) {
            auto chk = parse_checkpoint(info);
            if(chk[0] == 0 && chk[1] == 0 && chk[2] == 0) continue;
            if(chk[1] < m_checkpoint[1] || (chk[1] == m_checkpoint[1] && chk[2] <= m_checkpoint[2])) continue;
            std::unique_lock<std::mutex> lck{m_mtx};
            m_checkpoint = chk;
            if(!info.empty() && m_on_event) m_on_event(info);
            break;
        }
		while (!m_should_stop) {
			if (!m_parent->next_event(info, &n_dropped, true) && m_should_stop) break;
            std::unique_lock<std::mutex> lck{m_mtx};
            auto chk = parse_checkpoint(info);
            if(!info.empty() && m_on_event) m_on_event(info);
            if(n_dropped != 0 && m_on_drop) m_on_drop(n_dropped);
            if(chk[0] != 0 || chk[1] != 0 || chk[2] != 0)
                m_checkpoint = chk;
            if(m_should_stop) break;
		}
    }

} // namespace zfspp