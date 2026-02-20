#pragma once
#include "index.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct SearchResult {
	std::string path;
	std::string name;
	int page;
	double score;
};

class BM25 {
public:
	explicit BM25(const std::vector<Document> &docs);
	std::vector<SearchResult> search(const std::string &query, int top_k = 100);

private:
	const std::vector<Document> &docs_;
	double avg_page_len_;
	int total_pages_;

	std::unordered_map<std::string, int> doc_freq_;
	// term -> list of (doc_index, page_index, term_frequency)
	std::unordered_map<std::string, std::vector<std::tuple<int, int, int>>>
		inverted_index_;

	std::vector<std::string> tokenize(const std::string &text);
	double idf(const std::string &term);
};