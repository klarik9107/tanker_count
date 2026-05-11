CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
LDFLAGS = -lm

TARGET  = tanker_count
OBJS    = tanker_count.o lodepng.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

tanker_count.o: tanker_count.c lodepng.h
	$(CC) $(CFLAGS) -c tanker_count.c

lodepng.o: lodepng.c lodepng.h
	$(CC) $(CFLAGS) -c lodepng.c

clean:
	rm -f $(OBJS) $(TARGET) tankers_detected.png

.PHONY: all clean
