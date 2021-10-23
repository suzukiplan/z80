all:
	clang-format -style=file < z80.hpp > z80.hpp.bak
	cat z80.hpp.bak > z80.hpp
	rm z80.hpp.bak
	make test-clock
	make test-status
	make test-clock-gb
	make test-im2

test-clock:
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-clock.cpp -lstdc++
	./a.out

test-clock-gb:
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-clock-gb.cpp -lstdc++
	./a.out

test-status:
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-status.cpp -lstdc++
	./a.out

test-im2:
	clang -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-im2.cpp -lstdc++
	./a.out

