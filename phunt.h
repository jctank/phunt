#ifndef PHUNT_H
#define PHUNT_H

#define DEFAULT_LOG	"/var/log/phunt.log"
#define DEFAULT_CONF	"/etc/phunt.conf"

#define FILE_NAME_SIZE	40
#define USER_MAX	32
//#define PATH_MAX	128
#define ARG_MAX		128

typedef int pht_t;

#define ACT_KILL	1
#define ACT_NICE	2
#define ACT_SUSP	3

#define TYPE_USR	1
#define TYPE_PTH	2
#define TYPE_MEM	3

struct pht {
    pht_t act_id;         
//    pht_t typ_id;        
};

struct pht_cmd {
    pht_t act_id;
    pht_t typ_id;
    char  arg[ARG_MAX];
    struct pht_cmd * next;
};

struct pht_mem {
    pht_t act_id;     
    pht_t typ_id;    /* = TYPE_MEM */
    long  mem_max;
};

struct pht_user {
    pht_t act_id;
    pht_t typ_id;    /* = TYPE_USR */
    char  user[USER_MAX];
};

struct pht_path {
    pht_t act_id;
    pht_t typ_id;    /* = TYPE_PTH*/
    char  path[128];
};


#endif
