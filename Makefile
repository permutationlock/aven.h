.POSIX:
.SUFFIXES:
.PHONY: all clean

all: build
build: build.c
	$(CC) -o build build.c
clean:
	rm build

