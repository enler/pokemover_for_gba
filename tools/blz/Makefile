CC ?= gcc
.PHONY: all clean

SRCS = blz.c

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

all: blz$(EXE)
	@:

blz$(EXE): $(SRCS)
	$(CC) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) blz blz.exe
