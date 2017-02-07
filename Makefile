all: main

main: jpeg.cpp jpeg.h mlbc.cpp mlbc.h habit.h main.cpp
	g++ -O2 -Wall -Wextra -Wshadow jpeg.cpp mlbc.cpp main.cpp -ljpeg -o main

write:
	./main --write t1.jpg out.jpg password helloworld

clean:
	rm -f main tmp.jpg out.jpg
