#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct Page {
	int number;
	std::string text;
};

struct Document {
	std::string path;
	std::string name;
	std::string extension; // md, pdf, txt
	std::string type;	   // text, scanned, empty
	uint64_t size;
	std::vector<Page> pages;
};

std::vector<Document> load_index(const std::string &path);

std::unordered_map<std::string, std::vector<std::tuple<int, int, int>>>
load_inv_index(const std::string &path);

void load_bm25_meta(
	const std::string &path, std::unordered_map<std::string, int> &doc_freq,
	std::unordered_map<int, std::unordered_map<int, int>> &page_lengths,
	double &avg_page_len, int &total_pages);