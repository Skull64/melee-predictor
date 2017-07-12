CXX = g++
CXXFLAGS = -O2

ASTYLE_DIR = $$HOME/astyle

all: clean build run

build:
	$(CXX) $(CXXFLAGS) -c Bracket.cpp
	$(CXX) $(CXXFLAGS) predictor.cpp Bracket.o -o predictor

run:
	./predictor

format:
	$(ASTYLE_DIR)/astyle --options=$(ASTYLE_DIR)/google.ini \
	                     --verbose --formatted *.cpp *.hpp

clean:
	rm -f *.o predictor
