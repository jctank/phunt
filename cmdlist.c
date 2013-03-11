#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdlist.h"

cmdlist * init_cmdlist() {
    cmdlist * cl;
    cl = (cmdlist *) malloc(sizeof(cmdlist));
   
    if (cl == NULL) {
	perror("malloc");
	return NULL;
    }
    cl->size = 0;
    cl->head = NULL;
    cl->tail = NULL;
    
    return cl;
}

int add_pht_cmd(cmdlist * cl, struct pht_cmd * pcmd) {
    if (cl == NULL || pcmd == NULL) 
	return -1;   
    if (cl->head ==  NULL) {
	cl->head = pcmd;
	cl->tail = pcmd;
    }
    else {
	cl->tail->next = pcmd; 
	cl->tail = pcmd;
	
//	struct pht_cmd * tmp = cl->head;
//	cl->head = pcmd;
//	cl->head->next = tmp;
    }
    cl->size++;

    return 0;
}

struct pht_cmd * create_pht_cmd(pht_t act_id, pht_t typ_id, char * arg) {
    struct pht_cmd * pcmd;
    pcmd = (struct pht_cmd *) malloc(sizeof (struct pht_cmd));
    pcmd->act_id = act_id;
    pcmd->typ_id = typ_id;
    strncpy(pcmd->arg, arg, sizeof pcmd->arg);
    pcmd->arg[sizeof(pcmd->arg)-1] = '\0';
    pcmd->next = NULL;
    
    return pcmd; 
}

struct pht_cmd * getHead(cmdlist * cl) {
    
    return cl == NULL ? NULL : cl->head;
}

int getSize(cmdlist * cl) {

    return cl == NULL ? -1 : cl->size;
}

void printcmd(cmdlist * cl){
    struct pht_cmd * cmd = cl->head;
    while (cmd != NULL) {
	printf("[%d] [%d] [%s]\n", cmd->act_id, cmd->typ_id, cmd->arg);
	cmd = cmd->next;
    }

}

void freecmdlist(cmdlist * cl) {
    if (cl != NULL) {
	
	struct pht_cmd * tmp;
	
	while (cl->head != NULL ){
	    tmp = cl->head;
	    cl->head = cl->head->next;
	    free(tmp);
	}
	free(cl);
    }
}

