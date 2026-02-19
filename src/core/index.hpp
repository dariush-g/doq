#pragma once
#include <string>
#include <vector>

struct Page {
	int number;
	std::string text;
};

struct Document {
	std::string path;
	std::string name;
	std::string extension; // md, pdf, txt
	std::string type;			// text, scanned, empty
	uint64_t size;
	std::vector<Page> pages;
};

std::vector<Document> load_index(const std::string &path);
