CFLAGS=-I/opt/local/include --std=c99 -Wall -Wextra
LDFLAGS=-L/opt/local/lib
LDLIBS=-lportaudio -lm

.PHONY: all clean

all: vu-meter

vu-meter: main.o vu-meter.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

main.o: main.c vu-meter.h
vu-meter.o: vu-meter.c vu-meter.h

clean:
	rm -f vu-meter main.o vu-meter.o
