.SILENT:

default:
	gcc -Wall -Wextra -pthread -o lab2_list lab2_list.c SortedList.c SortedList.h

tests:
	./test.sh

profile.gperf:
	rm -f ./raw.gperf
	LD_PRELOAD=/u/eng/class/classboy/lib/libprofiler.so.0 \
		CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=thread_init ./lab2_list ./raw.gperf >> $@
	rm -f ./raw.gperf

graphs:
	gnuplot lab2b.gp

dist: build tests graphs
	tar -cvzf lab2a-004791432.tar.gz Makefile README lab2_add.c lab2_list.c SortedList.h SortedList.c test.sh *.png *.csv *.gp

clean:
	rm -f *.tar.gz lab2_add lab2_list *.gch *.o *.csv *png
