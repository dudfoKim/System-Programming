#
# Makefile for the malloc lab driver
#
CC = gcc
CFLAGS = -Wall -O2 -g -DDRIVER

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

all: mdriver

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

naive:
	rm -f mm.c mm.o; ln -s mm-naive.c mm.c

implicit:
	rm -f mm.c mm.o; ln -s mm-implicit.c mm.c

explicit:
	rm -f mm.c mm.o; ln -s mm-explicit.c mm.c

seglist:
	rm -f mm.c mm.o; ln -s mm-seglist.c mm.c

clean:
	rm -f *~ *.o mdriver

