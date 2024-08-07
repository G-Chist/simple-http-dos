# MAKEFILE FOR DOS ATTACK

server: server.o
	gcc -g -Wall simple_http_server.o -o http_server -lm

client: client.o
	gcc -g -Wall simple_http_client.o -o http_client -lm

attacker: attacker.o
	gcc -g -Wall dos_attacker.o -o dos_client -lm

server.o: simple_http_server.c
	gcc -g -Wall -c simple_http_server.c -o simple_http_server.o

client.o: simple_http_client.c
	gcc -g -Wall -c simple_http_client.c -o simple_http_client.o

attacker.o: dos_attacker.c
	gcc -g -Wall -c dos_attacker.c -o dos_attacker.o

clean: 
	rm -f simple_http_server.o http_server simple_http_client.o http_client dos_attacker.o dos_client index.html

all: server client attacker