# -*- coding: utf-8 -*-
# Copyright © 2016, Swanson Project.
# Please see the COPYING file in this distribution for license details.

.PHONY: clean depends distclean libswanson swanson
all: depends libswanson swanson

LN = ln
SED = sed
ifeq ($(origin VALGRIND), undefined)
    VALGRIND := $(shell sh -c 'command -v valgrind')
endif

#------------------------------------------------------------------------------
# Out-of-tree builds
#
# To perform an out-of-tree build, just invoke make from some other directory:
#
#     $ cd ~/somewhere/else
#     $ make -f path/to/this/Makefile
#
# Build results will be placed into $PWD/.build by default; override the
# BUILD_ROOT variable if you want more control than this.

# In all of the rules below, we must preface every source file with
# $(SOURCE_ROOT), and every build result with $(BUILD_ROOT).
BUILD_ROOT=$(CURDIR)/.build
SOURCE_ROOT=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# A special rule that ensures that the build output directory exists.
BUILD_STAMP = $(BUILD_ROOT)/.stamp
$(BUILD_STAMP):
	@mkdir -p $(BUILD_ROOT)
	@touch $@

# Every build rule should depend on this target.  This ensures that the build
# output directory is created before we build anything, and forces make to
# rebuild everything if the Makefile changes.  (Note that this does NOT include
# Makefile.deps; doing so would cause every target to be rebuilt if ANY
# dependencies change.)
BUILD = $(BUILD_STAMP) $(SOURCE_ROOT)/Makefile

#------------------------------------------------------------------------------
# Source files

# Update the variables below according to the following rules (taken from [1]):
#
# VERSION = SOVERSION.REVISION.AGE
#
#   1. Start with a VERSION of `0.0.0` for each shared library.
#   2. Update VERSION only immediately before a public release of your software.
#      More frequent updates are unnecessary, and only guarantee that the
#      current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then
#      increment REVISION (`c.r.a` becomes `c.r+1.a`).
#   4. If any interfaces have been added, removed, or changed since the last
#      update, increment SOVERSION, and set REVISION to 0.
#   5. If any interfaces have been added since the last public release, then
#      increment AGE.
#   6. If any interfaces have been removed or changed since the last public
#      release, then set AGE to 0.
#
# Note that changing SOVERSION means that you are releasing a new
# backwards-incompatible version of the library.  This has implications on
# packaging, so once an API has stabilized, this should be a rare occurrence.
#
# [1] http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info

LIBSWANSON_SOVERSION = 0
LIBSWANSON_REVISION = 0
LIBSWANSON_AGE = 0
LIBSWANSON_VERSION = $(LIBSWANSON_SOVERSION).$(LIBSWANSON_REVISION).$(LIBSWANSON_AGE)

LIBSWANSON_H = \
    $(SOURCE_ROOT)/include/swanson.h
LIBSWANSON_C = \
    $(SOURCE_ROOT)/libswanson/s0.c
LIBSWANSON_O = $(LIBSWANSON_C:$(SOURCE_ROOT)/%.c=$(BUILD_ROOT)/objs/%.o)
LIBSWANSON_D = $(LIBSWANSON_O:.o=.d)

SWANSON_C = \
    $(SOURCE_ROOT)/swanson/swanson.c
SWANSON_O = $(SWANSON_C:$(SOURCE_ROOT)/%.c=$(BUILD_ROOT)/objs/%.o)
SWANSON_D = $(SWANSON_O:.o=.d)

TEST_SWANSON_C = \
    $(SOURCE_ROOT)/ccan/tap/tap.c \
    $(SOURCE_ROOT)/test-swanson/test-swanson.c
TEST_SWANSON_O = $(TEST_SWANSON_C:$(SOURCE_ROOT)/%.c=$(BUILD_ROOT)/objs/%.o)
TEST_SWANSON_D = $(TEST_SWANSON_O:.o=.d)

#------------------------------------------------------------------------------
# Compiling

# You can override CFLAGS if you want to control things like optimizations,
# warnings, and debug symbols.  We provide a suitable default if you don't care.
CFLAGS = -g -O3 -Wall -Werror

# Whatever we need to pass in to the C compiler to have it produce something
# that can be included in a shared library.
SHARED_CFLAGS = -fPIC

# If your compiler supports it, these options tell it to output make-style
# dependency information in .d that live alongside the .o files that it
# produces.
DEPENDENCY_CFLAGS = -MMD
include $(SOURCE_ROOT)/Makefile.deps

# You probably shouldn't override this part; this is how we tell the compiler
# where all of our include files live.  If $(SOURCE_ROOT) has been set
# correctly, then you don't need to touch this.
INCLUDE_CPPFLAGS = -I$(SOURCE_ROOT)/include -I$(SOURCE_ROOT)

# A pattern rule for compiling .c files into .o files.
$(BUILD_ROOT)/objs/%.o: $(SOURCE_ROOT)/%.c $(BUILD)
	@echo "CC   $(patsubst $(SOURCE_ROOT)/%, %, $<)"
	@mkdir -p $(dir $@)
	@$(CC) -c \
	    $(CFLAGS) \
	    $(SHARED_CFLAGS) \
	    $(DEPENDENCY_CFLAGS) \
	    $(INCLUDE_CPPFLAGS) \
	    $< -o $@

#------------------------------------------------------------------------------
# Dependency post-processing

depends: $(LIBSWANSON_O) $(SWANSON_O) $(TEST_SWANSON_O)
	@echo "DEPS  Makefile.deps"
	@$(SED) \
	    -e 's+'"$(BUILD_ROOT)"'+$$(BUILD_ROOT)+g' \
	    -e 's+'"$(SOURCE_ROOT)"'+$$(SOURCE_ROOT)+g' \
	    $(^:.o=.d) \
	    > Makefile.deps

#------------------------------------------------------------------------------
# Linking

# Whatever we need to pass in to the linker to have it produce a shared library.
SHARED_LDFLAGS = -shared

# You probably shouldn't override this part; this is how we tell the linker
# where all of our shared libraries live.  If $(BUILD_ROOT) has been set
# correctly, then you don't need to touch this.
INCLUDE_LDFLAGS = -L$(BUILD_ROOT)

LIBSWANSON_SO = $(BUILD_ROOT)/libswanson.so
LIBSWANSON_SO_X = $(LIBSWANSON_SO).$(LIBSWANSON_SOVERSION)
LIBSWANSON_VERSION_LDFLAGS = -soname libswanson.so.$(LIBSWANSON_SOVERSION)

libswanson: $(LIBSWANSON_SO) $(LIBSWANSON_SO_X) $(LIBSWANSON_SO_XYZ)
$(LIBSWANSON_SO): $(LIBSWANSON_O) $(BUILD)
	@echo "LD   $(patsubst $(BUILD_ROOT)/%, %, $@)"
	@$(LD) \
	    $(SHARED_LDFLAGS) \
	    $(LIBSWANSON_VERSION_LDFLAGS) \
	    -o $@ $(filter %.o, $^)
$(LIBSWANSON_SO_X): $(LIBSWANSON_SO)
	@$(LN) -sf $(notdir $<) $@

SWANSON_EXE = $(BUILD_ROOT)/swanson

swanson: $(SWANSON_EXE)
$(SWANSON_EXE): $(SWANSON_O) $(LIBSWANSON_SO) $(BUILD)
	@echo "LD   $(patsubst $(BUILD_ROOT)/%, %, $@)"
	@$(CC) \
	    $(INCLUDE_LDFLAGS) \
	    -o $@ $(filter %.o, $^) \
	    -Wl,-rpath,$(BUILD_ROOT) \
	    -lswanson

#------------------------------------------------------------------------------
# Tests

TEST_SWANSON_EXE = $(BUILD_ROOT)/test-swanson

test-swanson: $(TEST_SWANSON_EXE) $(TEST_SWANSON_WRAPPER)
$(TEST_SWANSON_EXE): $(TEST_SWANSON_O) $(LIBSWANSON_SO_X) $(BUILD)
	@echo "LD   $(patsubst $(BUILD_ROOT)/%, %, $@)"
	@mkdir -p $(dir $@)
	@$(CC) \
	    $(INCLUDE_LDFLAGS) \
	    -o $@ $(filter %.o, $^) \
	    -Wl,-rpath,$(BUILD_ROOT) \
	    -lswanson

check: $(TEST_SWANSON_EXE)
ifdef VALGRIND
	@echo "TEST (valgrind)"
	@$(VALGRIND) \
	    --error-exitcode=1 \
	    --leak-check=full \
	    --log-file=$(BUILD_ROOT)/valgrind.out \
	    $(TEST_SWANSON_EXE)
else
	@echo "TEST"
	@$(TEST_SWANSON_EXE)
endif

#------------------------------------------------------------------------------
# Cleaning up

clean:
	@rm -f $(LIBSWANSON_D)
	@rm -f $(LIBSWANSON_O)
	@rm -f $(LIBSWANSON_SO)
	@rm -f $(LIBSWANSON_SO_X)
	@rm -f $(SWANSON_D)
	@rm -f $(SWANSON_O)
	@rm -f $(SWANSON_EXE)
	@rm -f $(TEST_SWANSON_D)
	@rm -f $(TEST_SWANSON_O)
	@rm -f $(TEST_SWANSON_EXE)

distclean:
	@rm -rf $(BUILD_ROOT)
