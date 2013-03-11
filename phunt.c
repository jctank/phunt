#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#include <dirent.h>
#include "phunt.h"
#include "cmdlist.h"
#include "phuntutil.h"

    FILE * l;      /* Log file */
    cmdlist * cl;  /* pht_cmd list */

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

    /* Open log file for logging */
    if (strlen(log_file) == 0)
	open_log_file(DEFAULT_LOG);
    else
	open_log_file(log_file);

    /* Load conf file for processing */
    if (strlen(conf_file) == 0)
	load_conf_file(DEFAULT_CONF);	 
    else
	load_conf_file(conf_file);
  
    /* Hunt for processses */
    huntps(cl); 

    fclose(l);
    return 0;
}

void usage(){
    fprintf(stderr, "phunt -l <log file> -c <config file> [-d]\n");
}

/**
 * Load_conf_file  helper function
 * Store each command from the conf file to the cmdlist 
 */
static int store_conf_cmd(cmdlist * cl, char ** cmdtokens, int len) {
    if (len < 3 ) {
	log_entry(l, "Invalid configuration");
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
	log_entry(l, err);
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
	log_entry(l, err);
	abort();
    }

    struct pht_cmd * pcmd= create_pht_cmd(act_id, typ_id, cmdtokens[2]);
    add_pht_cmd(cl, pcmd);
}


/**
 * Open the log file for logging until the program exits
 */
int open_log_file(const char * logfile) {
    char entry1[40];
    char entry2[40];

    l = fopen(logfile, "a");
    if (l == NULL) {
	printf("Open file failed: %s \n", logfile);
	abort();
    }

    // Log entry to log file
    snprintf(entry1, sizeof(entry1), "phunt startup (PID=%d)", (int) getpid());
    log_entry(l, entry1);
    
    snprintf(entry2, sizeof(entry2), "opened logfile %s", logfile);
    log_entry(l, entry2);
}


/* Parse the content of the configuration file, and store it as 
 * pht_cmds in a cmdlist structure
 */
int load_conf_file(const char * confile) {

    FILE * fd;
    char entry[40];

    fd = fopen(confile, "r");
    if (fd == NULL) {
	printf("Open file failed: %s \n", confile);
	abort();
    }
   
    snprintf(entry, sizeof(entry), "parsing configuration %s", confile);	
    log_entry(l, entry);  
     
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

/**
 * Search processes according to the commands stored in the 
 * cmdlist structure. If all the criterions of a process are 
 * matched, the corresponding action will be acted upon. 
 */
int huntps(cmdlist * cl) {
    DIR * dir;
    struct dirent * dent;
    int pid;
    
    struct pht_cmd * pcmd;
    
    /* Open /proc dir for reading */
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
		/* Determine if the type and argument are match */
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
		  /* Invoke the corresponding action */
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
	            log_entry(l, log);
	    }
        } 
	rewinddir(dir);     /* reset directory position */
	pcmd = pcmd->next;  /* Get the next cmd */
    }
    closedir(dir);

    return 0;
}

