CFLAGS = -g -Wall -std=gnu11
LIBS = -lpthread

SRCS = simple_http.c Server.c
EXECUTABLE = simple_http
CC = gcc

all:
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SRCS) $(LIBS)

.PHONY:clean
clean:
	rm $(EXECUTABLE)
