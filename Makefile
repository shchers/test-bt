CC := $(CROSS_COMPILE)gcc
CFLAGS += -std=gnu89
CFLAGS += -D__USE_MISC
LDFLAGS += -pthread
BIN_OUT := test-bt

all:
	$(CC) $(CFLAGS) $(LDFLAGS) main.c sserial.c -Wall -o test-bt

clean:
	rm -vf $(BIN_OUT) *.o
