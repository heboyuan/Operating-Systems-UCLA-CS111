.SILENT:


default:
	gcc -Wall -Wextra -pthread -g -lmraa -lm -o lab4b lab4b.c

dist:
	tar -cvzf lab4b-004791432.tar.gz lab4b.c README Makefile

clean: 
	rm lab4b *.o *.tar.gz

check: default
	./lab4b << OFF\n