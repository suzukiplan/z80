all:
	clang-format -style=file < z80.hpp > z80.hpp.bak
	cat z80.hpp.bak > z80.hpp
	rm z80.hpp.bak
	cd test && make

test-only:
	cd test && make

