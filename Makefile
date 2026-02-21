CXX = g++
CXXFLAGS = -std=c++20 -Wall -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib

SRC = src/main.cpp src/core/index.cpp src/core/relevance.cpp src/interface/interface.cpp src/core/grouping.cpp src/core/fuzzy.cpp
DAEMON_SRC = src/daemon.cpp src/core/index.cpp src/core/relevance.cpp src/core/grouping.cpp src/core/fuzzy.cpp $(WATCHER_SRC)

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    WATCHER_SRC = src/watcher_mac.cpp
    LDFLAGS += -framework CoreServices
else
    WATCHER_SRC = src/watcher_linux.cpp
endif

all: doq doqd

doq: $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o doq

doqd: $(DAEMON_SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DAEMON_SRC) -o doqd

clean:
	rm -f doq doqd