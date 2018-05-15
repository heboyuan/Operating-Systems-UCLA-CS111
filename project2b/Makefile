.SILENT:

default: build lab2_list

tests: build
	./test.sh

graphs:
	gnuplot lab2b.gp

dist: tests graphs
	tar -cvzf lab2b-004791432.tar.gz Makefile README test.sh *.png *.csv lab2b.gp lab2_list.c SortedList.h SortedList.c profile.out
clean:
	rm -f *.tar.gz ./lab2_list *.o

lab2_list: clean lab2_list.o SortedList.o
	gcc -o $@ -pthread -g lab2_list.o SortedList.o

profile: lab2_list build
	rm -f ./raw.gperf
	LD_PRELOAD=/u/eng/class/classboy/my_lib/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=runner ./lab2_list ./raw.gperf >> profile.gperf
	mv profile.gperf profile.out
	rm -f ./raw.gperf

build:
	gcc -Wall -Wextra -g -pthread lab2_list.c SortedList.c -o lab2_list
