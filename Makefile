all:
	clang-format -style=file < z80.hpp > z80.hpp.bak
	cat z80.hpp.bak > z80.hpp
	rm z80.hpp.bak
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test.cpp -lstdc++
	./a.out
