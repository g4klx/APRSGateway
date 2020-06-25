CC      = gcc
CXX     = g++
CFLAGS  = -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lpthread
LDFLAGS = -g

OBJECTS = APRSGateway.o APRSWriterThread.o Conf.o Log.o StopWatch.o TCPSocket.o Thread.o Timer.o UDPSocket.o Utils.o

all:		APRSGateway

APRSGateway:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o APRSGateway

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 APRSGateway /usr/local/bin/

clean:
		$(RM) APRSGateway *.o *.d *.bak *~
 