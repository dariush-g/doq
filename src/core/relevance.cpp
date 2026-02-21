#include "relevance.hpp"
#include "../flags.hpp"
#include "fuzzy.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <unordered_set>

static const double K1 = 1.5;
static const double B = 0.75;

std::vector<std::string> BM25::tokenize(const std::string &text) {
	std::vector<std::string> tokens;
	std::string token;
	for (char c : text) {
		if (std::isalnum(c)) {
			token += std::tolower(c);
		} else if (!token.empty()) {
			tokens.push_back(token);
			token.clear();
		}
	}
	if (!token.empty())
		tokens.push_back(token);
	return tokens;
}

BM25::BM25(const std::vector<Document> &docs) : docs_(docs) {
	inverted_index_ = load_inv_index("inv_index.bin");
	load_bm25_meta("bm25_meta.bin", doc_freq_, page_lengths_, avg_page_len_,
				   total_pages_);
}

double BM25::idf(const std::string &term) {
	int df = doc_freq_.count(term) ? doc_freq_.at(term) : 0;
	return std::log((total_pages_ - df + 0.5) / (df + 0.5) + 1.0);
}

std::vector<SearchResult> BM25::search(const std::string &query, uint32_t flags,
									   int top_k) {
	auto tokenized = tokenize(query);
	auto query_terms = std::vector<std::string>();

	for (auto &q : tokenized) {
		if (inverted_index_.count(q))
			query_terms.push_back(q);
		else if (!(flags & NO_FUZZY)) {
			query_terms.push_back(best_match(q, inverted_index_));
		}
	}

	// accumulate scores per (doc, page)
	std::unordered_map<int, std::unordered_map<int, double>> scores;

	if (!(flags & NO_TEXT)) {
		for (auto &term : query_terms) {
			auto it = inverted_index_.find(term);
			if (it == inverted_index_.end())
				continue;
			if ((int)it->second.size() > 5000)
				continue;
			double term_idf = idf(term);

			for (auto &[di, pi, tf] : it->second) {
				int page_len = page_lengths_[di][pi];
				double numerator = tf * (K1 + 1);
				double denominator =
					tf + K1 * (1 - B + B * (page_len / avg_page_len_));
				scores[di][pi] += term_idf * (numerator / denominator);
			}
		}
	}
	
	std::vector<SearchResult> results;
	for (auto &[di, page_scores] : scores) {
		for (auto &[pi, score] : page_scores) {
			results.push_back({docs_[di].path, docs_[di].name,
							   docs_[di].pages[pi].number, score});
		}
	}

	std::partial_sort(
		results.begin(), results.begin() + std::min(top_k, (int)results.size()),
		results.end(), [](const SearchResult &a, const SearchResult &b) {
			return a.score > b.score;
		});

	results.resize(std::min(top_k, (int)results.size()));

	return results;
}