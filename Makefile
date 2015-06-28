# Colorswirl
#
# Author: Shane Tully
#
# Source:      https://github.com/shanet/Adalight
# Forked from: https://github.com/adafruit/Adalight

CC = gcc

NAME        = colorswirl
UPDATE_NAME = colorswirl_update
VERSION     = "\"2.0.0\""

BINARY         = $(NAME)
UPDATE_BINARY  = $(UPDATE_NAME)
INSTALL_DIR    = /usr/sbin/local
SYSTEMD_SCRIPT = colorswirl.service
SRC            = src/colorswirl.c src/usage.c
UPDATE_SRC     = src/colorswirl_update.c src/usage.c
LIBS           = -lm -lrt -pthread -lX11

MACROS = -DVERSION=$(VERSION) -DMQ_NAME="\"/$(NAME)\"" -D_GNU_SOURCE -DMAX_MSG_LEN=128
CFLAGS = -std=c99 -Wall -Wextra

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CFLAGS += -ggdb
else
	CFLAGS += -O2
endif

.PHONY: all colorswirl colorswirl_update

all: colorswirl colorswirl_update

colorswirl:
	$(CC) $(CFLAGS) $(MACROS) $(SRC) -o bin/$(BINARY) $(LIBS)

colorswirl_update:
	$(CC) $(CFLAGS) $(MACROS) $(UPDATE_SRC) -o bin/$(UPDATE_BINARY) $(LIBS)

install:
	mkdir -p $(INSTALL_DIR)
	cp bin/$(BINARY) $(INSTALL_DIR)/
	cp bin/$(UPDATE_BINARY) $(INSTALL_DIR)
	cp src/$(SYSTEMD_SCRIPT) /etc/systemd/system/

remove:
	rm -f $(INSTALL_DIR)/$(BINARY)
	rm -f $(INSTALL_DIR)/$(UPDATE_BINARY)

clean:
	rm -f bin/$(BINARY) bin/$(UPDATE_BINARY) src/*.o
