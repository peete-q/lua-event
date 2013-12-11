
CC=gcc
CFLAGS = -g -Wall -fPIC -Iinclude -I../libevent-2.0.21-stable/include -I../include
LFLAGS = -lmingw32 -shared -L../libevent-2.0.21-stable/.libs -L../lib -llua51 -levent -lws2_32

T = core.dll
SRC = src/*.c
OBJ = *.o

all:
	$(CC) -c $(SRC) $(CFLAGS)
	$(CC) -o $(T) $(OBJ) $(LFLAGS)

clean:
	rm -f $(T)
	rm -f $(OBJ)

