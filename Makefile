CXX := g++

SRCDIR := src
BINDIR := bin
BINNAME := hexdump
TESTDIR := test

SRCEXT := cpp
SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT))
CXXFLAGS := -Wall -Werror -Wpedantic
OPTFLAGS := -O3
INC := -I lib -I include

COMPILECMD := $(CXX) $(CXXFLAGS) $(DEBUG) $(OPTFLAGS) $(INC) $(SOURCES) -o

build:
	mkdir -p $(BINDIR)
	$(COMPILECMD) $(BINDIR)/$(BINNAME)

.PHONY: test
test:
	mkdir -p $(BINDIR)
	mkdir -p $(BINDIR)/$(TESTDIR)
	$(COMPILECMD) $(BINDIR)/$(TESTDIR)/$(BINNAME)

.PHONY: debug
debug:
	mkdir -p $(BINDIR)
	$(COMPILECMD) $(BINDIR)/$(BINNAME)-debug -g

.PHONY: clean
clean:
	rm -rf bin