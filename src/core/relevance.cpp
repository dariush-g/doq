#include "relevance.hpp"
#include <algorithm>
#include <cmath>
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

	inverted_index_ = load_inv_index("inv_index.bin");

	for (int di = 0; di < (int)docs_.size(); di++) {
		for (int pi = 0; pi < (int)docs_[di].pages.size(); pi++) {
			auto tokens = tokenize(docs_[di].pages[pi].text);
			int page_len = tokens.size();
			total_len += page_len;
			total_pages_++;

			std::unordered_map<std::string, int> tf;
			for (auto &t : tokens)
				tf[t]++;

			std::unordered_set<std::string> seen;
			for (auto &[term, freq] : tf) {
				// inverted_index_[term].emplace_back(di, pi, freq);
				if (seen.insert(term).second) {
					doc_freq_[term]++;
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

	// accumulate scores per (doc, page)
	std::unordered_map<int, std::unordered_map<int, double>> scores;

	for (auto &term : query_terms) {
		if (!inverted_index_.count(term))
			continue;
		double term_idf = idf(term);

		for (auto &[di, pi, tf] : inverted_index_.at(term)) {
			int page_len = docs_[di].pages[pi].text.size();
			double numerator = tf * (K1 + 1);
			double denominator =
				tf + K1 * (1 - B + B * (page_len / avg_page_len_));
			scores[di][pi] += term_idf * (numerator / denominator);
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