.SILENT:


default:
	gcc -Wall -Wextra -pthread -g -lmraa -lm -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -pthread -g -lmraa -lm -lcrypto -lssl -o lab4c_tls lab4c_tls.c

dist:
	tar -cvzf lab4c-004791432.tar.gz lab4c_tcp.c lab4c_tls.c README Makefile

clean: 
	rm -f lab4c_tcp lab4c_tls *.tar.gz *.txt
