CXX = g++
CXXFLAGS = -O2

all: clean build run

build:
	$(CXX) $(CXXFLAGS) -c Bracket.cpp
	$(CXX) $(CXXFLAGS) predictor.cpp Bracket.o -o predictor

run:
	./predictor

clean:
	rm -f *.o predictor
