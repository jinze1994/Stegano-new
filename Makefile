all: main

main: jpeg.cpp jpeg.h stegano.cpp stegano.h habit.h main.cpp
	gcc -O2 -Wall -Wextra -Wshadow jpeg.cpp stegano.cpp main.cpp -ljpeg -o main

write:
	./main --write t1.jpg out.jpg password helloworld

clean:
	rm -f main tmp.jpg out.jpg
