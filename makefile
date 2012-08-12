C=gcc --std=c11 -Wall
OPTIMISE=-O3
CFLAGS=-c `pkg-config --cflags libxml-2.0` -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=29
LDFLAGS=-o
LIBS=`pkg-config --libs libxml-2.0` -ljansson -lrtmp -lfuse
OBJECTS=auntie.o
TARGET=auntie

all:	auntie

again:	clean all

auntie:	auntie.o
	$(C) $(OPTIMISE) $(LIBS) $(LDFLAGS) auntie auntie.o

auntie.o:	auntie.c
	$(C) $(OPTIMISE) $(CFLAGS) auntie.c

auntie.c:	auntie.h

clean:
	rm -f $(TARGET) $(OBJECTS)
