all: cpm

clean:
	-rm cpm

cpm: cpm.cpp ../z80.hpp
	clang -Os -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror cpm.cpp -lstdc++ -o cpm

zexdoc: cpm
	./cpm zexdoc.cim

zexall: cpm
	./cpm zexall.cim
