C=gcc --std=c11 -Wall
OPTIMISE=-O3
CFLAGS=-c `pkg-config --cflags libxml-2.0` -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=29 -DDEBUG
LDFLAGS=-o
LIBS=`pkg-config --libs libxml-2.0` -ljansson -lrtmp -lfuse
OBJECTS=main.o auntie.o auntiefs.o
TARGET=auntie

all:	auntie

again:	clean all

auntie:	main.o auntie.o auntiefs.o
	$(C) $(OPTIMISE) $(LIBS) $(LDFLAGS) auntie main.o auntie.o auntiefs.o

main.o:	main.c
	$(C) $(OPTIMISE) $(CFLAGS) main.c

auntie.o:	auntie.c
	$(C) $(OPTIMISE) $(CFLAGS) auntie.c

auntiefs.o:	auntiefs.c
	$(C) $(OPTIMISE) $(CFLAGS) auntiefs.c

main.c:	auntiefs.h
auntie.c:	auntie.h http.h
auntiefs.c:	auntiefs.h

clean:
	rm -f $(TARGET) $(OBJECTS)
