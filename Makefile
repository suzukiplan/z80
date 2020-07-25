all:
	clang-format -style=file < z80.hpp > z80.hpp.bak
	cat z80.hpp.bak > z80.hpp
	rm z80.hpp.bak
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-clock.cpp -lstdc++
	./a.out
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-status.cpp -lstdc++
	./a.out
