//
//  main.c
//  PMan
//
//  Created by Fernando Flores on 2018-09-20.
//  Copyright © 2018 csc360. All rights reserved.
//

#include <stdio.h>
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "linkedlist.h"
#include "bg.h"

#define NUMBER_OF_TOKENS 10 //Max number if input tokens
#define INPUT_LENGTH 100 //Max length of input
#define BG 0
#define BGLIST 1
#define BGKILL 2
#define BGSTOP 3
#define BGSTART 4
#define PSTAT 5
#define EXIT 6

//This function tokenize(char *, char**) constructs
//an array of tokens/arguments, and returns the number
//of tokens/arguments in the input.
int tokenize(char *input, char **tokens) {
    int num_tokens = 0;
    char *token;
    token = strtok(input, " ");

    while(token != NULL) {
        tokens[num_tokens] = token;
        num_tokens++;
        token = strtok(NULL, " ");
    }
    //tokens[num_tokens] = NULL;

    return num_tokens;
}
void get_arguments(char ** args, char **tokens, int num_tokens) {
    int i;
    //char *args[num_tokens];
    for (i = 1; i < num_tokens; i++) {
        args[i - 1] = tokens[i];
    }
    args[i] = NULL;
    //return args;
}
int set_flag(char *cmd) {
    if (strcmp(cmd, "bg") == 0) {
        return BG;
    }
    else if (strcmp(cmd, "bglist") == 0) {
        return BGLIST;
    }
    else if (strcmp(cmd, "bgkill") == 0) {
        return BGKILL;
    }
    else if (strcmp(cmd, "bgstop") == 0) {
        return BGSTOP;
    }
    else if (strcmp(cmd, "bgstart") == 0) {
        return BGSTART;
    }
    else if (strcmp(cmd, "pstat") == 0) {
        return PSTAT;
    }
    else if (strcmp(cmd, "exit") == 0) {
        return EXIT;
    }
    else {
        return -1;
    }
}
void track_process_status(process *head, int childpid) {
    int status, pid = 1;
    process *pr;
    while(pid > 0) {
        if (childpid == -1) {
            pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED);
            pr = search(head, pid);
        }
        else {
            pid = waitpid(childpid, &status, WNOHANG|WUNTRACED|WCONTINUED);
            pr = search(head, childpid);
        }
        if (pid > 0) {
            //printf("%d: Status changed %d\n", pid, status);
            if (WIFSTOPPED(status)) {
                //printf("Stopping job %d\n", pid);
                pr->status = stopped;
                //return head;
            }
            if (WIFCONTINUED(status)) {
                //printf("Resuming job %d\n", pid);
                pr->status = active;
                //return head;
            }
            if (WIFEXITED(status)) {
                printf("Terminating job %d\n", pid);
                pr = search(head, pid);
                pr->status = terminated;
                head = remove_any(head, pr);
            }
            if (WIFSIGNALED(status)) {
                printf("Job %d terminated\n", pid);
                pr = search(head, pid);
                pr->status = terminated;
                head = remove_any(head, pr);
            }
        }
    }
}
int main() {

    char input[100];
    char *tokens[10];
    char *proc_directory = "/proc";
    char cont = 'c';
    process *head = NULL;
    //process *temp;
    //int loop_count = 0;
    callback disp = display_list;

    //TODO 4: comments, edits, format.
    //TODO 5: Readme and makefile.
    for(;;) {
        //if (loop_count > 10) {
        //    break;
        //}
        track_process_status(head, -1);

        //loop_count++;
        //printf("loop_count: %d\n", loop_count);
        fprintf(stdout, "PMan:> ");
        fflush(stdout);
        fgets(input, INPUT_LENGTH, stdin);
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        int num_tokens = tokenize(input, tokens);

        if (num_tokens > NUMBER_OF_TOKENS) {
            fprintf(stderr, "Error: Input has too many arguments.\n");
            continue;
        }

        char *arguments[num_tokens];
        get_arguments(arguments, tokens, num_tokens);

        //Print token array
        /*int i;
        for (i = 0; i < num_tokens; i++) {
            printf("%d: %s\n", i, arguments[i]);
        }*/

        int cmd_flag = set_flag(tokens[0]);
        int pid;
        process *p;
        //printf("cmd_flag: %d\n", cmd_flag);

        switch(cmd_flag) {
            case BG:
                //track_process_status(head, -1);
                if (num_tokens > 1) {
                    //printf("bg command entered.\n");
                    char *path = realpath(arguments[0], NULL);
                    if(path == NULL) {
                        printf("Error: Cannot find file with name: %s\n", arguments[0]);
                        break;
                    }
                    head = execute_bg_process(arguments, arguments[0], path, head, p, num_tokens - 1);
                }
                else {
                    printf("bg: program arguments required\n");
                    printf("bg usage: bg <program> <arg1> <arg2> ...\n");
                }
                break;
            case BGLIST:
                //track_process_status(head, -1);
                //printf("bglist command entered.\n");
                if (head == NULL) {
                    printf("No processes currently active.\n");
                }
                else {
                    traverse(head, disp);
                    int c = count(head);
                    printf("Total background jobs: %d\n", c);
                }
                //track_process_status(head, -1);
                break;
            case BGKILL:
                //track_process_status(head, -1);
                if (num_tokens > 1) {
                    //printf("bgkill command entered.\n");
                    if (head == NULL) {
                        printf("Error: No background processes active.\n");
                        break;
                    }
                    pid = atoi(arguments[0]);
                    p = search(head, pid);
                    if (p == NULL) {
                        printf("Error: Process %d does not exist.\n", pid);
                        break;
                    }
                    kill_process(pid);
                    //head = remove_any(head, p);
                    break;
                }
                else {
                    printf("bgkill usage: bgkill <pid>\n");
                    break;
                }
            case BGSTOP:
                //track_process_status(head, -1);
                if (num_tokens > 1) {
                    //printf("bgstop command entered.\n");
                    if (head == NULL) {
                        printf("Error: No background processes active.\n");
                        break;
                    }
                    pid = atoi(arguments[0]);
                    p = search(head, pid);
                    if (p == NULL) {
                        printf("Error: Process %d does not exist.\n", pid);
                        break;
                    }
                    stop_process(pid);
                    //track_process_status(head, pid);

                    //p->status = stopped;
                    break;
                }
                else {
                    printf("bgstop usage: bgstop <pid>\n");
                    break;
                }
            case BGSTART:
                //track_process_status(head, -1);
                if (num_tokens > 1) {
                    //printf("bgstart command entered.\n");
                    if (head == NULL) {
                        printf("Error: No background processes active.\n");
                        break;
                    }
                    pid = atoi(arguments[0]);
                    p = search(head, pid);
                    if (p == NULL) {
                        printf("Error: Process %d does not exist.\n", pid);
                        break;
                    }
                    continue_process(pid);
                    //track_process_status(head, pid);
                    //p->status = active;
                    break;
                }
                else {
                    printf("bgstart usage: bgstart <pid>\n");
                    break;
                }
            case PSTAT:
                printf("pstat command entered.\n");
                pid = atoi(arguments[0]);
                p = search(head, pid);
                if (p == NULL) {
                    printf("Error: process %d does not exist.\n", pid);
                    break;
                }
                process_status(arguments[0]);
                break;
            case EXIT:
                if (head != NULL) {
                    printf("Do you want to terminate active child processes (y/n)?\n");
                    setbuf(stdin, NULL);  // clear the buffer. You could try to remove this line to see the result
                	scanf("%c", &cont);
                    if (cont == 'y') {
                        printf("Killing active processes...\n");
                        while (head != NULL) {
                            kill_process(head->pid);
                            head = remove_front(head);
                        }
                    }
                }
                printf("Exiting program...\n");
                exit(0);
            default: //command not found
                printf("%s: command not found\n", tokens[0]);
        }
        track_process_status(head, -1);
    }
}
