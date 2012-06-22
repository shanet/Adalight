# Colorswirl
#
# Author: Shane Tully
#
# Source:      https://github.com/shanet/Adalight
# Forked from: https://github.com/adafruit/Adalight

CC = gcc
LANG = c

NAME = colorswirl
UPDATE_NAME = colorswirl_update
VERSION = "\"1.1.0\""

BINARY = $(NAME)
UPDATE_BINARY = $(UPDATE_NAME)
SRC = colorswirl.c usage.c
UPDATE_SRC = colorswirl_update.c usage.c
LIBS = -lm -lrt -pthread

MACROS = -DVERSION=$(VERSION) -DMQ_NAME="\"/$(NAME)\"" -D_GNU_SOURCE -DMAX_MSG_LEN=128
CFLAGS = -std=c99 -Wall -Wextra

.PHONY: all colorswirl colorswirl_update

all: colorswirl colorswirl_update

colorswirl:
	$(CC) $(CFLAGS) $(MACROS) -O2 $(SRC) -o $(BINARY) $(LIBS)

colorswirl_update:
	$(CC) $(CFLAGS) $(MACROS) -O2 $(UPDATE_SRC) -o $(UPDATE_BINARY) $(LIBS)

debug:
	$(CC) $(CFLAGS) $(MACROS) -ggdb $(SRC) -o $(BINARY) $(LIBS)
	$(CC) $(CFLAGS) $(MACROS) -ggdb $(UPDATE_SRC) -o $(UPDATE_BINARY) $(LIBS)

install:
	cp $(BINARY) /usr/sbin/
	cp $(UPDATE_BINARY) /usr/sbin

remove:
	rm -f /usr/sbin/$(BINARY)
	rm -f /usr/sbin/$(UPDATE_BINARY)

clean:
	rm -f $(BINARY) $(UPDATE_BINARY) *.o
