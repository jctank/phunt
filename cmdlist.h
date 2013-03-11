#ifndef CMDLIST_H
#define CMDLIST_H

#include "phunt.h"
#define CMD_MAX 100

typedef struct cmdlist {
    	
    struct pht_cmd * head;
    struct pht_cmd * tail;
    int size;

}cmdlist;

cmdlist * init_cmdlist();

struct pht_cmd * create_pht_cmd(pht_t act_id, pht_t typ_id, char * arg);

int add_pht_cmd(cmdlist * cl, struct pht_cmd * pcmd);

struct pht_cmd * getHead(struct cmdlist * cl);

void printcmd(cmdlist * cl);
int getSize(struct cmdlist * cl);

void freecmdlist(cmdlist * cl);

#endif
