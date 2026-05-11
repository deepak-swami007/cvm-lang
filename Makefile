CXX ?= clang++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pedantic -Iinclude
TARGET := build/cvm
SOURCES := $(wildcard src/*.cpp)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) examples/variables.cvm

clean:
	rm -rf build

