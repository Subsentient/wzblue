CFLAGS = -std=gnu99 -pedantic -Wall -g -O0
OUTFILE = ../wzblue
OBJECTS = wz.c netcore.c main.c

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
endif

all:
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS)
clean:
	rm -f *.o *.gch
