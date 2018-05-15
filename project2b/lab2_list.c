#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include "SortedList.h"

int num_threads = 1;
int num_iterations = 1;
int num_lists = 1;
int opt_yield = 0;
char my_lock = 'n';
long long *mutex_time;
typedef struct{
	SortedList_t m_list;
	pthread_mutex_t my_mutex;
	int my_spin;
}My_Sublist;
My_Sublist* my_list;
SortedListElement_t* my_list_ele;

//https://www.codeproject.com/Questions/640193/Random-string-in-language-C

int my_hash(const char *str){
	int val = *str;
	return val;
}

void* runner(void* temp){
	int my_start = *((int *) temp);
	int my_tid = my_start/num_iterations;
	int i;
	struct timespec s_time, e_time;
	My_Sublist *temp_sublist;

	for(i = my_start; i < my_start + num_iterations; i++){
		temp_sublist = &my_list[my_hash(my_list_ele[i].key)%num_lists];
		switch(my_lock){
			case 'n':
			{
				SortedList_insert(&(temp_sublist->m_list), &my_list_ele[i]);
				break;
			}
			case 's':
			{
				clock_gettime(CLOCK_MONOTONIC, &s_time);
				while(__sync_lock_test_and_set(&(temp_sublist->my_spin), 1));
				clock_gettime(CLOCK_MONOTONIC, &e_time);

				long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
				temp_time += e_time.tv_nsec;
				temp_time -= s_time.tv_nsec;
				mutex_time[my_tid] += temp_time;

				SortedList_insert(&(temp_sublist->m_list), &my_list_ele[i]);
				__sync_lock_release(&(temp_sublist->my_spin));
				break;
			}
			case 'm':
			{
				clock_gettime(CLOCK_MONOTONIC, &s_time);
				pthread_mutex_lock(&(temp_sublist->my_mutex));
				clock_gettime(CLOCK_MONOTONIC, &e_time);

				long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
				temp_time += e_time.tv_nsec;
				temp_time -= s_time.tv_nsec;
				mutex_time[my_tid] += temp_time;

				SortedList_insert(&(temp_sublist->m_list), &my_list_ele[i]);
				pthread_mutex_unlock(&(temp_sublist->my_mutex));
				break;
			}
		}
	}

	int len = 0;
	switch(my_lock){
		case 'n':
		{
			for (i = 0; i < num_lists; i++) {
				if((len = SortedList_length(&(my_list[i].m_list))) < 0){
					break;
				}
			}
			break;
		}
		case 's':
		{
			clock_gettime(CLOCK_MONOTONIC, &s_time);
			for(i = 0; i < num_lists; i++){
				while(__sync_lock_test_and_set(&(my_list[i].my_spin), 1));
			}
			clock_gettime(CLOCK_MONOTONIC, &e_time);

			long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
			temp_time += e_time.tv_nsec;
			temp_time -= s_time.tv_nsec;
			mutex_time[my_tid] += temp_time;

			for(i = 0; i < num_lists; i++){
				if((len = SortedList_length(&(my_list[i].m_list))) < 0){
					break;
				}
			}
			for(i = 0; i < num_lists; i++){
				__sync_lock_release(&(my_list[i].my_spin));
			}
			break;
		}
		case 'm':
		{
			clock_gettime(CLOCK_MONOTONIC, &s_time);
			for (i = 0; i < num_lists; i++){
				pthread_mutex_lock(&my_list[i].my_mutex);
			}
			clock_gettime(CLOCK_MONOTONIC, &e_time);

			long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
			temp_time += e_time.tv_nsec;
			temp_time -= s_time.tv_nsec;
			mutex_time[my_tid] += temp_time;
			
			for (i = 0; i < num_lists; i++){
				if((len = SortedList_length(&my_list[i].m_list)) < 0){
					break;
				}
			}

			for (i = 0; i < num_lists; i++){
				pthread_mutex_unlock(&my_list[i].my_mutex);
			}
			break;
		}

	}

	if(len < 0){
		fprintf(stderr, "Error: list corruption len incorrect\nyield: %d  lock: %c  threads: %d  iter: %d\n"
			, opt_yield, my_lock, num_threads, num_iterations);
		exit(2);
	}

	SortedListElement_t* temp_ele = NULL;
	for(i = my_start; i < my_start + num_iterations; i++){
		temp_sublist = &my_list[my_hash(my_list_ele[i].key)%num_lists];
		switch(my_lock){
			case 'n':
			{
				if(!(temp_ele = SortedList_lookup(&(temp_sublist->m_list), my_list_ele[i].key))){
					fprintf(stderr, "Error: list corruption and element disappear\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				if(SortedList_delete(temp_ele)){
					fprintf(stderr, "Error: list corruption and cannot delete\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				break;
			}
			case 'm':
			{
				clock_gettime(CLOCK_MONOTONIC, &s_time);
				pthread_mutex_lock(&(temp_sublist->my_mutex));
				clock_gettime(CLOCK_MONOTONIC, &e_time);

				long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
				temp_time += e_time.tv_nsec;
				temp_time -= s_time.tv_nsec;
				mutex_time[my_tid] += temp_time;

				if(!(temp_ele = SortedList_lookup(&(temp_sublist->m_list), my_list_ele[i].key))){
					fprintf(stderr, "Error: list corruption and element disappear\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				if(SortedList_delete(temp_ele)){
					fprintf(stderr, "Error: list corruption and cannot delete\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				pthread_mutex_unlock(&(temp_sublist->my_mutex));
				break;
			}
			case 's':
			{
				clock_gettime(CLOCK_MONOTONIC, &s_time);
				while(__sync_lock_test_and_set(&(temp_sublist->my_spin), 1));
				clock_gettime(CLOCK_MONOTONIC, &e_time);

				long long temp_time = (e_time.tv_sec - s_time.tv_sec) * 1000000000;
				temp_time += e_time.tv_nsec;
				temp_time -= s_time.tv_nsec;
				mutex_time[my_tid] += temp_time;

				if(!(temp_ele = SortedList_lookup(&temp_sublist->m_list, my_list_ele[i].key))){
					fprintf(stderr, "Error: list corruption and element disappear\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				if(SortedList_delete(temp_ele)){
					fprintf(stderr, "Error: list corruption and cannot delete\nyield: %d  lock: %c  threads: %d  iter: %d\n"
						, opt_yield, my_lock, num_threads, num_iterations);
					exit(2);
				}
				__sync_lock_release(&(temp_sublist->my_spin));
				break;
			}
		}

	}
	return NULL;
}

void handler(){
	fprintf(stderr, "Error: Segmentation fault\n");
	free(my_list_ele);
	free(my_list);
	free(mutex_time);
	exit(2);
}

int main(int argc, char **argv){
	srand(time(NULL));

	static struct option long_option[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{"lists", required_argument, 0, 'l'},
		{0,0,0,0}
	};

	int op;
	while(1){
		op = getopt_long(argc, argv, "i:t:y:s:l:", long_option, NULL);
		if(op == -1){
			break;
		}
		size_t i;
		switch(op){
			case 't':
				num_threads = atoi(optarg);
				break;
			case 'i':
				num_iterations = atoi(optarg);
				break;
			case 'y':
				for (i = 0; i < strlen(optarg); i++){
					if(optarg[i] == 'i') {
						opt_yield |= INSERT_YIELD;
					}else if(optarg[i] == 'd'){
						opt_yield |= DELETE_YIELD;
					}else if(optarg[i] == 'l'){
						opt_yield |= LOOKUP_YIELD;
					}else{
						fprintf(stderr, "Error: unrecoginized arguement\n");
						exit(1);
					}
				}
				break;
			case 's':
				if(optarg[0] == 's'|| optarg[0] == 'm'){
					my_lock = optarg[0];
				}else{
					fprintf(stderr, "Error: unrecoginized arguement\n");
					exit(1);
				}
				break;
			case 'l':
				num_lists = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Error: unrecoginized arguement\n");
				exit(1);
		}
	}

	int i;
	signal(SIGSEGV, handler);

	mutex_time = malloc(sizeof(long long) * num_threads);
	for(i = 0; i < num_threads; i++){
		mutex_time[i] = 0;
	}

	my_list = malloc(sizeof(My_Sublist)*num_lists);
	for(i = 0; i < num_lists; i++){
		my_list[i].m_list.key = NULL;
		my_list[i].m_list.next = &(my_list[i].m_list);
		my_list[i].m_list.prev = &(my_list[i].m_list);
		if(my_lock == 's'){
			my_list[i].my_spin = 0;
		}else if(my_lock == 'm'){
			pthread_mutex_init(&(my_list[i].my_mutex), NULL);
		}
	}

	my_list_ele = malloc(sizeof(SortedListElement_t) * num_threads * num_iterations);
	
	for(i = 0; i < num_threads * num_iterations; i++){
		char* temp_key = malloc(2 * sizeof(char));
		temp_key[0] = 'a' + rand() % 26;
		temp_key[1] = 0;
		my_list_ele[i].key = temp_key;
	}

	int start_loc[num_threads];
	for(i = 0; i < num_threads; i++){
		start_loc[i] = i*num_iterations;
	}

	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);

	pthread_t my_threads[num_threads];

	for(i= 0; i < num_threads; i++){
		if (pthread_create(&my_threads[i], NULL, runner, (void *) &start_loc[i])) {
			fprintf(stderr, "Error: cannot create threads\n");
			free(my_list_ele);
			free(my_list);
			free(mutex_time);
			exit(1);
		}
	}

	for (i = 0; i < num_threads; i++) {
		if (pthread_join(my_threads[i], NULL)) {
			fprintf(stderr, "Error: cannot join threads\n");
			free(my_list_ele);
			free(my_list);
			free(mutex_time);
			exit(1);
		}
	}

	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);
	
	for(i = 0; i < num_lists; i++){
		if(SortedList_length(&(my_list[i].m_list))!=0){
			fprintf(stderr, "Error: list corrupted list length not 0\n");
			free(my_list_ele);
			free(my_list);
			free(mutex_time);
			exit(1);
		}
	}

	long long my_time = (e_time.tv_sec - s_time.tv_sec)*1000000000;
	my_time += e_time.tv_nsec;
	my_time -= s_time.tv_nsec;
	int my_ops = num_threads*num_iterations*3;
	long long op_time = my_time/my_ops;
	char* res_option;
	switch(opt_yield){
		case 0x01:
			res_option = "i-";
			break;
		case 0x02:
			res_option = "d-";
			break;
		case 0x03:
			res_option = "id-";
			break;
		case 0x04:
			res_option = "l-";
			break;
		case 0x05:
			res_option = "il-";
			break;
		case 0x06:
			res_option = "dl-";
			break;
		case 0x07:
			res_option = "idl-";
			break;
		default:
			res_option = "none-";
	}
	char* res_lock;
	if(my_lock == 'n'){
		res_lock = "none";
	}else if(my_lock == 's'){
		res_lock = "s";
	}else if(my_lock == 'm'){
		res_lock = "m";
	}

	//printf("lock: %c\n", my_lock);
	
	printf("list-%s%s,%d,%d,%d,%d,%lld,%lld,%lld\n", res_option, res_lock, num_threads, num_iterations, num_lists, my_ops, my_time, op_time, total_time/((num_iterations*2 + 1)*num_threads));
	
	free(my_list_ele);
	free(my_list);
	free(mutex_time);
	exit(0);
}
