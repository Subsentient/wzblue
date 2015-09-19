GTKCHOICE = 2
PKGCONFIG = pkg-config
CFLAGS = -std=gnu99 -pedantic -Wall -g -O0 `$(PKGCONFIG) --cflags --libs gtk+-$(GTKCHOICE).0`
OUTFILE = ../wzblue
OBJECTS = wz.c netcore.c main.c gui.c
STRIP = strip

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN -mwindows
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
	STRIP = i686-pc-mingw32-strip
endif
	
all:
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS)

clean:
	rm -f *.o *.gch

stripbin:
	@echo ""
	@echo Stripping binary to make it smaller...
	$(STRIP) --strip-debug $(OUTFILE)
	$(STRIP) --strip-unneeded $(OUTFILE)
