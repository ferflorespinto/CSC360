#ifndef LINKEDLIST_H_INCLUDED
#define LINKEDLIST_H_INCLUDED
/* ^^ the include guards */

/* Prototypes for the functions */
typedef struct process {
    pid_t pid;
    char *name;
    char *directory;
    char status;
    struct process *next;
} process;

char active;
char stopped;
char terminated;

typedef void (*callback)(process* data);

process* create_process(pid_t pid, char *name, char *directory, char status, process *next);
process* prepend(process* head, pid_t pid, char *name, char *directory, char status);
process *prepend_process(process *head, process *newp);
process* append(process* head, pid_t pid, char *name, char *directory, char status);
process* append_process(process *head, process *p);
process* insert_after(process *head, pid_t pid, char *name, char *directory, char status, process* prev);
process* insert_before(process *head, pid_t pid, char *name, char *directory, char status, process* nxt);
void traverse(process* head, callback f);
process* remove_front(process* head);
process* remove_back(process* head);
process* remove_any(process* head,process* nd);
void display_list(process* p);
process* search(process* head, pid_t pid);
void dispose(process *head);
int count(process *head);

#endif
