all: server client

server:
	gcc -lcurses -std=c99 -O2 server.c -o organ-server

client:
	gcc -std=c99 client.c -o organ-client

clean:
	rm organ-client
	rm organ-server
