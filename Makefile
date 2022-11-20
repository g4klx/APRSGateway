CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread -DHAVE_LOG_H
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = APRSGateway.o APRSWriterThread.o Conf.o Log.o StopWatch.o TCPSocket.o Thread.o Timer.o UDPSocket.o Utils.o

all:		APRSGateway

APRSGateway:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o APRSGateway

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

APRSGateway.o: GitVersion.h FORCE

.PHONY: GitVersion.h

FORCE:


install:
		install -m 755 APRSGateway /usr/local/bin/

clean:
		$(RM) APRSGateway *.o *.d *.bak *~ GitVersion.h

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif
