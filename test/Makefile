CFLAGS=-I../ \
	-std=c++11 \
	-Wall \
	-Wfloat-equal \
	-Wshadow \
	-Wunused-variable \
	-Wsign-conversion \
	-Wclass-varargs \
	-Wtype-limits \
	-Wsequence-point \
	-Wunsequenced \
	-Werror

all:
	make test-checkreg-on-callback
	make test-execute
	make test-clock
	make test-status
	make test-im2
	make test-branch
	make test-out
	make test-remove-break

test-execute:
	clang $(CFLAGS) test-execute.cpp -lstdc++
	./a.out > test-execute.txt
	cat test-execute.txt

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

test-out:
	clang $(CFLAGS) test-out.cpp -lstdc++
	./a.out > test-out.txt
	cat test-out.txt

test-remove-break:
	clang $(CFLAGS) test-remove-break.cpp -lstdc++
	./a.out > test-remove-break.txt
	cat test-remove-break.txt

test-checkreg-on-callback:
	clang $(CFLAGS) test-checkreg-on-callback.cpp -lstdc++
	./a.out > test-checkreg-on-callback.txt
	cat test-checkreg-on-callback.txt
