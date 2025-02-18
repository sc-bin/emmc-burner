cc = gcc

src = $(wildcard *.c)
target = progress_bar


all:$(src)
	gcc $(src) -o $(target) -lfreetype -I/usr/include/freetype2

install:
	cp DejaVuSans.ttf /usr/share/fonts/
	cp $(target) /usr/bin/
	chmod +x /usr/bin/$(target)

uninstall:
	rm /usr/share/fonts/DejaVuSans.ttf
	rm /usr/bin/$(target)

clean:
	-rm $(target)