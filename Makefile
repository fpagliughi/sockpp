# Make file for the sockpp library
#
# This will compile all the .cpp files in the directory into the library.
# Any files to be excluded should be placed in the variable SRC_IGNORE, like:
# SRC_IGNORE = this.c that.cpp
#

MODULE = sockpp

# Define CROSS_COMPILE to specify a prefix for GCC
#CROSS_COMPILE=arm-linux-gnueabihf-

# ----- Tools -----

ifndef VERBOSE
  QUIET := @
endif

INSTALL ?= install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA =  $(INSTALL) -m 644

# ----- Directories -----

SRC_DIR ?= src
INC_DIR ?= include
HDR_DIR ?= $(INC_DIR)/$(MODULE)

EXAMPLES_DIR ?= examples
TESTS_DIR ?= tests
UNITS_DIR ?= tests/units

LIB_DIR ?= $(CROSS_COMPILE)lib
OBJ_DIR ?= $(CROSS_COMPILE)obj

vpath %.cpp $(SRC_DIR) $(SRC_DIR)/unix

INC_DIRS += $(INC_DIR)

prefix ?= /usr/local
exec_prefix ?= ${prefix}

includedir = $(prefix)/include/$(MODULE)
libdir = $(exec_prefix)/lib

_MK_OBJ_DIR := $(shell mkdir -p $(OBJ_DIR))
_MK_LIB_DIR := $(shell mkdir -p $(LIB_DIR))

ifeq "$(MAKECMDGOALS)" "install"
  _MK_INSTALL_INC_DIR := $(shell mkdir -p $(DESTDIR)$(includedir))
  _MK_INSTALL_LIB_DIR := $(shell mkdir -p $(DESTDIR)$(libdir))
endif

# ----- Definitions for the shared library -----

LIB_BASE  ?= $(MODULE)
LIB_MAJOR ?= 0
LIB_MINOR ?= 3

LIB_LINK = lib$(LIB_BASE).so
LIB_MAJOR_LINK = $(LIB_LINK).$(LIB_MAJOR)

LIB = $(LIB_MAJOR_LINK).$(LIB_MINOR)

TGT = $(LIB_DIR)/$(LIB)

# ----- Sources -----

SRCS += $(notdir $(wildcard $(SRC_DIR)/*.cpp))
SRCS += $(notdir $(wildcard $(SRC_DIR)/unix/*.cpp))
HDRS += $(notdir $(wildcard $(HDR_DIR)/*.h))

ifdef SRC_IGNORE
  SRCS := $(filter-out $(SRC_IGNORE),$(SRCS))
endif

OBJS := $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))

# ----- Dependencies created with .dep suffix in obj dir -----

DEPS := $(OBJS:.o=.dep)

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJ_DIR)/$*.Tdep
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

POST_COMPILE = mv -f $(OBJ_DIR)/$*.Tdep $(OBJ_DIR)/$*.dep

# ----- Compiler flags, etc -----

ifneq ($(CROSS_COMPILE),)
  CC  = $(CROSS_COMPILE)gcc
  CXX = $(CROSS_COMPILE)g++
  AR  = $(CROSS_COMPILE)ar
  LD  = $(CROSS_COMPILE)ld
endif

CPPFLAGS += -Wall -fPIC

# We need at least C++14 support, though newer should work fine.
CXXFLAGS += -std=c++14

ifdef DEBUG
  DEFS += DEBUG
  CPPFLAGS += -g -O0
else
  DEFS += _NDEBUG
  CPPFLAGS += -O2 -Wno-unused-result -Werror
endif

CPPFLAGS += $(addprefix -D,$(DEFS)) $(addprefix -I,$(INC_DIRS))

LIB_DEPS += c stdc++ pthread

LIB_DEP_FLAGS += $(addprefix -l,$(LIB_DEPS))

LDFLAGS := -g -shared -Wl,-soname,$(LIB_MAJOR_LINK) -L$(LIB_DIR)

# ----- Compiler directives -----

$(OBJ_DIR)/%.o: %.cpp $(OBJ_DIR)/%.dep
	@echo $(CXX) $<
	$(QUIET) $(COMPILE.cpp) $(OUTPUT_OPTION) $<
	$(QUIET) $(POST_COMPILE)

# ----- Build targets -----

.PHONY: all
all: lib

.PHONY: lib
lib: $(TGT) $(LIB_DIR)/$(LIB_LINK) $(LIB_DIR)/$(LIB_MAJOR_LINK)

$(TGT): $(OBJS)
	@echo Creating library: $@
	$(QUIET) $(CC) $(LDFLAGS) -o $@ $^ $(LIB_DEP_FLAGS)

$(LIB_DIR)/$(LIB_LINK): $(LIB_DIR)/$(LIB_MAJOR_LINK)
	$(QUIET) cd $(LIB_DIR) ; $(RM) $(LIB_LINK) ; ln -s $(LIB_MAJOR_LINK) $(LIB_LINK)

$(LIB_DIR)/$(LIB_MAJOR_LINK): $(TGT)
	$(QUIET) cd $(LIB_DIR) ; $(RM) $(LIB_MAJOR_LINK) ; ln -s $(LIB) $(LIB_MAJOR_LINK)

.PHONY: dump
dump:
	@echo LIB=$(LIB)
	@echo TGT=$(TGT)
	@echo LIB_DIR=$(LIB_DIR)
	@echo OBJ_DIR=$(OBJ_DIR)
	@echo INC_DIRS=$(INC_DIRS)
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo CXX=$(CXX)
	@echo COMPILE.cpp=$(COMPILE.cpp)
	@echo SRCS=$(SRCS)
	@echo HDRS=$(HDRS)
	@echo OBJS=$(OBJS)
	@echo DEPS:$(DEPS)
	@echo LIB_DEPS=$(LIB_DEPS)

.PHONY: clean
clean:
	$(QUIET) $(RM) $(TGT) $(LIB_DIR)/$(LIB_LINK) $(LIB_DIR)/$(LIB_MAJOR_LINK) \
	    $(OBJS)

.PHONY: distclean
distclean: clean
	$(QUIET) rm -rf $(OBJ_DIR) $(LIB_DIR)

.PHONY: examples examples_tcp examples_unix

examples: examples_tcp examples_unix

examples_tcp: $(EXAMPLES_DIR)/tcp lib
	$(MAKE) -C $<

examples_unix: $(EXAMPLES_DIR)/unix lib
	$(MAKE) -C $<

# ----- Installation targets -----

strip_options:
	$(eval INSTALL_OPTS := -s)

install-strip: $(TGT) strip_options install

install: $(TGT)
	for fil in $(HDRS) ; do $(INSTALL_DATA) ${INSTALL_OPTS} $(HDR_DIR)/$${fil} $(DESTDIR)${includedir} ; done
	$(INSTALL_DATA) ${INSTALL_OPTS} ${TGT} $(DESTDIR)${libdir}
	cd $(DESTDIR)${libdir} ; ln -s $(LIB) $(LIB_MAJOR_LINK)
	cd $(DESTDIR)${libdir} ; ln -s $(LIB_MAJOR_LINK) $(LIB_LINK)

uninstall:
	rm $(DESTDIR)${libdir}/${LIB}
	rm $(DESTDIR)${libdir}/$(LIB_MAJOR_LINK)
	rm $(DESTDIR)${libdir}/$(LIB_LINK)

# ----- Header dependencies -----

$(OBJ_DIR)/%.dep: ;
.PRECIOUS: $(OBJ_DIR)/%.dep

MKG := $(findstring $(MAKECMDGOALS),"clean distclean dump")
ifeq "$(MKG)" ""
  -include $(DEPS)
endif

