SOURCES = $(wildcard *.cpp) 
OBJECTS = $(SOURCES:%.cpp=%.o)
COMPILER = g++
FLAGS = -std=c++11 -O3 -Wall -Wvla -Werror -Wextra -pedantic

all: clean $(OBJECTS)
	$(COMPILER) $(FLAGS) $(OBJECTS) 

# Rule for `make *.o`, the make program uses this whenever it sees a
# *.o make dependency like in the rule above
%.o:
	$(COMPILER) $(FLAGS) -c $*.cpp

clean:
	rm -f a.out
	rm -f *.o

debug: FLAGS += -g3 -DDEBUG
debug: $(OBJECTS)
	$(COMPILER) $(FLAGS) $(OBJECTS)

sampleserver: all 
	./a.out 8000
