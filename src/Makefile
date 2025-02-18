cc = gcc

src = $(wildcard *.c)
target = exe.out


all:$(src)
	gcc $(src) -lSDL2 -lSDL2_ttf -o $(target) 

clean:
	-rm $(target)