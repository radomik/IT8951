SRCS=IT8951.c miniGUI.c main.c
CC=gcc
CFLAGS=-Wall -march=native -mtune=native -O2 -pipe
TARGET=eink
LDFLAGS=-lbcm2835 -lpthread

$(TARGET):$(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
