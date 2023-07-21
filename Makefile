CXX := g++
SRCDIR := src

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
CXXFLAGS := -Wall -Werror -Wpedantic -O3
INC := -I lib -I include

build:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) $(SOURCES) -o bin/hexdump

.PHONY: clean
clean:
	rm -rf bin