#include "watcher.hpp"
#include <CoreServices/CoreServices.h>
#include <iostream>

static std::function<void(const std::string &)> g_callback;

void fs_callback(ConstFSEventStreamRef, void *, size_t numEvents,
				 void *eventPaths, const FSEventStreamEventFlags *flags,
				 const FSEventStreamEventId *) {
	char **paths = (char **)eventPaths;
	for (size_t i = 0; i < numEvents; i++) {
		// only care about actual file changes, not directories
		if (flags[i] & kFSEventStreamEventFlagItemIsFile) {
			g_callback(std::string(paths[i]));
		}
	}
}

void start_file_watcher(const std::string &dir,
						std::function<void(const std::string &)> on_change) {
	g_callback = on_change;

	CFStringRef cf_path =
		CFStringCreateWithCString(nullptr, dir.c_str(), kCFStringEncodingUTF8);
	CFArrayRef cf_paths =
		CFArrayCreate(nullptr, (const void **)&cf_path, 1, nullptr);

	FSEventStreamRef stream = FSEventStreamCreate(
		nullptr, &fs_callback, nullptr, cf_paths, kFSEventStreamEventIdSinceNow,
		1.0, // 1 second latency before callback fires
		kFSEventStreamCreateFlagFileEvents);

	dispatch_queue_t queue =
		dispatch_queue_create("doq.watcher", DISPATCH_QUEUE_SERIAL);
	FSEventStreamSetDispatchQueue(stream, queue);
	FSEventStreamStart(stream);
}