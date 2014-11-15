#!/usr/bin/make -f

BIN:=parser
BIN_DEP:=main.c
BIN_OBJ:=$(BIN_DEP:%.c=%.o)

INSTALL_DIR:=./$(BIN)

CC=$(shell which gcc)
GDB=$(shell which gdb)
VALGRIND=$(shell which valgrind)
STRIP=$(shell which strip)

CFLAGS=-Wall -I/usr/include/gdal/ -g
LDFLAGS=-lgdal -lGeoIP

.PHONY: clean

all: $(BIN)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(LDFLAGS)

$(BIN): $(BIN_OBJ)
	$(CC) -o $(BIN) main.c $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(BIN) $(BIN_OBJ)
