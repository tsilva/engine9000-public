AMIGA_GCC=/opt/amiga/bin/m68k-amigaos-gcc
CFLAGS=-fomit-frame-pointer -noixemul -Wall -Werror -O2

all: load9000
	cp load9000 ../../../e9k-debugger/system
	mkdir -p ../../../e9k-debugger/docs/vasm-example/C
	cp load9000 ../../../e9k-debugger/docs/vasm-example/C
	mkdir -p ../../../e9k-debugger/docs/vasm-example/DH0/C
	cp load9000 ../../../e9k-debugger/docs/vasm-example/DH0/C

load9000: main.c Makefile
	$(AMIGA_GCC) $(CFLAGS) $< -o $@ -lamiga -s

clean:
	rm -f load9000 ../../../e9k-debugger/system/load9000
