#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

typedef int (func_t) (void);

struct pcb_s {
	uint32_t regs[13];
	uint32_t sp;
	uint32_t lr_svc;
	uint32_t lr_user;
	uint32_t cpsr;
	int status;
	int returnCode;

	func_t * entry;
	struct pcb_s * next_pcb;
};

struct node {
    int key;
    struct pcb_s process;
    struct node* left;
    struct node* right;
    struct node* parent;
    int color;
};

typedef struct node node;

struct tree{
    node* root;
    node* nil;

};

typedef struct tree tree;

#endif
