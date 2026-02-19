#include "index.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: doq <index.bin>\n";
		return 1;
	}

	std::vector<Document> docs = load_index(argv[1]);

	std::cout << "Loaded " << docs.size() << " documents\n";
	for (auto &doc : docs) {
		std::cout << doc.name << " [" << doc.type << "] - " << doc.pages.size()
				  << " pages\n";
	}

	return 0;
}