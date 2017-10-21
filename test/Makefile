pthreadpool.o: ../src/pthreadpool.c
	gcc -c ../src/pthreadpool.c -o pthreadpool.o

test.o: test.c
	gcc -c test.c -o test.o

test: test.o pthreadpool.o
	gcc -o test test.o pthreadpool.o

clean:
	rm -rf pthreadpool.o test.o test