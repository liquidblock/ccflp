CC=gcc
CFLAGS=-c -Wall -std=gnu11 -Wpedantic -Wextra -O3
LDFLAGS=
SRCDIR=.
SOURCES=$(wildcard $(SRCDIR)/*.c) 
HEADERS=$(wildcard $(SRCDIR)/*.h)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ccflp
PREFIX=/usr/bin

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
		$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
		$(CC) $(CFLAGS) $< -o $@

$(OBJECTS): $(HEADERS)

clean:
		rm -f $(SRCDIR)/*.o

install:
		cp $(EXECUTABLE) $(PREFIX)

uninstall:
		rm -vi $(PREFIX)/$(EXECUTABLE)
