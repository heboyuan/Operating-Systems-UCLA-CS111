void* runner(void * index){
    for(int i = *((int*) index); i < *((int*) index)+ num_iterations; i++){
        switch(my_lock){
            case 'n':
            {
                SortedList_insert(my_list, &my_list_ele[i]);
                break;
            }
            case 'm':
            {
                pthread_mutex_lock(&my_mutex);
                SortedList_insert(my_list, &my_list_ele[i]);
                pthread_mutex_unlock(&my_mutex);
                break; 
            }
            case 's': 
            {
                while(__sync_lock_test_and_set(&my_spin, 1));
                SortedList_insert(my_list, &my_list_ele[i]);
                __sync_lock_release(&my_spin);
                break;
            }
        }
    }

    // Get length 
    int list_length = 0; 
    switch(my_lock){
        case 'n':
        {
            list_length = SortedList_length(my_list);
            break;
        }
        case 'm':
        {
            pthread_mutex_lock(&my_mutex);
            list_length = SortedList_length(my_list);
            pthread_mutex_unlock(&my_mutex);
            break; 
        }
        case 's': 
        {
            while(__sync_lock_test_and_set(&my_spin, 1));
            list_length = SortedList_length(my_list);
            __sync_lock_release(&my_spin);
            break;
        }
    }
    if (list_length == -1) {
        fprintf(stderr, "ERROR; failed to get length of list\n");
        exit(2);
    }

    // Looks up and delete
    SortedListElement_t *new = NULL;
    for(int i = *((int*) index); i < *((int*) index)+ num_iterations; i++){
        switch(my_lock){
            case 'n':
            {
                new = SortedList_lookup(my_list, my_list_ele[i].key);
                if(new == NULL){
                    fprintf(stderr, "ERROR; fail to find the element in the list\n");
                    exit(2);
                }
                if(SortedList_delete(new)){
                    fprintf(stderr, "ERROR; fail to delete the element in the list\n");
                    exit(2);
                }
                break;
            }
            case 'm':
            {
                pthread_mutex_lock(&my_mutex);
                new = SortedList_lookup(my_list, my_list_ele[i].key);
                if(new == NULL){
                    fprintf(stderr, "ERROR; fail to find the element in the list\n");
                    exit(2);
                }
                if(SortedList_delete(new)){
                    fprintf(stderr, "ERROR; fail to delete the element in the list\n");
                    exit(2);
                }
                pthread_mutex_unlock(&my_mutex);
                break; 
            }
            case 's': 
            {
                while(__sync_lock_test_and_set(&my_spin, 1));
                new = SortedList_lookup(my_list, my_list_ele[i].key);
                if(new == NULL){
                    fprintf(stderr, "ERROR; fail to find the element in the list\n");
                    exit(2);
                }
                if(SortedList_delete(new)){
                    fprintf(stderr, "ERROR; fail to delete the element in the list\n");
                    exit(2);
                }
                __sync_lock_release(&my_spin);
                break;
            }
        }
    }
    return NULL; 
}