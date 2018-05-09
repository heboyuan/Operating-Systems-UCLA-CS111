#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include "SortedList.h"

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
	if(list == NULL || list->key != NULL || element == NULL){
		return;
	}
	SortedListElement_t* my_next = list->next;
	while(strcmp(element->key, my_next->key) > 0 && my_next->key != NULL){
		if(opt_yield & INSERT_YIELD){
			sched_yield();
		}
		my_next = my_next->next;
	}
	if(opt_yield & INSERT_YIELD){
		sched_yield();
	}

	element->next = my_next;
	element->prev = my_next->prev;
	my_next->prev->next = element;
	element->next->prev = element;
}

int SortedList_delete( SortedListElement_t *element){
	if(element == NULL || element->next->prev != element || element->prev->next != element){
		return 1;
	}
	if(opt_yield & DELETE_YIELD){
		sched_yield();
	}
	element->next->prev = element->prev;
	element->prev->next = element->next;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
	if(list == NULL || list->key != NULL){
		return NULL;
	}
	SortedListElement_t* my_next = list->next;
	while(1){
		if(my_next->key == NULL){
			return NULL;
		}else if(strcmp(my_next->key, key) == 0){
			return my_next;
		}else{
			if(opt_yield & LOOKUP_YIELD){
				sched_yield();
			}
			my_next = my_next->next;
		}
	}
}

int SortedList_length(SortedList_t *list){
	if(list == NULL || list->key != NULL){
		return -1;
	}

	SortedListElement_t* my_next = list->next;
	int len = 0;
	while(my_next != list){
		if (my_next->next->prev != my_next || my_next->prev->next != my_next){
			return -1;
		}else{
			len++;
			if(opt_yield & LOOKUP_YIELD){
				sched_yield();
			}
			my_next = my_next->next;
		}
	}
	return len;
}