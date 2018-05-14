# Capture the output, and produce a plot of the total number of operations per second for each
# synchronization method
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
done
for i in 1 2 4 8 12 16 24
do
	./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done
