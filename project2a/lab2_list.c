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
int opt_yield = 0;
char my_lock = 'n';
SortedList_t my_list;
SortedListElement_t* my_list_ele;
int my_spin = 0;
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

//https://www.codeproject.com/Questions/640193/Random-string-in-language-C

void* runner(void* temp){
	int my_start = *((int *) temp);
	int i;

	for(i = my_start; i < my_start + num_iterations; i++){
		if(my_lock == 'n'){
			SortedList_insert(&my_list, &my_list_ele[i]);
		}else if(my_lock == 's'){
			while(__sync_lock_test_and_set(&my_spin, 1));
			SortedList_insert(&my_list, &my_list_ele[i]);
			__sync_lock_release(&my_spin);
		}else if(my_lock == 'm'){
			pthread_mutex_lock(&my_mutex);
			SortedList_insert(&my_list, &my_list_ele[i]);
			pthread_mutex_unlock(&my_mutex);
		}
	}

	int len = 0;
	if(my_lock == 'n'){
		len = SortedList_length(&my_list);
	}else if(my_lock == 's'){
		while(__sync_lock_test_and_set(&my_spin, 1));
		len = SortedList_length(&my_list);
		__sync_lock_release(&my_spin);
	}else if(my_lock == 'm'){
		pthread_mutex_lock(&my_mutex);
		len = SortedList_length(&my_list);
		pthread_mutex_unlock(&my_mutex);
	}
	if(len < 0){
		fprintf(stderr, "Error: list corruption\n");
		free(my_list_ele);
		exit(2);
	}

	SortedListElement_t* temp_ele = NULL;
	for(i = my_start; i < my_start + num_iterations; i++){
		if(my_lock == 'n'){
			if(!(temp_ele = SortedList_lookup(&my_list, my_list_ele[i].key))){
				fprintf(stderr, "Error: list corruption and element disappear\n");
				free(my_list_ele);
				exit(2);
			}
			if(SortedList_delete(temp_ele)){
				fprintf(stderr, "Error: list corruption and cannot delete\n");
				free(my_list_ele);
				exit(2);
			}
		}else if(my_lock == 's'){
			pthread_mutex_lock(&my_mutex);
			if(!(temp_ele = SortedList_lookup(&my_list, my_list_ele[i].key))){
				fprintf(stderr, "Error: list corruption and element disappear\n");
				free(my_list_ele);
				exit(2);
			}
			if(SortedList_delete(temp_ele)){
				fprintf(stderr, "Error: list corruption and cannot delete\n");
				free(my_list_ele);
				exit(2);
			}
			pthread_mutex_unlock(&my_mutex);
		}else if(my_lock == 'm'){
			while(__sync_lock_test_and_set(&my_spin, 1));
			if(!(temp_ele = SortedList_lookup(&my_list, my_list_ele[i].key))){
				fprintf(stderr, "Error: list corruption and element disappear\n");
				free(my_list_ele);
				exit(2);
			}
			if(SortedList_delete(temp_ele)){
				fprintf(stderr, "Error: list corruption and cannot delete\n");
				free(my_list_ele);
				exit(2);
			}
			__sync_lock_release(&my_spin);
		}
	}
	return NULL;
}

void handler(){
	fprintf(stderr, "Error: Segmentation fault\n");
	free(my_list_ele);
	exit(2);
}

int main(int argc, char **argv){
	srand(time(NULL));

	static struct option long_option[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};

	int op;
	while(1){
		op = getopt_long(argc, argv, "i:t:y:s:", long_option, NULL);
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
			default:
				fprintf(stderr, "Error: unrecoginized arguement\n");
				exit(1);
		}
	}

	signal(SIGSEGV, handler);

	my_list.key = NULL;
	my_list.next = &my_list;
	my_list.prev = &my_list;

	my_list_ele = malloc(sizeof(SortedListElement_t) * num_threads * num_iterations);

	int i, j; //, rand_len;

	// static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	// for(i = 0; i < num_threads * num_iterations; i++){
	// 	rand_len = rand() % 10 + 1;
	// 	char* temp_key = malloc(sizeof(char) * (rand_len + 1));
	// 	for(j = 0; j < rand_len; j++){
	// 		temp_key[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	// 	}
	// 	temp_key[rand_len] = 0;
	// 	my_list_ele[i].key = temp_key;
	// }

    for(i = 0; i < total_elements; i++){
        int random_number = rand() % 26; //Bound to a-z 
        char* random_key = malloc(2 * sizeof(char)); // 1 char key + null byte
        random_key[0] = 'a' + random_number; // turn randNumber into character
        random_key[1] = '\0';

        my_list_ele[i].key = random_key;
    }

	int start_loc[num_threads];
	for(i = 0, j = 0; i < num_threads; i++, j += num_iterations){
		start_loc[i] = j;
	}

	struct timespec s_time;
	clock_gettime(CLOCK_MONOTONIC, &s_time);

	pthread_t my_threads[num_threads];

	for(i= 0; i < num_threads; i++){
		if (pthread_create(&my_threads[i], NULL, runner, (void *) &start_loc[i])) {
			fprintf(stderr, "Error: cannot create threads\n");
			free(my_list_ele);
			exit(1);
		}
	}

	for (int i = 0; i < num_threads; i++) {
		if (pthread_join(my_threads[i], NULL)) {
			fprintf(stderr, "Error: cannot join threads\n");
			free(my_list_ele);
			exit(1);
		}
	}

	struct timespec e_time;
	clock_gettime(CLOCK_MONOTONIC, &e_time);
	
	if(SortedList_length(&my_list) != 0){
		fprintf(stderr, "Error: list error\n");
		free(my_list_ele);
		exit(2);
	}

	long long my_time = (e_time.tv_sec - s_time.tv_sec)*1000000000;
	my_time += e_time.tv_nsec;
	my_time -= s_time.tv_nsec;
	int my_ops = num_threads*num_iterations*2;
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

	printf("list-%s%s,%d,%d,1,%d,%lld,%lld\n", res_option, res_lock, num_threads, num_iterations, my_ops, my_time, op_time);
	free(my_list_ele);
	exit(0);
}