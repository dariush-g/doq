#include "core/grouping.hpp"
#include "core/index.hpp"
#include "core/relevance.hpp"
#include "watcher.hpp"
#include <iostream>
#include <msgpack.hpp>
#include <mutex>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define SOCKET_PATH "/tmp/doq.sock"

std::mutex index_mutex;
static std::unordered_map<std::string, std::chrono::steady_clock::time_point>
	last_seen;

int main() {
	std::vector<Document> docs = load_index("index.bin");
	BM25 bm25(docs);

	// create socket
	int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
	unlink(SOCKET_PATH);
	bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
	listen(server_fd, 5);

	std::thread watcher_thread([&]() {
		start_file_watcher(
			"/Users/dariushghassemieh/Documents", [&](const std::string &path) {
				auto now = std::chrono::steady_clock::now();
				if (last_seen.count(path) &&
					now - last_seen[path] < std::chrono::seconds(2))
					return;
				last_seen[path] = now;

				std::cout << "file changed: " << path << "\n";
				system(("python3 "
						"/Users/dariushghassemieh/doq/src/indexer/main.py " +
						path + " --file")
						   .c_str());
				std::lock_guard<std::mutex> lock(index_mutex);
				docs = load_index("index.bin");
				bm25 = BM25(docs);
			});
	});
	watcher_thread.detach();

	while (true) {
		int client_fd = accept(server_fd, nullptr, nullptr);

		char buf[1024];
		int n = read(client_fd, buf, sizeof(buf) - 1);
		buf[n] = '\0';
		std::string query(buf);

		msgpack::sbuffer buffer;
		msgpack::packer<msgpack::sbuffer> pk(&buffer);

		{
			std::lock_guard<std::mutex> lock(index_mutex);
			auto results = bm25.search(query);
			auto grouped_results = group_results(results);

			pk.pack_array(grouped_results.size());
			for (auto &r : grouped_results) {
				pk.pack_array(2);
				pk.pack(r.first);

				pk.pack_array(r.second.size());
				for (auto &sr : r.second) {
					pk.pack_array(4);
					pk.pack(sr.path);
					pk.pack(sr.name);
					pk.pack(sr.page);
					pk.pack(sr.score);
				}
			}
		}

		uint32_t size = buffer.size();
		write(client_fd, &size, sizeof(size));
		write(client_fd, buffer.data(), buffer.size());
		close(client_fd);
	}
}