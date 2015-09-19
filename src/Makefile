GTKCHOICE = 2
PKGCONFIG = pkg-config
CFLAGS = -std=gnu99 -pedantic -Wall -g -O0 `$(PKGCONFIG) --cflags --libs gtk+-$(GTKCHOICE).0`

OUTFILE = ../wzblue
OBJECTS = wz.c netcore.c main.c gui.c

ifeq ($(OS),WIN)
	CFLAGS += -lcomctl32 -lws2_32 -DWIN -mwindows
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
endif

all:
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS)
clean:
	rm -f *.o *.gch
