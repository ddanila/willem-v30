CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c89 -pedantic
DOSFLAGS = -0 -Md -ansi -O

.PHONY: all test clean dos dos-test dist

all: test

build/test_virtual: src/willem.c tests/test_virtual.c include/willem.h
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/willem.c tests/test_virtual.c -o $@

test: build/test_virtual
	./build/test_virtual
	bash tests/test_validate_read.sh

# The DOS front end is added after the portable protocol core passes its
# virtual-board tests. The link-time -i selects bcc's one-segment tiny model.
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

dos-test: dos
	bash tools/test_dosbox.sh

dist: dos
	rm -rf build/dist
	bash tools/make_dist.sh build/dos/WILLEM.COM dos build/dist

clean:
	rm -rf build
