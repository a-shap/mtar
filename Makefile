CC?=gcc
CFLAGS=-Wall -std=c99 -g

.PHONY: build clean test

all: build

build:
	$(CC) $(CFLAGS) tar.c archive.c mytar.c -o mytar

test:
	mkdir -p test
	./mytar c test/test.tar mytar.c
	cd test; ../mytar x test.tar; cmp --silent mytar.c ../mytar.c || (echo "files are different" & exit 1)

clean:
	rm -f ./mytar
	rm -rf test