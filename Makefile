CFLAGS = -std=gnu89 -pedantic -Wall -g -O0
OUTFILE = wzblue

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN
	OUTFILE = wzblue.exe
endif

all:
	$(CC) lobbycomm.c netcore.c main.c -o $(OUTFILE) $(CFLAGS)
clean:
	rm -f wzblue wzblue.exe
