all: cpm zexdoc zexall

clean:
	-rm cpm

cpm: cpm.cpp ../z80.hpp
	clang -Os -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror cpm.cpp -lstdc++ -o cpm

zexdoc: cpm
	./cpm -e zexdoc.cim

zexall: cpm
	./cpm -e zexall.cim

ci: cpm
	./cpm -e -n zexall.cim

full: cpm
	./cpm zexall.cim

