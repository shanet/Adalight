CC = gcc
LANG = c

NAME = colorswirl
VERSION = 1.0.0

BINARY = $(NAME)
SRC = colorswirl.c
LIBS = -lm

CFLAGS = -std=c99 -Wall -Wextra -DVERSION=$(VERSION)

all:
	$(CC) $(CFLAGS) -O2 $(SRC) -o $(BINARY) $(LIBS)

debug:
	$(CC) $(CFLAGS) -ggdb $(SRC) -o $(BINARY) $(LIBS)

clean:
	rm -f $(BINARY) *.o
