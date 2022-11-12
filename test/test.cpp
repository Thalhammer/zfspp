#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <pthread.h>
#include <sys/select.h>
#include <system_error>
#include <thread>
#include <zfeature_common.h>
#include <zfspp.h>

#include <libzfs.h>

#include <fcntl.h>
#include <sys/fs/zfs.h>
#include <sys/nvpair.h>
#include <unistd.h>

TEST(ZFSPP_Test, Dummy) {
	if (0) {
		zfspp::zfs client;
        zfspp::event_watcher watcher(client);
        std::array<uint64_t, 3> skip{421, 1668258730, 926835398};
        watcher.set_checkpoint(skip);
        watcher.set_on_event([](const zfspp::nv_list& info){
            std::cout << info.to_json() << std::endl;
        });
        watcher.set_on_drop([](size_t n_dropped){
            std::cout << "Dropped " << n_dropped << " events" << std::endl;
        });
        watcher.start();
        std::cin.get();
        watcher.stop();
        for(auto e : watcher.checkpoint())
            std::cout << e << " ";
        std::cout << std::endl;
		std::cout << "Exit" << std::endl;
	}
    if(0) {
        zfspp::zfs client;
        auto fs = client.root_datasets()[0];
        std::cout << fs.properties().to_json() << std::endl;
        std::cout << fs.user_properties().to_json() << std::endl;
        fs.set_property("random:user", "hello world");
    }
    if(0) {
        zfspp::zfs client;
        auto pool = client.open_pool("testpool");
        std::cout << pool.config().to_json(true) << std::endl;
        std::cout << pool.features().to_json() << std::endl;
    }
    if(0) {
        zfspp::zfs client;
        zfspp::nv_list child;
        child.add_string("type", "file");
        child.add_uint64("is_log", 0);
        child.add_string("path", "/home/dominik/Dokumente/zfspp/build/testfs2");
        child.add_uint64("ashift", 9);

        zfspp::nv_list root;
        root.add_string("type", "root");
        root.add_nvlist_array("children", {&child, 1});

        std::cout << "create" << std::endl;
        auto pool = client.create_pool("testpool2", root, {}, {});
        std::cout << "done" << std::endl;
        std::cout << pool.config().to_json() << std::endl;
        std::cout << pool.features().to_json() << std::endl;
        pool.destroy();
    }
    if(1) {
        zfspp::zfs client;
        std::string reason;
        client.validate_dataset_name("helloworld", zfspp::dataset_type::filesystem, &reason);
        std::cout << reason << std::endl;
    }
}