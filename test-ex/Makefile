COMMON_FLAGS=-I../ \
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
	clang -std=c++17 $(COMMON_FLAGS) cpm.cpp -lstdc++ -o cpm

zexdoc: cpm
	./cpm -e zexdoc.cim

zexall: cpm
	./cpm -e zexall.cim

ci:
	g++-10 -std=c++2a $(COMMON_FLAGS) -Wclass-memaccess cpm.cpp -lstdc++ -o cpm
	./cpm -e -n zexall.cim

full: cpm
	./cpm zexall.cim

