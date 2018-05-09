#include "SortedList.h"
#include <sched.h>
#include <string.h>
#include <stdio.h>


void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
    if (list == NULL || element == NULL || list->key != NULL)
        return;
    SortedListElement_t* next = list->next;
    while(next->key != NULL && strcmp(element->key, next->key) > 0){
        //printf("Insert finding\n");
        if(opt_yield & INSERT_YIELD)
            sched_yield();
        next = next->next;
    }
    if(opt_yield & INSERT_YIELD)
        sched_yield();
    next = next->prev;
    element->next = next->next;
    element->prev = next;
    next->next = element;
    element->next->prev = element;
}

int SortedList_delete( SortedListElement_t *element){
  if(element == NULL)
    return 1;
  if(element->next->prev == element && element->prev->next == element){
    if(opt_yield & DELETE_YIELD)
      sched_yield();
    element->next->prev = element->prev;
    element->prev->next = element->next;
    return 0;
  } else {
    return 1;
  }
    
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
    if(list == NULL)
        return NULL;
    SortedListElement_t* next = list->next;
    while(next->key != NULL){
        if(strcmp(next->key, key) == 0){
            return next;
        }
        else{
            if(opt_yield & LOOKUP_YIELD)
                sched_yield();
            next = next->next;
        }
    }
    return NULL;
}

int SortedList_length(SortedList_t *list){
    if(list == NULL)
        return -1;
    int length = 0;
    SortedListElement_t* next = list->next;
    while(next != list){
        //printf("List lenght now = %d \n", length);
        if(next->prev->next == next && next->next->prev == next){
            //printf("List length condition met\n");
            ++length;
            if(opt_yield & LOOKUP_YIELD)
                sched_yield();
            next = next->next;
        }
        else return -1;
    }
    return length;
}