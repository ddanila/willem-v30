CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c89 -pedantic
DOSFLAGS = -0 -Md -i -ansi -O

.PHONY: all test clean dos

all: test

build/test_virtual: src/willem.c tests/test_virtual.c include/willem.h
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/willem.c tests/test_virtual.c -o $@

test: build/test_virtual
	./build/test_virtual

# The DOS front end is added after the portable protocol core passes its
# virtual-board tests. bcc -0 -Md emits an 8086 small-model DOS .COM file.
build/dos/dosio.o: src/dosio.asm
	mkdir -p build/dos
	nasm -f as86 $< -o $@

build/dos/willem.o: src/willem.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/dosmain.o: src/dosmain.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/WILLEM.COM: build/dos/dosmain.o build/dos/willem.o build/dos/dosio.o
	bcc -0 -Md -i -o $@ $^

dos: build/dos/WILLEM.COM
	file build/dos/WILLEM.COM

clean:
	rm -rf build
