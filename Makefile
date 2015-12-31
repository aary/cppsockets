SOURCES = $(wildcard *.cpp src/*.cpp) 
OBJECTS = $(SOURCES:%.cpp=%.o)
COMPILER = g++
FLAGS = -std=c++11 -O3 -Wall -Wvla -Werror -Wextra -pedantic -DSOCKET_LOG_COMMUNICATION

# Rule for `make *.o`, the make program uses this whenever it sees a
# *.o make dependency like in the rule above
%.o:
	$(COMPILER) $(FLAGS) -c $*.cpp

clean:
	rm -f a.out
	rm -f *.o
	rm -f sampleserver

debug: FLAGS += -g3 -DDEBUG
debug: $(OBJECTS)
	$(COMPILER) $(FLAGS) $(OBJECTS)

sampleserver: clean $(OBJECTS) 
	$(COMPILER) $(FLAGS) $(OBJECTS) -o sampleserver
