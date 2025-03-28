CC = gcc
CFLAGS = -Wall -std=c11 -g -fPIC
LDFLAGS = -shared -L.
INC = include/
SRC = src/
BIN = bin/

all: parser

parser: $(BIN)libvcparser.so

$(BIN)libvcparser.so: VCParser.o LinkedListAPI.o VCHelper.o | $(BIN)
	$(CC) $(LDFLAGS) -o $@ VCParser.o LinkedListAPI.o VCHelper.o

VCParser.o: $(SRC)VCParser.c $(INC)VCParser.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)VCParser.c

LinkedListAPI.o: $(SRC)LinkedListAPI.c $(INC)LinkedListAPI.h
	$(CC) $(CFLAGS) -I$(INC) -c -fPIC $(SRC)LinkedListAPI.c

VCHelper.o: $(SRC)VCHelper.c $(INC)VCParser.h
	$(CC) $(CFLAGS) -I$(INC) -c $(SRC)VCHelper.c

clean:
	rm -rf *.o $(BIN)libvcparser.so