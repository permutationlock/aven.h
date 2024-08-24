.POSIX:
.SUFFIXES:
.PHONY: all clean

all: build
build: build.c
	$(CC) $(CFLAGS) -o build build.c
clean:
	rm build

