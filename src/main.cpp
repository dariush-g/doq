#include "core/index.hpp"
#include "core/relevance.hpp"
#include "interface/interface.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
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

	std::vector<Document> docs = load_index("index.bin");

	BM25 bm25(docs);
	auto results = bm25.search(query);
	// for (auto &r : results) {
	// 	std::cout << r.name << " p." << r.page << " (score: " << r.score
	// 			  << ")\n";
	// }

	int selected = 0;
	std::cout << "\033[s"; // save cursor position
	render_search_results(results, selected);

	if (results.size() == 0) {
		std::cout << "No text results found" << std::endl;
		return 0;
	}

	while (true) {
		int key = read_key();
		if (key == -1)
			selected = std::max(0, selected - 1);
		if (key == 1)
			selected = std::min((int)results.size() - 1, selected + 1);
		if (key == '\n')
			break;

		std::cout << "\033[u"; // restore cursor to saved position
		render_search_results(results, selected);
	}

	open_result(results[selected]);

	return 0;
}