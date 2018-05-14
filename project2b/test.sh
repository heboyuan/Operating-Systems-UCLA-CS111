# Capture the output, and produce a plot of the total number of operations per second for each
# synchronization method
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list_1.csv
done
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list_1.csv
done

# Run your program with --yield=id, 4 lists, 1,4,8,12,16 threads, and 1, 2, 4, 8, 16 iteratio
# ns (and no synchronization) to see how many iterations it takes to reliably fail (and make 
# sure your Makefile expects some of these tests to fail).

for i in 1 4 8 12 16
do
	for j in 1 2 4 8 16
	do
		./lab2_list --threads=$i  --iterations=$j   --yield=id --lists=4 >> lab2b_list.csv
	done
done

# Run your program with --yield=id, 4 lists, 1,4,8,12,16 threads, and 10, 20, 40, 80 iterations
# , --sync=s and --sync=m to confirm that updates are now properly protected (i.e., all runs su
# cceeded).
for i in 1 4 8 12 16
do
	for j in 10 20 40 80
	do
		./lab2_list --threads=$i  --iterations=$j   --yield=id --lists=4 --sync=s >> lab2b_list.csv
	done
done
for i in 1 4 8 12 16
do
	for j in 10 20 40 80
	do
		./lab2_list --threads=$i  --iterations=$j   --yield=id --lists=4 --sync=m >> lab2b_list.csv
	done
done

#Rerun both synchronized versions, without yields, for 1000 iterations, 1,2,4,8,12 threads, and
#  1,4,8,16 lists
for i in 1 2 4 8 12
do
	for j in 1 4 8 16
	do
		./lab2_list --threads=$i  --iterations=1000 --lists=$j --sync=m >> lab2b_list.csv
	done
done
for i in 1 2 4 8 12
do
	for j in 1 4 8 16
	do
		./lab2_list --threads=$i  --iterations=1000 --lists=$j --sync=s >> lab2b_list.csv
	done
done