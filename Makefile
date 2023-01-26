CXX := g++
SRCDIR := src

SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
CXXFLAGS := -Wall -Werror -O3
INC := -I lib

build:
	@mkdir -p bin
	@echo Compiling...
	@$(CXX) $(CXXFLAGS) $(INC) $(SOURCES) -o bin/hexdump
	@echo Compilation Complete