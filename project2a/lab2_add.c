#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sched.h>

int num_threads = 1;
int num_iterations = 1;
int my_yield = 0;
char my_lock = 'n';
int my_spin = 0;
long long my_counter = 0;
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

void add(long long *pointer, long long value){
    long long sum = *pointer + value;
    if (my_yield)
        sched_yield();
    *pointer = sum;
}

void* runner(){
	int i;
	for(i = 0; i < num_iterations; i++){
		if(my_lock == 'n'){
			add(&my_counter, 1);
		}else if(my_lock == 's'){
			while (__sync_lock_test_and_set(&my_spin, 1));
			add(&my_counter, 1);
			__sync_lock_release(&my_spin);
		}else if(my_lock == 'm'){
			pthread_mutex_lock(&my_mutex);
			add(&my_counter, 1);
			pthread_mutex_unlock(&my_mutex);
		}else if(my_lock == 'c'){
			long long old_val, new_val;
			do{
				old_val = my_counter;
				new_val = old_val + 1;
			}while(__sync_val_compare_and_swap(&my_counter, old_val, new_val) != old_val);
		}
	}
	for(i = 0; i < num_iterations; i++){
		if(my_lock == 'n'){
			add(&my_counter, -1);
		}else if(my_lock == 's'){
			while (__sync_lock_test_and_set(&my_spin, -1));
			add(&my_counter, -1);
			__sync_lock_release(&my_spin);
		}else if(my_lock == 'm'){
			pthread_mutex_lock(&my_mutex);
			add(&my_counter, -1);
			pthread_mutex_unlock(&my_mutex);
		}else if(my_lock == 'c'){
			long long old_val, new_val;
			do{
				old_val = my_counter;
				new_val = old_val - 1;
			}while(__sync_val_compare_and_swap(&my_counter, old_val, new_val) != old_val)
		}
	}
}

int main(int argc, char **argv){
	static struct option long_option[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", no_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};

	int op;
	while(1){
		op = getopt_long(argc, argv, "i:t:ys:", long_option, NULL);
		if(op == -1)
			break;
		switch(op){
			case 't':
				num_threads = atoi(optarg);
				break;
			case 'i':
				num_iterations = atoi(optarg);
				break;
			case 'y':
				my_yield = 1;
				break;
			case 's':
				if(optarg[0] == 's'|| optarg[0] == 'm' || optarg[0] == 'c'){
					my_lock = optarg[0];
				}else{
					fprintf(stderr, "Error: unrecoginized arguement\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Error: unrecoginized arguement\n");
				exit(1);
		}
	}

	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);

	pthread_t my_threads[num_threads];

	int i;
	for(i = 0; i < num_threads; i++){
		if(pthread_create(&my_threads[i], NULL, runner, NULL)){
			fprintf(stderr, "Error: cannot create threads\n");
			exit(1);
		}
	}

	for(i = 0; i< num_threads; i++){
		if(pthread_join(my_threads[i], NULL)){
			fprintf(stderr, "Error: cannot join threads\n");
			exit(1);
		}
	}

	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);

	long long my_time = (e_time.tv_sec - s_time.tv_sec)*1000000000;
	my_time += e_time.tv_nsec;
	my_time -= s_time.tv_nsec;

	int my_ops = num_threads*num_iterations*2;
	long long op_time = my_time/my_ops;
	char* res_yield = "";
	if(my_yield){
		res_yield = "yield-"
	}
	char* res_lock = "";
	if(my_lock == 'n'){
		res_lock = "none";
	}else{
		strcat(res_lock, my_lock);
	}
	printf("add-%s%s,%d,%d,%d,%lld,%lld,%lld\n", res_yield, res_lock, num_threads, num_iterations,
			my_ops, my_time, op_time, my_counter);
	exit(0);
}