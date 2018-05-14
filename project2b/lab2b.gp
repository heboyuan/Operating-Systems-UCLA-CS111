#! /usr/local/cs/bin/gnuplot
#
# purpose:
#     generate graph for lab2b
#
# input: lab2b_list.csv
#    1. test name
#    2. # threads
#    3. # iterations per thread
#    4. # lists
#    5. # operations performed (threads x iterations x (ins + lookup + delete))
#    6. run time (ns)
#    7. run time per operation (ns)
#    8.wait time for locker (only for mutex)
#
# output:
#    lab2b_1.png ... total number of operations per second 
#    lab2b_list-2.png ... wait-for-lock time, and the average time per operation against the number of competing threads
#    lab2b_list-3.png ... threads and iterations that run (protected) w/o failure
#    lab2b_list-4.png ... mutex aggregated throughput vs. the number of threads
#    lab2b_list-5.png ... spin aggregated throughput vs. the number of threads
#

# general plot parameters
set terminal png
set datafile separator ","

set title "total number of operations per second"
set xlabel "threads"
set logscale x 2
set xrange[0.75:]
set ylabel "throughput"
set logscale y 10
set output 'lab2b_1.png'
set key left top
plot \
     "< grep -E 'list-none-m,[0-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
     title  'throughput w/mutex' with linespoints lc rgb 'blue', \
     "< grep -E 'list-none-s,[0-9]+,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
     title  'throughput w/spin-lock' with linespoints lc rgb 'red'


set title "wait-for-lock time & average time per operation vs threads"
set xlabel "threads"
set logscale x 2
set xrange[0.75:]
set ylabel "wait time"
set logscale y 10
set output 'lab2b_2.png'
set key left top
plot \
     "<grep -E 'list-none-m,[0-9],1000,1,|list-none-m,16,1000,1,|list-none-m,24,1000,1,' lab2b_list.csv" \
     using ($2):($7) title 'time per opration' with linespoints lc rgb 'blue',\
     "<grep -E 'list-none-m,[0-9],1000,1,|list-none-m,16,1000,1,|list-none-m,24,1000,1,' lab2b_list.csv" \
     using ($2):($8) title 'wait time' with linespoints lc rgb 'red'

set title "threads and iterations that run (protected) w/o failure"
set xlabel "threads"
set logscale x 2
set xrange[0.75:]
set ylabel "iterations"
set logscale y 10
set output 'lab2b_3.png'
set key left top
plot \
     "<grep -E 'list-id-none,[0-9]+,[0-9]+,4,' lab2b_list.csv" \
     using ($2):($3) title 'no sync' with points lc rgb 'blue',\
     "<grep -E 'list-id-m,[0-9]+,[0-9]+,4,' lab2b_list.csv" \
     using ($2):($3) title 'mutex' with points lc rgb 'red',\
     "<grep -E 'list-id-s,[0-9]+,[0-9]+,4,' lab2b_list.csv" \
     using ($2):($3) title 'spinlock' with points lc rgb 'green'

set title "mutex aggregated throughput vs. the number of threads"
set xlabel "threads"
set logscale x 2
set xrange[0.75:]
set ylabel "throughput"
set logscale y 10
set output 'lab2b_4.png'
set key left top
plot \
     "<grep -E 'list-none-m,[1-9],1000,1,|list-none-m,12,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '1 list' with linespoints lc rgb 'blue' ,\
     "<grep -E 'list-none-m,[1-9]+,1000,4' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '4 list' with linespoints lc rgb 'brown' ,\
     "<grep -E 'list-none-m,[1-9]+,1000,8' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '8 list' with linespoints lc rgb 'red' ,\
     "<grep -E 'list-none-m,[1-9]+,1000,16' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '16 list' with linespoints lc rgb 'green'


set title "spin aggregated throughput vs. the number of threads"
set xlabel "threads"
set logscale x 2
set xrange[0.75:]
set ylabel "throughput"
set logscale y 10
set output 'lab2b_5.png'
set key left top
plot \
     "<grep -E 'list-none-s,[1-9],1000,1,|list-none-s,12,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '1 list' with linespoints lc rgb 'blue' ,\
     "<grep -E 'list-none-s,[1-9]+,1000,4' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '4 list' with linespoints lc rgb 'brown' ,\
     "<grep -E 'list-none-s,[1-9]+,1000,8' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '8 list' with linespoints lc rgb 'red' ,\
     "<grep -E 'list-none-s,[1-9]+,1000,16' lab2b_list.csv" using ($2):(1000000000/($7))\
     title '16 list' with linespoints lc rgb 'green'

