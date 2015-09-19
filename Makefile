
all:
ifeq ($(OS),WIN)
	$(MAKE) -C win32
endif
	$(MAKE) -C src
clean:
	$(MAKE) -C src clean
	$(MAKE) -C win32 clean
	rm -f wzblue wzblue.exe

stripbin:
	$(MAKE) -C src stripbin
