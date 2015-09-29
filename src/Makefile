GTKCHOICE = 2
PKGCONFIG = pkg-config
CFLAGS = -std=gnu99 -pedantic -Wall -g -O0 `$(PKGCONFIG) --cflags gtk+-$(GTKCHOICE).0`
LDFLAGS = `$(PKGCONFIG) --libs gtk+-$(GTKCHOICE).0` substrings/libsubstrings.a
OUTFILE = ../wzblue
OBJECTS = wz.o netcore.o main.o gui.o settings.o
STRIP = strip

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -DWIN -mwindows -lcomctl32
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
	STRIP = i686-pc-mingw32-strip
endif
	
all:
	$(MAKE) -C substrings static
	$(CC) wz.c -c $(CFLAGS)
	$(CC) netcore.c -c $(CFLAGS)
	$(CC) main.c -c $(CFLAGS)
	$(CC) gui.c -c $(CFLAGS)
	$(CC) settings.c -c $(CFLAGS)
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS) $(LDFLAGS)

clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch

stripbin:
	@echo ""
	@echo Stripping binary to make it smaller...
	$(STRIP) --strip-debug $(OUTFILE)
	$(STRIP) --strip-unneeded $(OUTFILE)
