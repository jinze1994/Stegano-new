all: main naive

main: jpeg.c jpeg.h rgen.c rgen.h matrix.c matrix.h mlbc.c mlbc.h stegano.c stegano.h habit.h main.c
	gcc -O2 -Wall -Wextra -Wshadow jpeg.c rgen.c matrix.c mlbc.c stegano.c main.c -ljpeg -lcrypto -o main

naive: jpeg.c jpeg.h rgen.c rgen.h matrix.c matrix.h dup.c dup.h stegano.c stegano.h habit.h main.c
	gcc -O2 -Wall -Wextra -Wshadow jpeg.c rgen.c matrix.c dup.c stegano.c main.c -ljpeg -lcrypto -o naive

write:
	./main --write t1.jpg out.jpg password helloworld!

read:
	./main --read out.jpg password

valid:
	valgrind ./main --write t1.jpg out.jpg password helloworld!
	valgrind ./main --read out.jpg password

test:
	gcc test.c matrix.c

recode:
	./main --recode t1.jpg tmp.jpg

clean:
	rm -f a.out main naive tmp.jpg out.jpg
