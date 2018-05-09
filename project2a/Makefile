.SILENT:

build:
	gcc -Wall -Wextra -pthread -o lab2_add lab2_add.c
	gcc -Wall -Wextra -pthread -o lab2_list lab2_list.c SortedList.c SortedList.h

tests:
	./test.sh

graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist: build tests graphs
	tar -cvzf lab2a-004791432.tar.gz Makefile README lab2_add.c lab2_list.c SortedList.h SortedList.c test.sh *.png *.csv *.gp

clean:
	rm -f *.tar.gz lab2_add lab2_list *.gch *.o *.png *.csv *.gp