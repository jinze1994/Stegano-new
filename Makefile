all: main

main: jpeg.c jpeg.h habit.h main.c 
	g++ -O2 -Wall -Wextra -Wshadow main.c jpeg.c -ljpeg -o main

write:
	./main --write t1.jpg out.jpg password helloworld

clean:
	rm main
