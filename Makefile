CXX = g++
CXXFLAGS = -std=c++20 -Wall -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib

SRC = src/core/main.cpp src/core/index.cpp
TARGET = doq

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)