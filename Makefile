SOURCES = $(wildcard *.cpp) 
SOURCES := $(filter-out tcp_socket_client.cpp, $(SOURCES))
SOURCES := $(filter-out tcp_socket_server.cpp, $(SOURCES))
OBJECTS = $(SOURCES:%.cpp=%.o)
COMPILER = g++
FLAGS = -std=c++14 -O3 -Wall -Wvla -Werror -Wextra -pedantic #-DSOCKET_LOG_COMMUNICATION

all: clean $(OBJECTS)

# Rule for `make *.o`, the make program uses this whenever it sees a
# *.o make dependency like in the rule above
%.o:
	$(COMPILER) $(FLAGS) -c $*.cpp

install: SocketRAII.cpp SocketUtilities.cpp
	$(COMPILER) $(FLAGS) SocketRAII.cpp -c
	$(COMPILER) $(FLAGS) SocketUtilities.cpp -c
	ar rcs cppsockets.a SocketRAII.o SocketUtilities.o
	rm SocketRAII.o SocketUtilities.o

clean:
	rm -f a.out
	rm -f *.o
	rm -f sampleserver
	rm -f sampleclient
	rm -f cppsockets.a

debug: FLAGS += -g3 -DDEBUG
debug: $(OBJECTS)
	$(COMPILER) $(FLAGS) $(OBJECTS)

# build sample server and client sources
tcp_socket_client.o: tcp_socket_client.cpp
	$(COMPILER) $(FLAGS) -c tcp_socket_client.cpp
tcp_socket_server.o: tcp_socket_server.cpp
	$(COMPILER) $(FLAGS) -c tcp_socket_server.cpp

# Build sample server and client
sampleserver: $(OBJECTS) tcp_socket_server.o
	$(COMPILER) $(FLAGS) $(OBJECTS) tcp_socket_server.o -o $@

sampleclient: $(OBJECTS) tcp_socket_client.o
	$(COMPILER) $(FLAGS) $(OBJECTS) tcp_socket_client.o -o $@
