PROJECT=router
SOURCES=ARPTable.cpp IPPacketQueue.cpp Router.cpp RoutingTable.cpp skel.c
LIBRARY=nope
INCPATHS=include
LIBPATHS=.
LDFLAGS=
CFLAGS=-c -Wall -Wextra -fPIC -O2 -march=native -mtune=native
CPPFLAGS=$(CFLAGS) --std=c++17
CC=gcc
CPP=g++

# Automatic generation of some important lists
OBJECTS_1=$(SOURCES:.c=.o)
OBJECTS=$(OBJECTS_1:.cpp=.o)
INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))
LIBFLAGS=$(foreach TMP,$(LIBPATHS),-L$(TMP))

# Set up the output file names for the different output types
BINARY=$(PROJECT)

all: $(SOURCES) $(BINARY)

$(BINARY): $(OBJECTS)
	$(CPP) $(LIBFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(CPP) $(INCFLAGS) $(CPPFLAGS) $< -o $@

.c.o:
	$(CC) $(INCFLAGS) $(CFLAGS) $< -o $@

distclean: clean
	rm -f $(BINARY)

clean:
	rm -f $(OBJECTS)
