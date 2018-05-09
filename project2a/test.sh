#!/bin/sh

#Run your program for ranges of iterations (100, 1000, 10000, 100000) values, 
#capture the output, and note how many threads and iterations it takes to (fa
#irly consistently) result in a failure (non-zero sum)
for i in 2 4 8 12
do
	for j in 100 1000 10000 100000
	do
		./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
	done
done


#Re-run your tests, with yields, for ranges of threads (2,4,8,12) and iterat
#ions (10, 20, 40, 80, 100, 1000, 10000, 100000)
for i in 2 4 8 12
do
	for j in 10 20 40 80 100 1000 10000 100000
	do
		./lab2_add --yield --threads=$i --iterations=$j >> lab2_add.csv
	done
done

#Compare the average execution time of the yield and non-yield versions a range
# threads (2, 8) and of iterations (100, 1000, 10000, 100000)
for i in 2 3 4 5 6 7 8
do
	for j in 100 1000 10000 100000
	do
		./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
	done
done

for i in 2 3 4 5 6 7 8
do
	for j in 100 1000 10000 100000
	do
		./lab2_add --yield --threads=$i --iterations=$j >> lab2_add.csv
	done
done


#Use your --yield options to confirm that, even for large numbers of threads (2, 
#4, 8, 12) and iterations (10,000 for mutexes and CAS, only 1,000 for spin locks
#) that reliably failed in the unprotected scenarios, all three of these seriali
#zation mechanisms eliminate the race conditions in the add critical section

for i in 2 4 8 12
do
	./lab2_add --yield --sync=m --iterations=10000 --threads=$i >> lab2_add.csv
done
for i in 2 4 8 12
do
	./lab2_add --yield --sync=c --iterations=10000 --threads=$i >> lab2_add.csv
done
for i in 2 4 8 12
do
	./lab2_add --yield --sync=s --iterations=1000 --threads=$i >> lab2_add.csv
done

#Using a large enough number of iterations (e.g. 10,000) to overcome the issues 
#raised in the question 2.1.3, test all four (no yield) versions (unprotected, 
#mutex, spin-lock, compare-and-swap) for a range of number of threads (1,2,4,8,
#12), capture the output, and plot (using the supplied data reduction script) t
#he average time per operation (non-yield), vs the number of threads. 

for i in 1 2 4 8 12
do
	./lab2_add --iterations=10000 --threads=$i >> lab2_add.csv
done
for i in 1 2 4 8 12
do
	./lab2_add --sync=m --iterations=10000 --threads=$i >> lab2_add.csv
done
for i in 1 2 4 8 12
do
	./lab2_add --sync=c --iterations=10000 --threads=$i >> lab2_add.csv
done
for i in 1 2 4 8 12
do
	./lab2_add --sync=s --iterations=10000 --threads=$i >> lab2_add.csv
done


#Run your program with a single thread, and increasing numbers of iterations (10,
# 100, 1000, 10000, 20000), capture the output
for i in 10 100 1000 10000 20000
do
    ./lab2_list --iterations=$i >> lab2_list.csv
done

#Run your program and see how many parallel threads (2,4,8,12) and iterations (1,
# 10,100,1000) it takes to fairly consistently demonstrate a problem

for i in 2 4 8 12
do
	for j in 1 10 100 1000
	do
		./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
	done
done

#run it again using various combinations of yield options and see how many threads
# (2,4,8,12) and iterations (1, 2,4,8,16,32) it takes to fairly consistently demon
#strate the problem
for i in 2 4 8 12
do
	for j in 1 2 4 8 16 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=i >> lab2_list.csv
	done
	for j in 1 2 4 8 16 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=d >> lab2_list.csv
	done
	for j in 1 2 4 8 16 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=il >> lab2_list.csv
	done
	for j in 1 2 4 8 16 32
	do
		./lab2_list --threads=$i --iterations=$j --yield=dl >> lab2_list.csv
	done
done

#Add two new options to your program to call two new versions of these methods: one
# set of operations protected by pthread_mutexes (--sync=m), and another protected 
#by test-and-set spin locks (--sync=s). Using your --yield options, demonstrate that
# either of these protections seems to eliminate all of the problems, even for moder
#ate numbers of threads (12) and iterations (32)
./lab2_list --threads=12 --iterations=32 --yield=i --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=m >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=i --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=d --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=il --sync=s >> lab2_list.csv
./lab2_list --threads=12 --iterations=32 --yield=dl --sync=s >> lab2_list.csv

#Choose an appropriate number of iterations (e.g. 1000) to overcome start-up costs and
# rerun your program without the yields for a range of # threads (1, 2, 4, 8, 12, 16, 24).
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2_list.csv
done
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2_list.csv
done
