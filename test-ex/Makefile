COMMON_FLAGS=-I../ \
	-O2 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wtype-limits \
	-Werror \
	-DZ80_DISABLE_DEBUG \
	-DZ80_DISABLE_NESTCHECK \
	-DZ80_UNSUPPORT_16BIT_PORT \
	-DZ80_CALLBACK_PER_INSTRUCTION

all: cpm zexdoc zexall

clean:
	-rm cpm

cpm: cpm.cpp ../z80.hpp
	clang -std=c++17 $(COMMON_FLAGS) cpm.cpp -lstdc++ -o cpm

zexdoc: cpm
	./cpm -e zexdoc.cim

zexall: cpm
	./cpm -e zexall.cim

ci:
	@echo Test zexall with gcc
	g++-10 -std=c++2a $(COMMON_FLAGS) -Wclass-memaccess cpm.cpp -lstdc++ -o cpm
	./cpm -e -n zexall.cim
	@echo Test zexdoc with clang
	clang -std=c++17 $(COMMON_FLAGS) cpm.cpp -lstdc++ -o cpm
	./cpm -e -n zexdoc.cim

full: cpm
	./cpm zexall.cim

