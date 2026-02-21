#include "core/grouping.hpp"
#include "core/index.hpp"
#include "core/relevance.hpp"
#include "flags.hpp"
#include "interface/interface.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <msgpack.hpp>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

#define SOCKET_PATH "/tmp/doq.sock"

void handle_exit(int) {
	std::cout << "\033[?1049l"; // restore screen
	std::cout << "\033[?25h";	// show cursor
	std::exit(0);
}

int main(int argc, char *argv[]) {
	std::signal(SIGINT, handle_exit);
	std::signal(SIGTERM, handle_exit);

	uint32_t flags = 0;

	if (argc < 2) {
		std::cerr << "Usage: doq <queries>\n";
		return 1;
	}

	std::vector<std::string> queries;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--no-fuzzy") == 0) {
			flags |= NO_FUZZY;
			continue;
		}
		if (strcmp(argv[i], "--no-scanned") == 0) {
			flags |= NO_SCANNED;
			continue;
		}
		if (strcmp(argv[i], "--no-text") == 0) {
			flags |= NO_TEXT;
			continue;
		}
		if (strcmp(argv[i], "--verbose") == 0) {
			flags |= ADVANCED;
			continue;
		}

		queries.push_back(argv[i]);
	}

	std::string query;
	for (auto q : queries) {
		query.append(q + ' ');
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		std::cerr << "could not connect to doqd — is it running?\n";
		return 1;
	}

	// send flags then query
	write(fd, &flags, sizeof(flags));
	write(fd, query.c_str(), query.size());

	uint32_t size;
	read(fd, &size, sizeof(size));

	std::string buf(size, '\0');
	size_t total = 0;
	while (total < size) {
		int n = read(fd, buf.data() + total, size - total);
		if (n <= 0)
			break;
		total += n;
	}

	close(fd);
	msgpack::object_handle oh = msgpack::unpack(buf.data(), buf.size());
	msgpack::object obj = oh.get();

	std::vector<std::pair<std::string, std::vector<SearchResult>>>
		grouped_results;

	auto &outer = obj.via.array;

	for (size_t i = 0; i < outer.size; i++) {
		auto &pair = outer.ptr[i].via.array;
		std::string filename = pair.ptr[0].as<std::string>();

		std::vector<SearchResult> pages;
		auto &pages_arr = pair.ptr[1].via.array;
		for (size_t j = 0; j < pages_arr.size; j++) {
			auto &sr = pages_arr.ptr[j].via.array;
			SearchResult r;
			r.path = sr.ptr[0].as<std::string>();
			r.name = sr.ptr[1].as<std::string>();
			r.page = sr.ptr[2].as<int>();
			r.score = sr.ptr[3].as<double>();
			pages.push_back(r);
		}

		grouped_results.push_back({filename, pages});
	}

	if (grouped_results.empty()) {
		std::cerr << "No results found\n";
		return 0;
	}

	int selected = 0;

	// Enter alternate screen
	std::cout << "\033[?1049h";
	// std::cout << "\033[?25l";
	auto page_index = 0;
	while (true) {
		std::cout << "\033[H\033[2J"; // move to top-left, clear screen
		render_search_results(grouped_results, selected);

		int key = read_key();
		if (key == -1) {
			selected = std::max(0, selected - 1);
		} else if (key == 1) {
			selected = std::min((int)grouped_results.size() - 1, selected + 1);
		} else if (key == '\n') {
			while (true) {
				std::cout << "\033[H\033[2J"; // move to top-left, clear screen
				render_selected_result(grouped_results, selected, page_index);

				int key = read_key();
				if (key == -1) {
					page_index = std::max(0, page_index - 1);
				} else if (key == 1) {
					page_index = std::min(
						(int)grouped_results[selected].second.size() - 1,
						page_index + 1);
				} else if (key == 'q') {
					break;
				} else if (key == '\n') {
					std::cout << "\033[?1049l";
					// show cursor again
					std::cout << "\033[?25h";

					open_result(grouped_results[selected].second[page_index]);

					return 0;
				}
			}
		}
	}

	// Exit alternate screen
	std::cout << "\033[?1049l";
	// show cursor again
	std::cout << "\033[?25h";

	open_result(grouped_results[selected].second[page_index]);

	return 0;
}