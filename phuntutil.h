#ifndef PHUNTUTIL_H
#define PHUNTUTIL_H

/* Tokenize a string, str, to tokens, and them in argv */
void tokenize(char * str, char **argv);

/* Log a string to a file pointed by l */
void log_entry(FILE * l, const char * entry);

/* Check if the process was suspended */
int issusp(pid_t pid);

/** 
 * Check if the process exceed the limit 
 * If the process exceeds the limit, 1 is returned;  
 * otherwise 0 is returned. (0 is returned on error as well)
 */
int exceed_mem(pid_t pid, int limit);

/**
 * Check if the process run by the indicated user
 * If matched, 1 is returned, 0 otherwise 
 */
int match_user(pid_t pid, char * usrname);

/**
 * Check if the process run in the environment indicated
 * by the path. If the path is the subset of the path of 
 * the process, 1 is returned, 0 otherwise
 */
int has_path(pid_t pid, char * path);

#endif
