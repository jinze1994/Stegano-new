all: main

main: jpeg.cpp jpeg.h habit.h main.cpp 
	g++ -O2 -Wall -Wextra -Wshadow main.cpp jpeg.cpp -ljpeg -o main

write:
	./main --write t1.jpg out.jpg password helloworld

clean:
	rm main
