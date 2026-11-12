CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c89 -pedantic
DOSFLAGS = -0 -Md -ansi -O

.PHONY: all test clean dos dos-test dist experiment port-model-test pulse-test fastaddr-test original-2716-test

all: test

build/test_virtual: src/willem.c tests/test_virtual.c include/willem.h
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/willem.c tests/test_virtual.c -o $@

test: build/test_virtual
	./build/test_virtual
	bash tests/test_validate_read.sh
	bash tests/test_m2764a_gate.sh
	python3 tests/test_trace.py

# The DOS front end is added after the portable protocol core passes its
# virtual-board tests. The link-time -i selects bcc's one-segment tiny model.
build/dos/dosio.o: src/dosio.asm
	mkdir -p build/dos
	nasm -f as86 $< -o $@

build/dos/fastaddr.o: src/fastaddr.asm
	mkdir -p build/dos
	nasm -f as86 $< -o $@

build/dos/willem.o: src/willem.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/dosmain.o: src/dosmain.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/WILLEM.COM: build/dos/dosmain.o build/dos/willem.o build/dos/dosio.o build/dos/fastaddr.o
	bcc -0 -Md -i -o $@ $^

dos: build/dos/WILLEM.COM
	file build/dos/WILLEM.COM

dos-test: dos
	bash tools/test_dosbox.sh
	python3 tests/test_2716_diag.py
	python3 tests/test_2716_write.py
	python3 tests/test_2716_dos_core.py
	python3 tests/test_fastaddr_guard.py

dist: dos
	rm -rf build/dist
	bash tools/make_dist.sh build/dos/WILLEM.COM dos build/dist

clean:
	rm -rf build

# Optional Unicorn dependency; does not access programmer hardware.
pulse-test:
	python3 tests/test_2716_pulse.py

fastaddr-test:
	python3 tests/test_fastaddr.py

# Explicitly authorized sacrificial-chip diagnostics; never run by make test.
build/dos/rf2_experiment.o: tools/rf2_experiment.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/RF2EXP.COM: build/dos/rf2_experiment.o build/dos/willem.o build/dos/dosio.o build/dos/fastaddr.o
	bcc -0 -Md -i -o $@ $^

experiment: build/dos/RF2EXP.COM

port-model-test: dos experiment
	python3 tests/test_dos_port_model.py
	python3 tests/test_rf2_experiment.py
	python3 tests/test_rf2_probe.py
	python3 tests/test_rf2_fill.py

# No LPT accesses: exercises the exact 2716 timer path for host-clock checks.
build/dos/timer2_probe.o: tools/timer2_probe.c
	mkdir -p build/dos
	bcc $(DOSFLAGS) -c $< -o $@

build/dos/T2PROBE.COM: build/dos/timer2_probe.o build/dos/dosio.o
	bcc -0 -Md -i -o $@ $^

# Bounded diagnostic that follows original Willem DOS 2716 port sequences.
build/dos/original_2716_lab.o: tools/original_2716_lab.c
	mkdir -p build/dos
	bcc $(DOSFLAGS) -c $< -o $@

build/dos/ORG2716.COM: build/dos/original_2716_lab.o build/dos/dosio.o
	bcc -0 -Md -i -o $@ $^

original-2716-test: dos experiment build/dos/ORG2716.COM
	python3 tests/test_original_2716_lab.py
	python3 tests/test_original_2716_fill.py
	python3 tools/check_original_2716_trace.py programming/serial-boot-20261110/evidence/original-2716/pcb3-clean.log.gz programming/serial-boot-20261110/evidence/original-2716/PATTERN.BIN
	python3 tools/check_original_2716_trace.py programming/serial-boot-20261110/evidence/original-2716/plain-program.log.gz programming/serial-boot-20261110/evidence/original-2716/PATTERN.BIN

build/dos/program_2732a.o: tools/program_2732a.c include/willem.h
	mkdir -p build/dos
	bcc $(DOSFLAGS) -Iinclude -c $< -o $@

build/dos/PG2732A.COM: build/dos/program_2732a.o build/dos/willem.o build/dos/dosio.o
	bcc -0 -Md -i -o $@ $^

.PHONY: 2732a-test
2732a-test: dos build/dos/PG2732A.COM
	python3 tests/test_2732a.py
