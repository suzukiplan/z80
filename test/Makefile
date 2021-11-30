all:
	make test-clock
	make test-status
	make test-clock-gb
	make test-im2
	make test-branch

test-clock:
	clang -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-clock.cpp -lstdc++
	./a.out

test-clock-gb:
	clang -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-clock-gb.cpp -lstdc++
	./a.out

test-status:
	clang -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-status.cpp -lstdc++
	./a.out

test-im2:
	clang -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-im2.cpp -lstdc++
	./a.out > test-im2.txt
	cat test-im2.txt

test-branch:
	clang -I../ -std=c++11 -Wall -Wfloat-equal -Wshadow -Werror test-branch.cpp -lstdc++
	./a.out > test-branch.txt
	cat test-branch.txt
