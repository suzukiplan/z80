CFLAGS=-I../ \
	-std=c++11 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wclass-varargs \
	-Wtype-limits \
	-Werror

all:
	make test-clock
	make test-status
	make test-im2
	make test-branch

test-clock:
	clang $(CFLAGS) test-clock.cpp -lstdc++
	./a.out

test-status:
	clang $(CFLAGS) test-status.cpp -lstdc++
	./a.out

test-im2:
	clang $(CFLAGS) test-im2.cpp -lstdc++
	./a.out > test-im2.txt
	cat test-im2.txt

test-branch:
	clang $(CFLAGS) test-branch.cpp -lstdc++
	./a.out > test-branch.txt
	cat test-branch.txt
