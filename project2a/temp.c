void* runner(void * index){
    // Sortedlist_insert 
    int my_start = *((int *) index);
    int i;

    for(i = my_start; i < my_start + num_iterations; i++){
        if(my_lock == 'n'){
            SortedList_insert(my_list, &my_list_ele[i]);
        }else if(my_lock == 's'){
            while(__sync_lock_test_and_set(&my_spin, 1));
            SortedList_insert(my_list, &my_list_ele[i]);
            __sync_lock_release(&my_spin);
        }else if(my_lock == 'm'){
            pthread_mutex_lock(&my_mutex);
            SortedList_insert(my_list, &my_list_ele[i]);
            pthread_mutex_unlock(&my_mutex);
        }
    }

    // Get length 
    int len = 0;
    if(my_lock == 'n'){
        len = SortedList_length(my_list);
    }else if(my_lock == 's'){
        while(__sync_lock_test_and_set(&my_spin, 1));
        len = SortedList_length(my_list);
        __sync_lock_release(&my_spin);
    }else if(my_lock == 'm'){
        pthread_mutex_lock(&my_mutex);
        len = SortedList_length(my_list);
        pthread_mutex_unlock(&my_mutex);
    }
    if(len < 0){
        fprintf(stderr, "Error: list corruption len incorrect\n");
        char lock_report[2];
        lock_report[0] = my_lock;
        lock_report[1] = 0;
        fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
        exit(2);
    }

    // Looks up and delete
    SortedListElement_t* temp_ele = NULL;
    for(i = my_start; i < my_start+ num_iterations; i++){
        if(my_lock == 'n'){
                temp_ele = SortedList_lookup(my_list, my_list_ele[i].key);
                if(temp_ele == NULL){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);;
                    exit(2);
                }
                if(SortedList_delete(temp_ele)){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
                    exit(2);
                }
            }else if(my_lock == 'm')
            {
                pthread_mutex_lock(&my_mutex);
                temp_ele = SortedList_lookup(my_list, my_list_ele[i].key);
                if(temp_ele == NULL){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
                    exit(2);
                }
                if(SortedList_delete(temp_ele)){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
                    exit(2);
                }
                pthread_mutex_unlock(&my_mutex);
            }
            else if(my_lock == 's')
            {
                while(__sync_lock_test_and_set(&my_spin, 1));
                temp_ele = SortedList_lookup(my_list, my_list_ele[i].key);
                if(temp_ele == NULL){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
                    exit(2);
                }
                if(SortedList_delete(temp_ele)){
                    fprintf(stderr, "Error: list corruption len incorrect\n");
                    char lock_report[2];
                    lock_report[0] = my_lock;
                    lock_report[1] = 0;
                    fprintf(stderr, "yield: %d  lock: %s  threads: %d  iter: %d\n", opt_yield, lock_report, num_threads, num_iterations);
                    exit(2);
                }
                __sync_lock_release(&my_spin);
        }
    }
    return NULL; 
}