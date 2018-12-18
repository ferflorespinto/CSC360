#ifndef BG_H_INCLUDED
#define BG_H_INCLUDED
/* ^^ the include guards */

/* Prototypes for the functions */
process *execute_bg_process(char **tokens, char *pr_name, char *pr_path,
    process *head, process *p, int num_tokens);
void kill_process(pid_t pid);
void stop_process(pid_t pid);
void continue_process(pid_t pid);
void process_status(char *pid);

#endif
