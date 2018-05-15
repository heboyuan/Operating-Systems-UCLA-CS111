.SILENT:

default: build lab2_list

tests: build
	./test.sh

graphs:
	gnuplot lab2b.gp

dist: tests graphs
	tar -cvzf lab2b-004757414.tar.gz Makefile README test.sh *.png *.csv plot.gp lab2_list.c SortedList.h SortedList.c profile.out
clean:
	rm -f *.tar.gz ./lab2_list *.o

lab2_list: clean lab2_list.o SortedList.o
	gcc -o $@ -pthread -g lab2_list.o SortedList.o

profile: lab2_list build
	rm -f ./raw.gperf
	LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=thread_init ./lab2_list ./raw.gperf >> profile.gperf
	mv profile.gperf profile.out
	rm -f ./raw.gperf

build:
	gcc -Wall -Wextra -g -pthread lab2_list.c SortedList.c -o lab2_list
