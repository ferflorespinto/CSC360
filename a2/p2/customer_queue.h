#ifndef CUSTOMER_QUEUE_H_INCLUDED
#define CUSTOMER_QUEUE_H_INCLUDED

/* Prototypes for the functions */
typedef struct customer {
    int id;
    int class;
    int arrival_time;
    int service_time;
    struct clerk *clerk;
    struct customer *next;
} customer;

typedef void (*callback)(customer* customer);

customer* create_customer(int id, int class, int arrival_time, int service_time, customer *next);
int count(customer *head);
customer* enqueue(customer* head, int id, int class, int arrival_time, int service_time);
void display_list(customer* c);
void traverse(customer* head, callback f);
customer* dequeue(customer* head);
customer* search(customer* head, int id);
void dispose(customer *head);
bool is_empty(customer *head);

#endif
