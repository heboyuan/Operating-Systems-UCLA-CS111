.SILENT:

default: client server

client:
	gcc -Wall -Wextra -lz -o lab1b-client lab1b-client.c

server:
	gcc -Wall -Wextra -lz -o lab1b-server lab1b-server.c

clean:
	rm -f *.tar.gz lab1b-client lab1b-server *.txt

dist:
	tar -czvf lab1b-004791432.tar.gz Makefile README lab1b-client.c lab1b-server.c