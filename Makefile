.POSIX:
.SUFFIXES:
.PHONY: all clean

all: build
build: build.c aven/arena.c
	$(CC) $(CFLAGS) -Iinclude -o build build.c aven/arena.c
clean:
	rm build

