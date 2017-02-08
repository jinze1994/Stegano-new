all: main

main: jpeg.c jpeg.h rgen.c rgen.h matrix.c matrix.h stegano.c stegano.h habit.h main.c
	gcc -O2 -Wall -Wextra -Wshadow jpeg.c rgen.c matrix.c stegano.c main.c -ljpeg -lcrypto -o main

write:
	./main --write t1.jpg out.jpg password helloworld

read:
	./main --read out.jpg password

clean:
	rm -f main tmp.jpg out.jpg
