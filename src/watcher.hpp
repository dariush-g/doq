#pragma once
#include <functional>
#include <string>

void start_file_watcher(const std::string &dir,
						std::function<void(const std::string &)> on_change);