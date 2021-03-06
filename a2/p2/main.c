/*
 * main.c
 * ACS
 *
 * Name: Jorge Fernando Flores Pinto
 * ID: V00880059
 * CSC360 – Fall 2018
 */

#include <stdio.h>
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include "customer_queue.h"

/***************************** Constant values *******************************/

#define NUM_CLERKS 4
#define BUSINESS_CLASS 1
#define ECONOMY_CLASS 0
#define BUSY 1
#define IDLE 0

/******************** Struct definition and functions ***********************/

typedef struct clerk {
    int id;
    int status;
    struct customer *cust;
} clerk;

clerk* create_clerk(int id, customer *cust) {
    clerk* new_c = (clerk*)malloc(sizeof(clerk));
    if(new_c == NULL) {
        printf("Error creating a new clerk.\n");
        exit(1);
    }
    new_c->id = id;
    new_c->cust = cust;
    new_c->status = IDLE;

    return new_c;
}
void initialize_clerks(clerk *clerks[]) {
    int i;
    for(i = 0; i < NUM_CLERKS; i++) {
        clerks[i] = create_clerk(i + 1, NULL);
    }
}

/***************************** Global Variables ******************************/

pthread_cond_t econ_convar;
pthread_cond_t bus_convar;
pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;
customer *businessq = NULL;
customer *economyq = NULL;
clerk *clerks[NUM_CLERKS];
struct timeval start, current;
float waiting_total, business_waiting, economy_waiting;
int business_count, economy_count, customer_count;
int busq_length, econq_length;
callback disp = display_list;

/***************************** Helper functions ******************************/

float get_time() {
    gettimeofday(&current, NULL);
    //tv-usec returns time in microseconds, so conversion is needed
    long current_time = (current.tv_sec * 1000000.0) + current.tv_usec;
    long start_time = (start.tv_sec * 1000000.0) + start.tv_usec;
    //we convert back to seconds...
    return (float) (current_time - start_time) / 1000000.0;
}
void time_estimates_ACS() {
    printf("\n");
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n",
        (business_waiting + economy_waiting) / customer_count);

    if (business_count > 0) {
        printf("The total number of business-class customers was %d.\n", business_count);
        printf("The average waiting time for all business-class customers is: %.2f seconds. \n",
            business_waiting / business_count);
    }
    else {
        printf("There were no business-class customers this time.\n");
        printf("The average waiting time for all business-class customers is: %.2f seconds. \n",
            0.00);
    }
    if (economy_count > 0) {
        printf("The total number of economy-class customers was %d.\n", economy_count);
        printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",
            economy_waiting / economy_count);
    }
    else {
        printf("There were no economy-class customers this time.\n");
        printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",
            0.00);
    }
}

/************************* Input validation functions ************************/

void validate_args(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: Not enough arguments\nUsage: ACS <customer_file>\n");
        exit(1);
    }
    else if (argc > 2) {
        fprintf(stderr, "Error: Too many arguments\nUsage: ACS <customer_file>\n");
        exit(1);
    }
}
bool validate_customer_values(int id, int class, int arrival, int service) {
    bool is_invalid = false;
    if (id < 0) {
        fprintf(stderr, "Error: Invalid input; id (%d) is negative\n", id);
        is_invalid = true;
    }
    if (class < 0) {
        fprintf(stderr, "Error: Invalid input; class (%d) is negative\n", class);
        is_invalid = true;
    }
    if (arrival < 0) {
        fprintf(stderr, "Error: Invalid input; arrival_time (%d) is negative\n",
            arrival);
        is_invalid = true;
    }
    if (service < 0) {
        fprintf(stderr, "Error: Invalid input; service_time (%d) is negative\n",
            service);
        is_invalid = true;
    }
    return is_invalid;
}

/***** Mutex and condition variable initialization/destruction functions *****/

void initialize_mutexes_convars() {
	if (pthread_mutex_init(&mutex1, NULL) != 0) {
		printf("Error: failed to initialize mutex1\n");
		exit(1);
	}
    if (pthread_mutex_init(&mutex2, NULL) != 0) {
		printf("Error: failed to initialize mutex2\n");
		exit(1);
	}
    if (pthread_mutex_init(&mutex3, NULL) != 0) {
		printf("Error: failed to initialize mutex3\n");
		exit(1);
	}
	if (pthread_cond_init(&econ_convar, NULL) != 0) {
		printf("Error: failed to initialize conditional variable econ_convar\n");
		exit(1);
	}
    if (pthread_cond_init(&bus_convar, NULL) != 0) {
		printf("Error: failed to initialize conditional variable bus_convar\n");
		exit(1);
	}
}
void destroy_mutexes_convars() {
	if (pthread_mutex_destroy(&mutex1) != 0) {
		printf("Error: failed to destroy mutex1\n");
		exit(1);
	}
    if (pthread_mutex_destroy(&mutex2) != 0) {
		printf("Error: failed to destroy mutex2\n");
		exit(1);
	}
    if (pthread_mutex_destroy(&mutex3) != 0) {
		printf("Error: failed to destroy mutex3\n");
		exit(1);
	}
	if (pthread_cond_destroy(&econ_convar) != 0) {
		printf("Error: failed to destroy econ_convar\n");
		exit(1);
	}
    if (pthread_cond_destroy(&bus_convar) != 0) {
		printf("Error: failed to destroy bus_convar\n");
		exit(1);
	}
}

/************* Customer data structure initialization functions **************/

int get_customer_count(FILE *cust_fp, char *filename) {
    int temp_count;
    fscanf(cust_fp, "%d", &temp_count);
    if (temp_count < 0) {
        fprintf(stderr, "Error: Invalid input; number of customers (%d) is negative\n",
            temp_count);
        fclose(cust_fp);
        exit(1);
    }
    return temp_count;
}
void get_customers(FILE *cust_fp, customer *customers[], int customer_count) {
    char *format = "%d:%d,%d,%d";
    int i;
    bool input_invalid = false;
    customer *curr;
    for(i = 0; i < customer_count; i++) {
        int id, class, arrival, service;
        fscanf(cust_fp, format, &id, &class, &arrival, &service);
        input_invalid = validate_customer_values(id, class, arrival, service);
        if (input_invalid) {
            fclose(cust_fp);
            exit(1);
        }
        curr = create_customer(id, class, arrival, service, NULL);
        if (curr->class == BUSINESS_CLASS) {
            businessq = enqueue(businessq, curr->id, curr->class, curr->arrival_time, curr->service_time);
            business_count++;
        }
        else if (curr->class == ECONOMY_CLASS) {
            economyq = enqueue(economyq, curr->id, curr->class, curr->arrival_time, curr->service_time);
            economy_count++;
        }
        customers[i] = curr;
    }
}

/********************* Customer service cycle functions **********************/

void request_clerks_for_service(customer *c, float temp_start) {
    int i;
    float temp_wait;
    pthread_mutex_lock(&mutex1);
    clerk *curr_clerk;
    for (i = 0; i < NUM_CLERKS; i++) {
        curr_clerk = clerks[i];
        if (curr_clerk->status == IDLE) {
            curr_clerk->status = BUSY;
            curr_clerk->cust = c;
            c->clerk = curr_clerk;
            printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n",
                get_time(), c->id, curr_clerk->id);
            if (c->class == BUSINESS_CLASS) {
                pthread_mutex_lock(&mutex2);
                businessq = dequeue(businessq);
                busq_length--;
                temp_wait = get_time();
                pthread_mutex_lock(&mutex3);
                business_waiting += (temp_wait - temp_start);
                pthread_mutex_unlock(&mutex3);
                pthread_mutex_unlock(&mutex2);
            }
            else if (c->class == ECONOMY_CLASS) {
                pthread_mutex_lock(&mutex2);
                economyq = dequeue(economyq);
                econq_length--;
                temp_wait = get_time();
                pthread_mutex_lock(&mutex3);
                economy_waiting += (temp_wait - temp_start);
                pthread_mutex_unlock(&mutex3);
                pthread_mutex_unlock(&mutex2);

            }
            /*
            printf("Business queue: \n");
            traverse(businessq, disp);
            printf("\n");
            printf("Economy queue: \n");
            traverse(economyq, disp);
            printf("\n");
            */
            pthread_mutex_unlock(&mutex1);
            return;
        }
    }
    /*
     * If we are here, that means all clerks are busy and the customer will have
     * to wait for the next available clerk.
     */

    /*
     * Customers will wait for a signal from the next available clerk. All
     * customers in each queue will receive a signal, but only the customer
     * that is the head of the queue signaled will be allowed to proceed. The
     * other customers in the queue will wait again for the next signal.
     */
    int temp_cid, temp_qid;
    temp_cid = c->id;
    if (c->class == BUSINESS_CLASS) {
        do {
            pthread_cond_wait(&bus_convar, &mutex1);
            pthread_mutex_lock(&mutex2);
            temp_qid = businessq->id;
            pthread_mutex_unlock(&mutex2);
            temp_wait = get_time();
        }
        while (temp_cid != temp_qid);
        pthread_mutex_lock(&mutex3);
        business_waiting += (temp_wait - temp_start);
        pthread_mutex_unlock(&mutex3);

    }
    else if (c->class == ECONOMY_CLASS) {
        do {
            pthread_cond_wait(&econ_convar, &mutex1);
            pthread_mutex_lock(&mutex2);
            temp_qid = economyq->id;
            pthread_mutex_unlock(&mutex2);
            temp_wait = get_time();
        }
        while (temp_cid != temp_qid);
        pthread_mutex_lock(&mutex3);
        economy_waiting += (temp_wait - temp_start);
        pthread_mutex_unlock(&mutex3);
    }
    for (i = 0; i < NUM_CLERKS; i++) {
        curr_clerk = clerks[i];
        if (curr_clerk->status == IDLE) {
            break;
        }
    }

    curr_clerk->cust = c;
    curr_clerk->status = BUSY;
    c->clerk = curr_clerk;
    printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n",
        get_time(), c->id, curr_clerk->id);
    pthread_mutex_unlock(&mutex1);

    pthread_mutex_lock(&mutex2);
    if (c->class == BUSINESS_CLASS) {
        businessq = dequeue(businessq);
        busq_length--;
    }
    else if (c->class == ECONOMY_CLASS) {
        economyq = dequeue(economyq);
        econq_length--;
    }

    /*
    printf("Business queue: \n");
    traverse(businessq, disp);
    printf("\n");
    printf("Economy queue: \n");
    traverse(economyq, disp);
    printf("\n");
    */
    pthread_mutex_unlock(&mutex2);
}
void finish_service(customer *c) {
    pthread_mutex_lock(&mutex1);
    c->clerk->status = IDLE;
    if (count(businessq) > 0)
        pthread_cond_signal(&bus_convar);
    else
        pthread_cond_signal(&econ_convar);
    pthread_mutex_unlock(&mutex1);
}
void* customer_cycle(void *ptr) {
    customer *c = (customer*) ptr;
    float temp_start;
    temp_start = get_time();
    request_clerks_for_service(c, temp_start);
    usleep(c->service_time * 100000);
    printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n",
        get_time(), c->id, c->clerk->id);
    finish_service(c);

    return (void*)0;
}
int create_cust_threads(customer *customers[], int cust_count, pthread_t threads[]) {
    int i, err;
    customer *curr;

    for (i = 0; i < cust_count; i++) {
        curr = customers[i];
        usleep(curr->arrival_time * 100000);
        printf("A customer arrives: customer ID %2d. \n", curr->id);
        if (curr->class == BUSINESS_CLASS) {
            pthread_mutex_lock(&mutex2);
            busq_length++;
            printf("Customer ID %d enters the business queue: the queue ID %1d, and length of the queue %2d. \n",
                curr->id, curr->class, busq_length);
            pthread_mutex_unlock(&mutex2);
        }
        else if (curr->class == ECONOMY_CLASS) {
            pthread_mutex_lock(&mutex2);
            econq_length++;
            printf("Customer ID %d enters the economy queue: the queue ID %1d, and length of the queue %2d. \n",
                curr->id, curr->class, econq_length);
            pthread_mutex_unlock(&mutex2);
        }
        if ((err = pthread_create(&threads[i], NULL, customer_cycle, curr))) {
            fprintf(stderr, "error: pthread_create, err: %d\n", err);
            return EXIT_FAILURE;
        }

    }
    return 0;
}

/*************************** Main function **********************************/

int main(int argc, char *argv[]) {
    printf("*** Airline Check-in System ***\n");
    //Initializing constants
    customer_count = 0;
    business_count = 0;
    economy_count = 0;
    busq_length = 0;
    econq_length = 0;

    //Check for correct number of arguments
    validate_args(argc, argv);
    initialize_mutexes_convars();

    FILE *cust_fp;
    cust_fp = fopen(argv[1], "r");
    if (cust_fp == NULL) {
        printf("Error: Unable to open customer file with name: %s\n", argv[1]);
        exit(1);
    }

    customer_count = get_customer_count(cust_fp, argv[1]);
    printf("Number of customers: %d\n\n", customer_count);
    customer *customer_arr[customer_count];
    get_customers(cust_fp, customer_arr, customer_count);
    fclose(cust_fp);

    initialize_clerks(clerks);

    pthread_t threads[customer_count];
    gettimeofday(&start, NULL);
    create_cust_threads(customer_arr, customer_count, threads);

    // The main thread waits for all other threads to complete...
    int i;
	for (i = 0; i < customer_count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
			printf("Error: failed to join pthread.\n");
			exit(1);
		}
	}

    destroy_mutexes_convars();
    time_estimates_ACS();
	pthread_exit(NULL);
    return EXIT_SUCCESS;
}
