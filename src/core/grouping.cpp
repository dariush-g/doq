#include "grouping.hpp"
#include <unordered_map>

std::vector<std::pair<std::string, std::vector<SearchResult>>>
group_results(const std::vector<SearchResult> &results) {
	std::unordered_map<std::string, std::vector<SearchResult>> grouped;

	for (const auto &r : results) {
		grouped[r.name].push_back(r);
	}

	std::vector<std::pair<std::string, std::vector<SearchResult>>> ordered;
	for (auto &[name, pages] : grouped) {
		// sort pages within each doc by score descending
		std::sort(pages.begin(), pages.end(),
				  [](const SearchResult &a, const SearchResult &b) {
					  return a.score > b.score;
				  });
		ordered.push_back({name, pages});
	}

	// sort documents by their best page's score
	std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
		return a.second[0].score > b.second[0].score;
	});

	return ordered;
}