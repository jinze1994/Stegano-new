all: naive

main: jpeg.c jpeg.h rgen.c rgen.h matrix.c matrix.h mlbc.c mlbc.h stegano.c stegano.h habit.h main.c
	gcc -O2 -Wall -Wextra -Wshadow jpeg.c rgen.c matrix.c mlbc.c stegano.c main.c -ljpeg -lcrypto -o main

naive: jpeg.c jpeg.h rgen.c rgen.h matrix.c matrix.h dup.c dup.h stegano.c stegano.h habit.h main.c
	gcc -O2 -Wall -Wextra -Wshadow jpeg.c rgen.c matrix.c dup.c stegano.c main.c -ljpeg -lcrypto -o naive

write:
	./naive --write t1.jpg out.jpg password helloworld!

read:
	./naive --read out.jpg password

valid:
	valgrind ./naive --write t1.jpg out.jpg password helloworld!
	valgrind ./naive --read out.jpg password

test:
	gcc test.c matrix.c

recode:
	./main --recode t1.jpg tmp.jpg

clean:
	rm -f a.out main naive tmp.jpg out.jpg
