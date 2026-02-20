#include "fuzzy.hpp"
#include <rapidfuzz/fuzz.hpp>
#include <sstream>

std::string best_match(
	const std::string &term,
	const std::unordered_map<
		std::string, std::vector<std::tuple<int, int, int>>> &inv_index) {
	std::string best;
	double best_score = 0.0;

	for (const auto &[key, _] : inv_index) {
		if (std::abs((int)key.size() - (int)term.size()) > 2)
			continue;
		double score = rapidfuzz::fuzz::ratio(term, key);
		if (score > best_score) {
			best_score = score;
			best = key;
		}
	}

	if (best_score > 80.0)
		return best;
	return term;
}
