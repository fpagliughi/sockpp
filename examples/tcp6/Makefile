# Makefile for sample apps from the sockpp library
#

SOCKPP_DIR ?= ../..

SRCS += $(wildcard *.cpp)
EXES = $(SRCS:.cpp=)

CXXFLAGS += -std=c++14 -Wall -g
CPPFLAGS += -I$(SOCKPP_DIR)/include

# debug/release versions
ifdef DEBUG
  DEFS += DEBUG
  CPPFLAGS += -O0
else
  DEFS += _NDEBUG
  CPPFLAGS += -O2 -Wno-unused-result -Werror
endif

COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)

LDFLAGS += -L$(SOCKPP_DIR)/lib
LDLIBS += -lsockpp

%: %.cpp
	$(COMPILE.cc) -o $@ $@.cpp $(LDFLAGS) $(LDLIBS) -pthread

.PHONY: all
all: $(EXES)

.PHONY: clean distclean
clean:
	rm -f $(EXES)

distclean: clean


.PHONY: dump
dump:
	@echo SRCS: $(SRCS)
	@echo EXES: $(EXES)
