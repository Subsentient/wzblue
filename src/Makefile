GTKCHOICE = 2
PKGCONFIG = pkg-config
CFLAGS = -std=gnu11 -pedantic -Wall -g -O0 `$(PKGCONFIG) --cflags gtk+-$(GTKCHOICE).0`
LDFLAGS = `$(PKGCONFIG) --libs gtk+-$(GTKCHOICE).0` substrings/libsubstrings.a
OUTFILE = ../wzblue
OBJECTS = wz.o netcore.o main.o gui.o settings.o

ifdef TOOLCHAIN
	CC = $(TOOLCHAIN)-gcc
	CXX = $(TOOLCHAIN)-g++
	RANLIB = $(TOOLCHAIN)-ranlib
	WINDRES = $(TOOLCHAIN)-windres
	PKGCONFIG = $(TOOLCHAIN)-pkg-config
	STRIP = $(TOOLCHAIN)-strip
else
	CC = gcc
	CXX = g++
	RANLIB = ranlib
	WINDRES = windres
	PKGCONFIG = pkg-config
	STRIP = strip
endif

ifeq ($(OS),WIN)
	CFLAGS += -lws2_32 -mwindows -lcomctl32
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
endif
	
all: substrings_sub wzc netcorec mainc guic settingsc
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all WINDRES=$(WINDRES)
endif
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS) $(LDFLAGS)
	
substrings_sub:
	$(MAKE) -C substrings static CC=$(CC) RANLIB=$(RANLIB)
wzc:
	$(CC) wz.c -c $(CFLAGS)
netcorec:
	$(CC) netcore.c -c $(CFLAGS)
mainc:
	$(CC) main.c -c $(CFLAGS)
guic:
	$(CC) gui.c -c $(CFLAGS)
settingsc:
	$(CC) settings.c -c $(CFLAGS)



clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch

stripbin:
	@echo ""
	@echo Stripping binary to make it smaller...
	$(STRIP) --strip-debug $(OUTFILE)
	$(STRIP) --strip-unneeded $(OUTFILE)
