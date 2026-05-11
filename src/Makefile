GTKCHOICE = 3
PKGCONFIG = pkg-config
CFLAGS = -std=gnu11 -pedantic -Wall -g -O0 `$(PKGCONFIG) --cflags libcurl` `$(PKGCONFIG) --cflags gtk+-$(GTKCHOICE).0` -Wno-deprecated-declarations -D_FORTIFY_SOURCE=0 -fstack-protector
LDFLAGS = `$(PKGCONFIG) --libs gtk+-$(GTKCHOICE).0` `$(PKGCONFIG) --libs libcurl` substrings/libsubstrings.a -fstack-protector
OUTFILE = ../wzblue
OBJECTS = wz.o curl.o main.o gui.o settings.o cJSON.o

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
	OUTFILE = ../wzblue.exe
	OBJECTS += ../win32/win.o
	LDFLAGS += -mwindows -lstdc++ -lws2_32 -lcomctl32
endif
	
all: substrings_sub wzc mainc guic settingsc curlc cJSONc
ifeq ($(OS),WIN)
	$(MAKE) -C ../win32 all WINDRES=$(WINDRES)
endif
	$(CC) $(OBJECTS) -o $(OUTFILE) $(CFLAGS) $(LDFLAGS)
	
substrings_sub:
	$(MAKE) -C substrings static CC=$(CC) RANLIB=$(RANLIB)
wzc:
	$(CC) wz.c -c $(CFLAGS)
cJSONc:
	$(CC) cJSON/cJSON.c -c $(CFLAGS)
mainc:
	$(CC) main.c -c $(CFLAGS)
guic:
	$(CC) gui.c -c $(CFLAGS)
settingsc:
	$(CC) settings.c -c $(CFLAGS)

curlc:
	$(CC) curl.c -c $(CFLAGS)


clean:
	$(MAKE) -C substrings clean
	rm -f *.o *.gch

stripbin:
	@echo ""
	@echo Stripping binary to make it smaller...
	$(STRIP) --strip-debug $(OUTFILE)
	$(STRIP) --strip-unneeded $(OUTFILE)
