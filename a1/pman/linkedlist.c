/*
* File     : linkedlist.c
* Author   : zentut.com
* Purpose  : C Linked List Data Structure
* Copyright: @ zentut.com
* Modified by: Jorge Flores Pinto
*
* This source code was modified and adapted for the uses of Assignment 1 (PMan),
* and consists of part of the original source code found in:
*   http://www.zentut.com/c-tutorial/c-linked-list/#C_Linked_List_Program
*
*/
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  /* Primitive System Data Types */
#include <stdlib.h>


typedef struct process {
    pid_t pid;
    char *name;
    char *directory;
    char status;
    struct process *next;
} process;

char active = 'a';
char stopped = 's';
char terminated = 't';

typedef void (*callback)(process* process);


/*
    create a new process node
    initialize the data and next field

    return the newly created node
*/
process* create_process(pid_t pid, char *name, char *directory, char status, process *next) {
    process* new_p = (process*)malloc(sizeof(process));
    if(new_p == NULL) {
        printf("Error creating a new process node.\n");
        exit(1);
    }
    new_p->pid = pid;
    new_p->name = strdup(name);
    new_p->directory = strdup(directory);
    new_p->status = status;
    new_p->next = next;

    return new_p;
}

/*
    add a new process node at the beginning of the list
*/
process* prepend(process* head, pid_t pid, char *name, char *directory, char status) {
    process* new_process = create_process(pid, name, directory, status, head);
    head = new_process;
    return head;
}
process *prepend_process(process *head, process *newp) {
    newp->next = head;
    head  = newp;
    return head;
}

/*
    add a new process node at the end of the list
*/
process* append(process* head, pid_t pid, char *name, char *directory, char status) {
    if(head == NULL)
        return create_process(pid, name, directory, status, NULL);
    /* go to the last node */
    process *cursor = head;
    while(cursor->next != NULL)
        cursor = cursor->next;

    /* create a new process node */
    process* new_process =  create_process(pid, name, directory, status, NULL);
    cursor->next = new_process;

    return head;
}
process* append_process(process *head, process *p) {
    if (head == NULL) {
        head = p;
        return head;
    }
    else {
        process *cursor = head;
        while(cursor->next != NULL)
            cursor = cursor->next;

        cursor->next = p;
        return head;
    }
}

/*
    insert a new process node after the prev process node
*/
process* insert_after(process *head, pid_t pid, char *name, char *directory, char status, process* prev) {
    if(head == NULL || prev == NULL)
        return NULL;
    /* find the prev process node, starting from the first process node*/
    process *cursor = head;
    while(cursor != prev)
        cursor = cursor->next;

    if(cursor != NULL) {
        process* new_process = create_process(pid, name, directory, status, cursor->next);
        cursor->next = new_process;
        return head;
    }
    else
    {
        return NULL;
    }
}

/*
    insert a new process node before the nxt process node
*/
process* insert_before(process *head, pid_t pid, char *name, char *directory, char status, process* nxt) {
    if(nxt == NULL || head == NULL)
        return NULL;

    if(head == nxt) {
        //process* prepend(process* head, pid_t pid, char *name, char *directory)
        head = prepend(head, pid, name, directory, status);
        return head;
    }

    /* find the prev process node, starting from the first process node*/
    process *cursor = head;
    while(cursor != NULL) {
        if(cursor->next == nxt)
            break;
        cursor = cursor->next;
    }

    if(cursor != NULL) {
        process* new_process = create_process(pid, name, directory, status, cursor->next);
        cursor->next = new_process;
        return head;
    }
    else {
        return NULL;
    }
}

/*
    traverse the linked list
*/
void traverse(process* head, callback f) {
    //printf("inside traverse\n");
    process* cursor = head;
    while(cursor != NULL) {
        f(cursor);
        cursor = cursor->next;
    }
}
/*
    remove process node from the front of list
*/
process* remove_front(process* head) {
    //printf("in remove front\n");
    if(head == NULL)
        return NULL;

    process *front = head;
    if (front == head) {
        head = head->next;
        free(front);
        return head;
    }

    head = head->next;
    front->next = NULL;
    /* is this the last process node in the list */
    if(front == head)
        head = NULL;
    free(front);
    return head;
}

/*
    remove process node from the back of the list
*/
process* remove_back(process* head) {
    if(head == NULL)
        return NULL;

    process *cursor = head;
    process *back = NULL;
    while(cursor->next != NULL) {
        back = cursor;
        cursor = cursor->next;
    }

    if(back != NULL)
        back->next = NULL;

    /* if this is the last process node in the list*/
    if(cursor == head)
        head = NULL;

    free(cursor);

    return head;
}

/*
    remove a process node from the list
*/
process* remove_any(process* head, process* nd) {
    if(nd == NULL)
        return NULL;
    /* if the process node is the first process node */
    if(nd == head)
        return remove_front(head);

    /* if the process node is the last process node */
    if(nd->next == NULL)
        return remove_back(head);

    /* if the process node is in the middle */
    process* cursor = head;
    while(cursor != NULL) {
        if(cursor->next == nd)
            break;
        cursor = cursor->next;
    }

    if(cursor != NULL) {
        process* tmp = cursor->next;
        cursor->next = tmp->next;
        tmp->next = NULL;
        free(tmp);
    }
    return head;

}
/*
    display a process node
*/
void display_list(process* p) {
    if(p != NULL) {
        //printf("%d\n", p->pid);
        //printf("%s\n", p->name);
        //printf("%s\n", p->directory);
        if (p->status == active)
            printf("%d: %s <active>\n", p->pid, p->directory);
        else if (p->status == stopped)
            printf("%d: %s <stopped>\n", p->pid, p->directory);
        else if (p->status == terminated) {
            p = p->next;
        }

    }
    else {
        printf("No background processes found.\n");
    }
}

/*
    Search for a specific process node with pid

    return the first matched process node that stores the pid,
    otherwise return NULL
*/
process* search(process* head, pid_t pid) {
    process *cursor = head;
    while(cursor != NULL) {
        if(cursor->pid == pid)
            return cursor;
        cursor = cursor->next;
    }
    return NULL;
}

/*
    remove all element of the list
*/
void dispose(process *head) {
    process *cursor, *tmp;

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
/*
    return the number of elements in the list
*/
int count(process *head) {
    process *cursor = head;
    int c = 0;
    while(cursor != NULL) {
        if (cursor->status != terminated) {
            c++;
        }
        cursor = cursor->next;
    }
    return c;
}
