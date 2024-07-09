CXX = g++
CXXFLAGS = -std=c++11 -g -I ../
TARGETS = Thread_test

.PHONY: all clean

all: ${TARGETS}

Thread_test: Thread_test.cpp ../Thread.cpp ../Util.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

clean:
	rm ${TARGETS}