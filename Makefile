all:
	gcc -Wall main.c `pkg-config fuse3 --cflags --libs` -o test

run:
	./test -d -s -o allow_other -o default_permissions /tmp/dir02

clean:
	rm test