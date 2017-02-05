CC = g++
CFLAGS = -c -Wall
LDFLAGS = 

SOURCES=main.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE = ttrack

CFLAGS += `pkg-config --cflags gtkmm-3.0`
LDFLAGS += `pkg-config --libs gtkmm-3.0`

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cc.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
	
.PHONEY: run
run: all
	./ttrack
