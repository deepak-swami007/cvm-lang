CXX ?= clang++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pedantic -Iinclude
SANITIZE_FLAGS ?= -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET := build/cvm
SANITIZE_TARGET := build/cvm_sanitize
SOURCES := $(wildcard src/*.cpp)

.PHONY: all run sanitize clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

$(SANITIZE_TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) $(SOURCES) -o $(SANITIZE_TARGET)

run: $(TARGET)
	./$(TARGET)

sanitize: $(SANITIZE_TARGET)

clean:
	rm -rf build
