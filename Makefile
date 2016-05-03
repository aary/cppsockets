COMPILER = g++
USER_FLAGS = 
INCLUDE_DIR = include
FLAGS = -std=c++14 -O3 -Wall -Wvla -Werror -Wextra -pedantic $(USER_FLAGS) -I $(INCLUDE_DIR)


all: clean $(OBJECTS)

# Rule for `make *.o`, the make program uses this whenever it sees a
# *.o make dependency like in the rule above
%.o:
	$(COMPILER) $(FLAGS) -c $*.cpp

install: src/SocketRAII.cpp src/SocketUtilities.cpp
	$(COMPILER) $(FLAGS) src/SocketRAII.cpp -c
	$(COMPILER) $(FLAGS) src/SocketUtilities.cpp -c
	ar rcs libcppsockets.a SocketRAII.o SocketUtilities.o
	@rm *.o
	ln -s include/* ./

clean_private:
	rm -f a.out
	rm -f *.o
	rm -f libcppsockets.a
	find . -type l -maxdepth 1 -delete

clean_public:
	rm -f sampleserver
	rm -f sampleclient

clean: clean_private clean_public
	@printf ""

debug: FLAGS += -g3 -DDEBUG
debug: $(OBJECTS)
	$(COMPILER) $(FLAGS) $(OBJECTS)

# build sample server and client sources
tcp_client.o: tests/tcp_client.cpp
	$(COMPILER) $(FLAGS) -c tests/tcp_client.cpp
tcp_server.o: tests/tcp_server.cpp
	$(COMPILER) $(FLAGS) -c tests/tcp_server.cpp

# Build sample server and client
sampleserver: install tcp_server.o
	$(COMPILER) $(FLAGS) libcppsockets.a tcp_server.o -o $@
	@make clean_private

sampleclient: install tcp_client.o
	$(COMPILER) $(FLAGS) libcppsockets.a tcp_client.o -o $@
	@make clean_private
