CC=gcc
CFLAGS += -F/System/Library/PrivateFrameworks
LDFLAGS += -framework Foundation 
LDFLAGS += -framework CoreBrightness 
LDFLAGS += -lobjc

SRCS = $(wildcard *.c)
TARGET=follow

all: follow

follow: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

install:
	install -v $(TARGET) /usr/local/bin/$(TARGET)	
	cp eu.onderweg.follow.plist ~/Library/LaunchAgents	
	
.PHONY: all install