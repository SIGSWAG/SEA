#ifndef TREE_H
#define TREE_H

#include "sched.h"

#define RED 1
#define BLACK 0

struct node {
    int key;
    struct node* left;
    struct node* right;
    struct node* parent;
    int color;
    struct pcb_s* process;
};

typedef struct node node;

struct tree{
    node* root;
    node* nil;
    int nb_node;
};

typedef struct tree tree;


node* tree_minimum(node* nil, node* x);

node* tree_maximum(node* nil, node* x);

node* tree_successor(node* nil, node* x);

node* tree_predecessor(node* nil, node* x);

// we assume that x has a right child
void rotate_left(tree* T, node* x);

//we assume that x has a left child
void rotate_right(tree* T, node* x);

void rb_insert_correction(tree* T, node* z);

void infixe(node* nil, node* x);

void insert(tree *T, node *z);

node* search(tree* T, int key);

void rb_transplant(tree* T, node* u, node *v);

void rb_delete_correction(tree* T, node*x);

void rb_delete(tree* T, node* z);

void delete_in_tree(tree* t, int key);

void insert_in_tree(tree* t, int key, struct pcb_s* proc);

#endif
