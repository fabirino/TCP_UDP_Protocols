all: cliente server

cliente: tcp_client.c
	gcc -Wall tcp_client.c -o cliente

server: tcp_server.c
	gcc -Wall -pthread tcp_server.c -o server
