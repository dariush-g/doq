#pragma once
#include "relevance.hpp"
#include <string>
#include <vector>

std::vector<std::pair<std::string, std::vector<SearchResult>>>
group_results(const std::vector<SearchResult> &results);