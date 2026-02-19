CXX = g++
CXXFLAGS = -std=c++20 -Wall -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib

SRC = src/main.cpp src/core/index.cpp src/core/relevance.cpp src/interface/interface.cpp
TARGET = doq

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)