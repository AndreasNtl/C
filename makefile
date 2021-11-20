
all:main

main: main.c functions.c interfaces.h
	gcc -o main main.c functions.c -g3
	
clean:
	rm main

