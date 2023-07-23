CXX := g++

SRCDIR := src
BINDIR := bin

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
CXXFLAGS := -Wall -Werror -Wpedantic -O3
INC := -I lib -I include

ifeq ($(OS),Windows_NT)
# CXXFLAGS += -std=c++17 -static-libgcc -static-libstdc++
endif

build:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) $(SOURCES) -o $(BINDIR)/hexdump

.PHONY: clean
clean:
	rm -rf bin