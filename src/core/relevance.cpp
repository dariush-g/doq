#include "relevance.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>
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
	total_pages_ = 0;
	double total_len = 0;

	for (auto &doc : docs_) {
		for (auto &page : doc.pages) {
			auto tokens = tokenize(page.text);
			total_len += tokens.size();
			total_pages_++;

			// count unique terms per page for doc_freq
			std::unordered_set<std::string> seen;
			for (auto &t : tokens) {
				if (seen.insert(t).second) {
					doc_freq_[t]++;
				}
			}
		}
	}

	avg_page_len_ = total_pages_ > 0 ? total_len / total_pages_ : 1.0;
}

double BM25::idf(const std::string &term) {
	int df = doc_freq_.count(term) ? doc_freq_.at(term) : 0;
	return std::log((total_pages_ - df + 0.5) / (df + 0.5) + 1.0);
}

std::vector<SearchResult> BM25::search(const std::string &query, int top_k) {
	auto query_terms = tokenize(query);
	std::vector<SearchResult> results;

	for (auto &doc : docs_) {
		for (auto &page : doc.pages) {
			auto tokens = tokenize(page.text);
			int page_len = tokens.size();
			if (page_len == 0)
				continue;

			// term frequency map for this page
			std::unordered_map<std::string, int> tf;
			for (auto &t : tokens)
				tf[t]++;

			double score = 0.0;
			for (auto &term : query_terms) {
				int f = tf.count(term) ? tf.at(term) : 0;
				if (f == 0)
					continue;
				double numerator = f * (K1 + 1);
				double denominator =
					f + K1 * (1 - B + B * (page_len / avg_page_len_));
				score += idf(term) * (numerator / denominator);
			}

			if (score > 0) {
				results.push_back({doc.path, doc.name, page.number, score});
			}
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