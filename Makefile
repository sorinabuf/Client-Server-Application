CC=g++ -std=c++11 -Wall -Wextra

all: build

build: server subscriber

server: server.o
	$(CC) $^ -o $@

server.o: server.cpp
	$(CC) $^ -c

subscriber: subscriber.o
	$(CC) $^ -o $@

subscriber.o: subscriber.cpp
	$(CC) $^ -c

clean:
	rm -f *.o server subscriber