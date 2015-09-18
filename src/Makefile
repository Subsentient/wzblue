GTKCHOICE = 2
CFLAGS = -std=gnu99 -pedantic -Wall -g -O0 `pkg-config --cflags --libs gtk+-$(GTKCHOICE).0`
OUTFILE = ../wzblue
OBJECTS = wz.c netcore.c main.c gui.c

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
endif

all:
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS)
clean:
	rm -f *.o *.gch
