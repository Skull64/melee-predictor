CXX = g++
CXXFLAGS = -O2 -fopenmp
CXXFLAGS_DEBUG = -g -DPROGRESS_BAR

ASTYLE_DIR = $$HOME/astyle

all: clean build

build:
	$(CXX) $(CXXFLAGS) -c Bracket.cpp
	$(CXX) $(CXXFLAGS) predictor.cpp Bracket.o -o predictor

debug:
	$(CXX) $(CXXFLAGS_DEBUG) -c Bracket.cpp
	$(CXX) $(CXXFLAGS_DEBUG) predictor.cpp Bracket.o -o predictor

run:
	./predictor

format:
	$(ASTYLE_DIR)/astyle --options=$(ASTYLE_DIR)/google.ini \
	                     --verbose --formatted *.cpp *.hpp

clean:
	rm -f *.o predictor
