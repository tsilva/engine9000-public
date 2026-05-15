#
# COPYRIGHT © 2026 Enable Software Pty Ltd - All Rights Reserved
#
# https://github.com/alpine9000/engine9000-public
#
# See COPYING for license details
#

CC ?= cc
W64_CC ?= x86_64-w64-mingw32-gcc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2 -flto
BUILDDIR=build
EXE=$(BUILDDIR)/z80_srcmap
W64_EXE=$(BUILDDIR)/z80_srcmap.exe
SYSTEM_DIR=../../e9k-debugger/system
Z80_SRCMAP_TEST_DIR=tests
Z80_SRCMAP_TEST_EXE=$(EXE)

.PHONY: all w64 test clean

all: $(EXE)
	cp $(EXE) $(SYSTEM_DIR)/z80_srcmap

$(EXE): main.c list.c list.h | $(BUILDDIR)
	$(CC) $(CFLAGS) main.c list.c -o $@

w64: $(W64_EXE)
	cp $(W64_EXE) $(SYSTEM_DIR)/z80_srcmap.exe

$(W64_EXE): main.c list.c list.h | $(BUILDDIR)
	$(W64_CC) $(CFLAGS) main.c list.c -o $@

include tests/test.mk

test: $(EXE) $(Z80_SRCMAP_TESTS)

$(BUILDDIR):
	mkdir -p $@

clean: clean-z80srcmap-tests
	rm -rf $(BUILDDIR)
	rm -f z80_srcmap z80_srcmap.exe
