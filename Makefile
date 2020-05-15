CC?=gcc
CFLAGS=-Wall -std=c99 -g
LFLAGS=
TARGET=libarchive.a
AR=ar

all: $(TARGET) mytar

archive.o: archive.h archive.c
	$(CC) $(CFLAGS) -c archive.c

$(TARGET): archive.o
	$(AR) -r $(TARGET) archive.o

mytar: $(TARGET) mytar.c
	$(CC) $(CFLAGS) mytar.c -o mytar -larchive -L.

clean:
	rm -f archive.o $(TARGET) ./mytar