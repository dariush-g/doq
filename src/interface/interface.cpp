#include "interface.hpp"
#include <cstdlib>
#include <filesystem>

int read_key() {
	termios old, raw;
	tcgetattr(STDIN_FILENO, &old);
	raw = old;
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);

	char c;
	read(STDIN_FILENO, &c, 1);

	if (c == '\033') {
		char seq[2];
		read(STDIN_FILENO, &seq[0], 1);
		read(STDIN_FILENO, &seq[1], 1);
		tcsetattr(STDIN_FILENO, TCSANOW, &old);
		if (seq[0] == '[') {
			if (seq[1] == 'A')
				return -1; // up
			if (seq[1] == 'B')
				return 1; // down
		}
		return 0;
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return c;
}

void render_search_results(
	const std::vector<std::pair<std::string, std::vector<SearchResult>>>
		&results,
	int selected) {
	for (int i = 0; i < (int)results.size(); i++) {
		std::string name =
			std::filesystem::path(results[i].second[0].name).stem().string();
		// truncate if still too long
		if (name.size() > 40)
			name = name.substr(0, 37) + "...";

		std::cout << "\033[2K";
		if (i == selected)
			std::cout << "\033[7m";
		std::cout << name << std::endl; // " p." << results[i].second[0].page;
		if (i == selected)
			std::cout << "\033[0m";
		std::cout << "\n";
	}
	std::cout.flush();
}

void render_selected_result(
	const std::vector<std::pair<std::string, std::vector<SearchResult>>>
		&results,
	int chosen, int selected) {
	auto filename = results[chosen].first;
	auto pages = results[chosen].second;

	for (int i = 0; i < (int)pages.size(); i++) {
		// truncate if still too long
		if (filename.size() > 40)
			filename = filename.substr(0, 37) + "...";

		std::cout << "\033[2K";
		if (i == selected)
			std::cout << "\033[7m";
		std::cout << filename << " p." << pages[i].page;
		if (i == selected)
			std::cout << "\033[0m";
		std::cout << "\n";
	}
	std::cout.flush();
}

#ifdef __APPLE__
void open_result(const SearchResult &result) {
	// try Skim first for page-level opening
	std::string skim_cmd = "open -a Skim --args -page " +
						   std::to_string(result.page) + " \"" + result.path +
						   "\" 2>/dev/null";

	int ret = system(skim_cmd.c_str());

	// fall back to Preview if Skim not installed
	if (ret != 0) {
		std::string cmd = "open \"" + result.path + "\"";
		system(cmd.c_str());
	}
}
#endif