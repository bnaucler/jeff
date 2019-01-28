CC = cc
TARGET = jeff
SOURCE = *.c
DESTDIR = /usr/bin
CFLAGS ?= -Wall -pedantic -g -lncurses

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(DESTDIR)
