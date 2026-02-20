#pragma once
#include "../core/relevance.hpp"
#include <cstdio>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <vector>

int read_key();
void render_search_results(
	const std::vector<std::pair<std::string, std::vector<SearchResult>>>
		&results,
	int selected);

void render_selected_result(
	const std::vector<std::pair<std::string, std::vector<SearchResult>>>
		&results,
	int chosen, int selected);

void open_result(const SearchResult &result);
