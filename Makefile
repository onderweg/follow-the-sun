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

start:
	launchctl load eu.onderweg.follow.plist
	launchctl start eu.onderweg.follow
	launchctl list | grep 'follow'

stop:
	launchctl unload eu.onderweg.follow.plist
	
.PHONY: all install