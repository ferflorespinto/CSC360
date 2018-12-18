//
//  bg.c
//  PMan
//
//  Created by Fernando Flores on 2018-09-20.
//  Copyright © 2018 csc360. All rights reserved.
//
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <sys/wait.h>   /* Wait for Process Termination */
#include <stdlib.h>     /* General Utilities */
#include <string.h>
#include <limits.h>
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT
#include "linkedlist.h"

process *execute_bg_process(char **tokens, char *pr_name, char *pr_path,
    process *head, process *p, int num_tokens) {

    pid_t childpid;          // The ID of child process
	int status;
    //char *path = realpath(pr_name, NULL);
    /*if(pr_path == NULL) {
        printf("Cannot find file with name: [%s]\n", pr_name);
    } else {
        printf("path: %s\n", pr_path);
    }*/

    tokens[num_tokens] = NULL;

    childpid = fork();

    if(childpid == 0) {
        //printf("Child!\n");
        if (execvp(pr_path, tokens) < 0) {
            perror("Error on executing.\n");
            exit(1);
        }
    }
    else {
        p = create_process(childpid, pr_name, pr_path, active, NULL);
        if (head == NULL) {
            head = p;
            //traverse(head, disp);
        }
        else {
            append_process(head, p);
        }
        sleep(1);
        return head;
    }
    printf("Failed to fork process.\n");
    return NULL;
}
void kill_process(pid_t pid) {
    if (kill(pid, SIGTERM) == 0) {
        //printf("Terminated job with pid: %d\n", pid);
    }
    else {
        printf("Error: Failed to terminate job with pid: %d\n", pid);
    }
    sleep(1);
}
void stop_process(pid_t pid) {
    if (kill(pid, SIGSTOP) == 0) {
        printf("Stopped job with pid: %d\n", pid);
    }
    else {
        printf("Error: Failed to stop job with pid: %d\n", pid);
    }
    sleep(1);
}
void continue_process(pid_t pid) {
    if (kill(pid, SIGCONT) == 0) {
        printf("Resumed job with pid: %d\n", pid);
    }
    else {
        printf("Error: Failed to resume job with pid: %d\n", pid);
    }
    sleep(1);
}
void process_status(char *pid) {
    char proc_stat_directory[PATH_MAX], proc_status_directory[PATH_MAX], comm[MAX_INPUT];
    FILE *stat_fp;
    FILE *status_fp;
    char state;
    long unsigned int utime, stime;
    long int rss;
    long int voluntary_ctxt_switches, nonvoluntary_ctxt_switches;

    char status_line[42][MAX_INPUT];

    /*
    As per proc/(pid)/stat format, and using identifiers for scanf, we include
    an asterisk (*) before all fields that should be ignored. The order of
    fields can be found in proc/(pid)/stat file specification. For
    the purposes of this project, we only need:
      - (2) comm: %s
      - (3) state: %c
      - (14) utime: %lu
      - (15) stime: %lu
      - (24) rss: %ld
    */
    char *format1 = "%*d %s %c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu";
                //   (1) (2)(3)(4) (5) (6) (7) (8) (9) (10) (11) (12) (13)
    char *format2 = "%lu %lu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %*lu %ld";
                //  (14) (15) (16) (17) (18) (19) (20) (21)  (22) (23) (24)
    char *format3 = "%*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu";
                //  (25)  (26) (27) (28) (29) (30) (31) (32) (33) (34) (35) (36)
    char *format4 = "%*lu %*d %*d %*u %*u %*llu %*lu %*ld";
                //  (37) (38)(39)(40)(41) (42)  (43) (44)

    char proc_format[strlen(format1) + strlen(format2) + strlen(format3) + strlen(format4) + 1];
    sprintf(proc_format, "%s %s %s %s", format1, format2, format3, format4);

    sprintf(proc_stat_directory, "/proc/%s/stat", pid);
    printf("proc_stat_directory: %s\n", proc_stat_directory);
    sprintf(proc_status_directory, "/proc/%s/status", pid);
    printf("proc_status_directory: %s\n", proc_status_directory);

    stat_fp = fopen(proc_stat_directory, "r");
    if (stat_fp == NULL) {
        printf("Error in opening stat file for job %s.\n", pid);
        return;
    }
    fscanf(stat_fp, proc_format, comm, &state, &utime, &stime, &rss);
    printf("comm: %s\n", comm);
    printf("state: %c\n", state);
    printf("utime: %fs\n", (float)utime / (float)sysconf(_SC_CLK_TCK));
    printf("stime: %fs\n", (float)stime / (float)sysconf(_SC_CLK_TCK));
    printf("rss: %ld pages\n", rss);

    status_fp = fopen(proc_status_directory, "r");
    if (status_fp == NULL) {
        printf("Error in opening status file for job %s.\n", pid);
        return;
    }
    int i = 0;
    while (fgets(status_line[i], MAX_INPUT, status_fp) != NULL) {
        i++;
    }
    //printf("%s\n", status_line[40]);
    //printf("%s\n", status_line[41]);
    sscanf(status_line[40], "voluntary_ctxt_switches: %ld",
        &voluntary_ctxt_switches);
    sscanf(status_line[41], "nonvoluntary_ctxt_switches: %ld",
        &nonvoluntary_ctxt_switches);

    printf("voluntary_ctxt_switches: %ld\n", voluntary_ctxt_switches);
    printf("nonvoluntary_ctxt_switches: %ld\n", nonvoluntary_ctxt_switches);
    //for(i=0; fgets( status[i], MaxTextLen, statusFile) != NULL; i++);

}
