CXX = g++
CXXFLAGS = -std=c++11 -g
SRCS = main.cpp Fiber.cpp

.PHONY: all clean

all: test

test: ${SRCS}
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f test