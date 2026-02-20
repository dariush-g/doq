#pragma once
#include "index.hpp"

std::string best_match(
	const std::string &term,
	const std::unordered_map<
		std::string, std::vector<std::tuple<int, int, int>>> &inv_index);
