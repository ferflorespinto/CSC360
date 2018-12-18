/*
* File     : customer_queue.c
* Author   : zentut.com
* Purpose  : C Linked List Data Structure
* Copyright: @ zentut.com
* Modified by: Jorge Fernando Flores Pinto
*
* This source code originally defined a linked list. It was modified and adapted
* for the uses of Assignment 2 (ACS), mainly to function as a queue.
* The original source code can be found in:
*   http://www.zentut.com/c-tutorial/c-linked-list/#C_Linked_List_Program
*
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  /* Primitive System Data Types */
#include <stdlib.h>
#include <stdbool.h>

typedef struct customer {
    int id;
    int class;
    int arrival_time;
    int service_time;
    struct clerk *clerk;
    struct customer *next;
} customer;

typedef void (*callback)(customer* data);


/*
    create a new process node
    initialize the data and next field

    return the newly created node
*/
customer* create_customer(int id, int class, int arrival_time, int service_time, customer *next) {
    customer* new_c = (customer*)malloc(sizeof(customer));
    if(new_c == NULL) {
        printf("Error creating a new customer node.\n");
        exit(1);
    }
    new_c->id = id;
    new_c->class = class;
    new_c->arrival_time = arrival_time;
    new_c->service_time = service_time;
    new_c->clerk = NULL;
    new_c->next = next;

    return new_c;
}
/*
    return the number of elements in the list
*/
int count(customer *head) {
    customer *cursor = head;
    int c = 0;
    while(cursor != NULL) {
        c++;
        cursor = cursor->next;
    }
    return c;
}
/*
    add a new customer node at the end of the queue
*/
customer* enqueue(customer* head, int id, int class, int arrival_time, int service_time) {
    if(head == NULL) {
        customer *cst = create_customer(id, class, arrival_time, service_time, NULL);
        return cst;
;
    }
    /* go to the last node */
    customer *cursor = head;
    while(cursor->next != NULL)
        cursor = cursor->next;

    /* create a new process node */
    customer* new_cust =  create_customer(id, class, arrival_time, service_time, NULL);
    cursor->next = new_cust;
    return head;
}
void display_list(customer* c) {
    if (count(c) == 0) {
        printf("Queue empty\n");
    }
    if(c != NULL) {
        printf("%d -> ", c->id);
    }
}
/*
    traverse the queue
*/
void traverse(customer* head, callback f) {
    customer* cursor = head;
    while(cursor != NULL) {
        f(cursor);
        cursor = cursor->next;
    }
}
/*
    remove process node from the front of queue
*/
customer* dequeue(customer* head) {
    if(head == NULL)
        return NULL;

    customer *front = head;
    if (front == head) {
        head = head->next;
        free(front);
        return head;
    }

    head = head->next;
    front->next = NULL;
    /* is this the last process node in the queue */
    if(front == head)
        head = NULL;
    free(front);
    return head;
}

/*
    Search for a specific customer node with id

    return the first matched process node that stores the id,
    otherwise return NULL
*/
customer* search(customer* head, int id) {
    customer *cursor = head;
    while(cursor != NULL) {
        if(cursor->id == id)
            return cursor;
        cursor = cursor->next;
    }
    return NULL;
}

/*
    remove all elements of the queue
*/
void dispose(customer *head) {
    customer *cursor, *tmp;

    if(head != NULL) {
        cursor = head->next;
        head->next = NULL;

        while(cursor != NULL) {
            tmp = cursor->next;
            free(cursor);
            cursor = tmp;
        }
    }
}
bool is_empty(customer *head) {
    if (head == NULL) {
        return true;
    }
    return false;
}
