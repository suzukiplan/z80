CFLAGS=-I../ \
	-std=c++11 \
	-O2 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wtype-limits \
	-Werror

all: cpm zexdoc zexall

clean:
	-rm cpm

cpm: cpm.cpp ../z80.hpp
	clang $(CFLAGS) cpm.cpp -lstdc++ -o cpm

zexdoc: cpm
	./cpm -e zexdoc.cim

zexall: cpm
	./cpm -e zexall.cim

ci:
	g++-10 $(CFLAGS) -Wclass-memaccess cpm.cpp -lstdc++ -o cpm
	./cpm -e -n zexall.cim

full: cpm
	./cpm zexall.cim

