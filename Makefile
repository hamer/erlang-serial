PROJ = serial

.PHONY: all
all: $(PROJ).beam $(PROJ) $(PROJ).exe

$(PROJ).beam: $(PROJ).erl
	erlc $(PROJ).erl

$(PROJ): $(wildcard src/*)
	-make -C src PROG=../serial OBJDIR=../build-posix
	-strip serial

$(PROJ).exe: $(wildcard src/*)
	-make -C src PROG=../serial.exe OBJDIR=../build-winapi CC=i686-w64-mingw32-gcc
	-i686-w64-mingw32-strip serial.exe

.PHONY: clean
clean:
	-make -C src PROG=../serial OBJDIR=../build-posix clean
	-make -C src PROG=../serial.exe OBJDIR=../build-winapi CC=i686-w64-mingw32-gcc clean
	-rm $(PROJ).beam
