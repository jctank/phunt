#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>

/* Tokenize str into stokens and store them into argv */
void tokenize(char * str, char **argv) {
    while (*str != '\0') {
	while (*str == ' ' || *str == '\t' || *str == '\n') 
	    *str++ = '\0';
	*argv++ = str;
//	printf("%s\n", str);
	while (*str != ' '  && *str != '\0' &&
	       *str != '\t' && *str != '\n')
	    str++; 
    }
    *argv = '\0';
}

/* Log a string to a file pointed by l */
void log_entry(FILE * l, const char * entry) {

    time_t now = time(NULL);
    char * ts = (char *)asctime(localtime(&now));
    fprintf(l, "%.*s %s phunt:\t%s \n", strlen(ts) - 1, ts ,
	    getpwuid(getuid())->pw_name,  entry);
}


/* Check if the process was suspended */
int issusp(pid_t pid) {
    char path[20];
    char cmd[20];
    char state;
    FILE * fd;
    int nfill, pid1;
    
    snprintf(path, sizeof path, "/proc/%d/stat", pid);
    
    fd = fopen(path, "r");
    if (fd == NULL) return 0;
    
    nfill = fscanf(fd, "%d %s %c", &pid1, cmd, &state);
    fclose(fd);

    if (nfill != 3) return 0;
   
    return state == 'T' ? 1 : 0;
}

/* Check if the process exceeds the memory usage in KB*/
int exceed_mem(pid_t pid, int limit) {
  
    int  vmsize, vmrss, share, text, lib, data, dt;
    char path[20];
    int  totMB, nfill;
    FILE * fd;
     
    snprintf(path, sizeof path, "/proc/%d/statm", pid);
    
    fd = fopen(path, "r");
    if (fd == NULL) {
	fprintf(stderr, "pid(%d) not exist\n", pid);
	return -1;
    } 
    
    nfill = fscanf(fd, "%d %d %d %d %d %d %d", &vmsize, &vmrss,
		&share, &text, &lib, &data, &dt);    
    if (nfill != 7)
	return 0;

    long page_sz = sysconf(_SC_PAGESIZE);
    totMB = (vmsize +vmrss +share +text +lib +data +dt) *page_sz /(1024*1024);

    return totMB > limit ? 1 : 0;
}

/* Check if the process run by the indicated user */
int match_user(pid_t pid, char * usrname) {

     if (usrname == NULL) 
	return 0;

     char rdir[20];
     char str[100];
     struct passwd * pwd;
     FILE * fd;
     pid_t uid;
     
     snprintf(rdir, sizeof rdir, "/proc/%d/status", pid);
	    
     fd = fopen (rdir, "r");
     if (fd == NULL ) {
        fprintf(stderr, "No such pid exist");	
   	return 0;
     }

     while (fgets(str, sizeof str, fd) != NULL){
	if (sscanf(str, "Uid:%d", &uid) == 1)
	    break;
     }
     fclose(fd);

     pwd = getpwuid(uid);
     if (pwd == NULL )
	return 0;

     return strcmp(pwd->pw_name, usrname) ? 0 : 1;
}

/* Check if the process runs under the environment indicated
 * by the path 
 */
int has_path(pid_t pid, char * path) {

    char rdir[20];
    char buf[128];
    ssize_t len;
    snprintf(rdir, sizeof rdir, "/proc/%d/cwd", pid);
    
    if ( (len = readlink(rdir, buf, sizeof(buf) -1)) != -1)
	buf[len] = '\0'; 
    else 
	return 0;

    return strstr(buf, path) == NULL ? 0 : 1;
}


