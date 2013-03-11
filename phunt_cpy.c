#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <unistd.h>

#include <pwd.h>
#include <dirent.h>
#include "phunt.h"
#include "cmdlist.h"

    FILE * l;
    cmdlist * cl;

int main(int argc, char *argv[]){
    
    // Get option arguments
    char c;
    char log_file[40] = {0};
    char conf_file[40] = {0};
    int  daemon = 0;
    while ((c = getopt(argc, argv, "l::c::d")) != EOF){
	switch (c){
	    case 'l':
		if (optarg)
		    strcpy(log_file, optarg); 
		break;
	    case 'c':
		if (optarg)
		    strcpy(conf_file, optarg);
		break;
	    case 'd':
		daemon = 1;
		break;
	    default:
		break;
	}
    }

    if (strlen(log_file) == 0)
	open_log_file(DEFAULT_LOG);
    else
	open_log_file(log_file);


    if (strlen(conf_file) == 0)
	load_conf_file(DEFAULT_CONF);	 
    else
	load_conf_file(conf_file);
    huntps(cl); 
    fclose(l);
    return 0;
}

void usage(){
    fprintf(stderr, "phunt -l <log file> -c <config file> [-d]\n");
}

void log_entry(const char * entry) {

    time_t now = time(NULL);
    char * ts = (char *)asctime(localtime(&now));
    fprintf(l, "%.*s %s phunt:\t%s \n", strlen(ts) - 1, ts ,
	    getpwuid(getuid())->pw_name,  entry);

//    printf("user=[%s]\n", getpwuid(1000)->pw_name);
}

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

int exceed_mem(pid_t pid, int limit) {
  
    int  vmsize, vmrss, share, text, lib, data, dt;
    char log[40];
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
    if (nfill != 7) {
	snprintf(log, sizeof log, 
		"[%d] Extract data failed(exceed_mem)", pid); 
	log_entry(log);
	return 0;

    } 

    long page_sz = sysconf(_SC_PAGESIZE);
    totMB = (vmsize +vmrss +share +text +lib +data +dt) *page_sz /(1024*1024);

    return totMB > limit ? 1 : 0;
}

int match_user(pid_t pid, char * usrname) {

     if (usrname == NULL) 
	return 0;

     char rdir[20];
     char log[40];
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
     if (pwd == NULL ) {
	snprintf(log, sizeof log, 
		"[%d] No match user [%s]", pid, usrname);
	log_entry(log);
	return 0;
     }

     return strcmp(pwd->pw_name, usrname) ? 0 : 1;
}

int has_path(pid_t pid, char * path) {

    char rdir[20];
    char log[40];
    char buf[ARG_MAX];
    ssize_t len;
    snprintf(rdir, sizeof rdir, "/proc/%d/cwd", pid);
    
    if ( (len = readlink(rdir, buf, sizeof(buf) -1)) != -1)
	buf[len] = '\0'; 
    else {
	snprintf(log, sizeof log, 
		"Read process ID[%d] failed(has_path)\n", pid);
	log_entry(log);
	return 0;
    }

    return strstr(buf, path) == NULL ? 0 : 1;
}

int store_conf_cmd(cmdlist * cl, char ** cmdtokens, int len) {
    if (len < 3 ) {
	log_entry("Invalid configuration");
	return -1;
    }
    
    char * err;
    pht_t act_id;
    pht_t typ_id;

    if (strcmp(cmdtokens[0], "kill") == 0)
	act_id = ACT_KILL;
    else if (strcmp(cmdtokens[0], "nice") == 0)
	act_id = ACT_NICE;
    else if (strcmp(cmdtokens[0], "suspend") == 0)
	act_id = ACT_SUSP;
    else {
	err = "Configuration: invalid action";
	fprintf(stderr, "%s (%s)\n", err, cmdtokens[0]);
	log_entry(err);
	abort();
    }

    if (strcmp(cmdtokens[1], "user") == 0)
	typ_id = TYPE_USR;
    else if (strcmp(cmdtokens[1], "path") == 0 )
	typ_id = TYPE_PTH;
    else if (strcmp(cmdtokens[1], "memory") == 0)
	typ_id = TYPE_MEM;
    else {
	err = "Configuration: invalid type";
	fprintf(stderr, "%s (%s)\n", err, cmdtokens[1]);
	log_entry(err);
	abort();
    }

    struct pht_cmd * pcmd= create_pht_cmd(act_id, typ_id, cmdtokens[2]);
    add_pht_cmd(cl, pcmd);
}

int open_log_file(const char * logfile) {
    char entry1[40];
    char entry2[40];

    l = fopen(logfile, "a");
    if (l == NULL) {
	printf("Open file failed: %s \n", logfile);
	abort();
    }

    snprintf(entry1, sizeof(entry1), "phunt startup (PID=%d)", (int) getpid());
    log_entry(entry1);
    
    // Log entry to log file
    snprintf(entry2, sizeof(entry2), "opened logfile %s", logfile);
    log_entry(entry2);
}


int load_conf_file(const char * confile) {

    FILE * fd;
    char entry[40];

    fd = fopen(confile, "r");
    if (fd == NULL) {
	printf("Open file failed: %s \n", confile);
	abort();
    }
   
    snprintf(entry, sizeof(entry), "parsing configuration %s", confile);	
    log_entry(entry);  
     
    cl = init_cmdlist();
    while ( !feof(fd) ) {
    	char buf[64];
    	char* buf1[100];
	char * tmp = fgets(buf, (int)sizeof(buf), fd);

	if(tmp == NULL) break;

	tokenize(buf, buf1);

	if (**buf1 != '#' && **buf1 != '\0') {
	    store_conf_cmd(cl, buf1, 3);   
	}
    }
    fclose(fd);
}


int huntps(cmdlist * cl) {
    DIR * dir;
    struct dirent * dent;
    int pid;
    
    struct pht_cmd * pcmd;
    
    dir = opendir("/proc");
    if (dir == NULL) {
	perror("open /proc failed");
	return -1;
    }
    
    pcmd = getHead(cl); int i =0;
    while (pcmd != NULL) {
        while ((dent = readdir(dir)) != NULL) {
    	    char log[1024] = {0};
	    int action = 0, cx = 0;
	    if (dent->d_type == DT_DIR && (pid = atoi(dent->d_name)) != 0) {
		if (pcmd->typ_id == TYPE_USR && match_user(pid, pcmd->arg)) {
		    action = 1;	    
		    cx += snprintf(log, sizeof(log)-cx-1, 
			"[%d] match user [%s]; ", pid, pcmd->arg);
		}
                if (pcmd->typ_id == TYPE_PTH && has_path(pid, pcmd->arg)) {
		    action = 1;
		    cx += snprintf(log+cx, sizeof(log)-cx-1, 
			"[%d] has path [%s]; ", pid, pcmd->arg);
		}
		if (pcmd->typ_id == TYPE_MEM && exceed_mem(pid, atoi(pcmd->arg))) {
		    action = 1;
		    cx += snprintf(log+cx, sizeof(log)-cx-1, 
			"[%d] exceed mem [%s]; ", pid, pcmd->arg);  
		}
		
		if (action) {
		    switch (pcmd->act_id) {
			case ACT_KILL:
			    if (kill(pid, SIGKILL) != -1) 
			        cx += snprintf(log+cx, sizeof(log)-cx-1, 
					"[%d] Action [KILL] activated, SIGKILL sent; ", pid);
			    if (kill(pid, 0) == -1)
			        cx += snprintf(log+cx, sizeof(log)-cx-1,
					"[%d] has killed; ", pid);
			    else
			        cx += snprintf(log+cx, sizeof(log)-cx-1,
					"[%d] killed? undefined; ", pid);
			    break;
			case ACT_SUSP:
			    if (kill(pid, SIGTSTP) != -1) 
			        cx += snprintf(log+cx, sizeof(log)-cx-1, 
					"[%d] Action [SUSP] activated, SIGTSTP sent; ", pid);
			    if (issusp(pid))
			    	cx += snprintf(log+cx, sizeof(log)-cx-1, 
					"[%d] has suspended; ", pid);
			    else
			    	cx += snprintf(log+cx, sizeof(log)-cx-1, 
					"[%d] suspended? undefined; ", pid);
			    break;
			case ACT_NICE:
			    setpriority(PRIO_PROCESS, pid, -3);
			    cx += snprintf(log+cx, sizeof(log)-cx-1,
					"[%d] Action [NICE] activated; ", pid);
			    if (getpriority(PRIO_PROCESS, pid) == -3)
				cx += snprintf(log+cx, sizeof(log)-cx-1,
					"[%d] priority has setted; ", pid);
			    else 
				cx += snprintf(log+cx, sizeof(log)-cx-1,
					"[%d] priority setted? undefined; ", pid);
			    break;
			default:
			    break;
		    }
		}
    		if (strlen(log) != 0)
	            log_entry(log);
	    }
        } 
	rewinddir(dir);
	pcmd = pcmd->next; 
    }
    closedir(dir);

    return 0;
}

