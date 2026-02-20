#include "core/grouping.hpp"
#include "core/index.hpp"
#include "core/relevance.hpp"
#include "interface/interface.hpp"
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

void handle_exit(int) {
	std::cout << "\033[?1049l"; // restore screen
	std::cout << "\033[?25h";	// show cursor
	std::exit(0);
}

int main(int argc, char *argv[]) {
	std::signal(SIGINT, handle_exit);
	std::signal(SIGTERM, handle_exit);

	if (argc < 2) {
		std::cerr << "Usage: doq <queries>\n";
		return 1;
	}

	std::vector<std::string> queries;
	for (int i = 1; i < argc; i++) {
		queries.push_back(argv[i]);
	}

	std::string query;
	for (auto q : queries) {
		query.append(q + ' ');
	}

	auto start = std::chrono::high_resolution_clock::now();

	std::vector<Document> docs = load_index("index.bin");

	auto loaded_index_time = std::chrono::high_resolution_clock::now();

	BM25 bm25(docs);
	const auto results = bm25.search(query);

	auto found_time = std::chrono::high_resolution_clock::now();

	auto grouped_results = group_results(results);

	auto grouped_time = std::chrono::high_resolution_clock::now();

	// for (auto &r : results) {
	// 	std::cout << r.name << " p." << r.page << " (score: " << r.score
	// 			  << ")\n";
	// }

	std::cout << "Loaded index in "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					 loaded_index_time - start)
					 .count()
			  << "ms" << std::endl;

	std::cout << "Searched in "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					 found_time - loaded_index_time)
					 .count()
			  << "ms" << std::endl;

	std::cout << "Grouped in "
			  << std::chrono::duration_cast<std::chrono::milliseconds>(
					 grouped_time - found_time)
					 .count()
			  << "ms" << std::endl;

	return 0;

	int selected = 0;

	if (results.empty()) {
		std::cout << "No results found\n";
		return 0;
	}

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