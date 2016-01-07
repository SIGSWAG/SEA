#include "tree.h"
#include "kheap.h"


node* tree_minimum(node* nil, node* x){
    node * n = x;
    while(n->left != nil){
        n = n->left;
    }

    return n;
}

node* tree_maximum(node* nil, node* x){
    node * n = x;
    while(n->right != nil){
        n = n->right;
    }

    return n;
}

node* tree_successor(node* nil, node* x){


    if(x->right != nil){

        return tree_minimum(nil, x->right);

    }
    node* y = x->parent;
    node* n = x;
    while(y!=nil && n == y->right ){
        n=y;
        y=y->parent;
    }

    return y;
}

node* tree_predecessor(node* nil, node* x){


    if(x->left != nil){

        return tree_maximum(nil, x->right);

    }
    node* y = x->parent;
    node* n = x;
    while(y!=nil && n == y->left ){
        n=y;
        y=y->parent;
    }

    return y;
}


// we assume that x has a right child
void rotate_left(tree* T, node* x){

    node* y = x->right;
    x->right = y->left;

    if(y->left != T->nil){

        y->left->parent = x;

    }

    y->parent = x->parent;

    if(x->parent == T->nil){
        T->root = y;
    }else if(x ==  x->parent->left){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;

}


//we assume that x has a left child
void rotate_right(tree* T, node* x){

    node* y = x->left;
    x->left = y->right;

    if(y->right != T->nil){

        y->right->parent = x;

    }

    y->parent = x->parent;

    if(x->parent == T->nil){
        T->root = y;
    }else if(x == x->parent->left){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }


    y->right = x;
    x->parent = y;
}


void rb_insert_correction(tree* T, node* z){

    node* y;

    while(z->parent->color == RED){

        if(z->parent == z->parent->parent->left){

            y = z->parent->parent->right;

            if(y->color == RED){ //case 1

                y->color = BLACK;
                z->parent->color = BLACK;
                z->parent->parent->color = RED;

                z = z->parent->parent;

            }else{

                if(z == z->parent->right){
                    z = z->parent;
                    rotate_left(T,z);


                }
                z->parent->color = BLACK;
                z->parent->parent->color =RED;
                rotate_right(T, z->parent->parent);

            }



        }else{


            y = z->parent->parent->left;

            if(y->color == RED){ //case 1

                y->color = BLACK;
                z->parent->color = BLACK;
                z->parent->parent->color = RED;

                z = z->parent->parent;

            }else{

                if(z == z->parent->left){
                    z = z->parent;
                    rotate_right(T,z);


                }
                z->parent->color = BLACK;
                z->parent->parent->color =RED;
                rotate_left(T, z->parent->parent);

            }
        }

    }

    T->root->color = BLACK;
}



void infixe(node* nil, node* x){
    if(x!=nil){
        //printf(" %d (%d) sons : %d (%d), %d (%d)",x->key,x->color, x->left->key, x->left->color, x->right->key, x->right->color);
        infixe(nil, x->left);
        infixe(nil, x->right);
    }

}

void insert(tree *T, node *z){

	T->nb_node++;

    node* y = T->nil;
    node* x = T->root;


    while(x != T->nil){
        y=x;

        if(z->key < x->key){
            x = x->left;

        }else{
            x = x->right;
        }

    }

    z->parent = y;

    if(y==T->nil){
        T->root = z;
    }else if(z->key < y->key){
        y->left = z;
    }else{
        y->right=z;
    }
    z->left = T->nil;
    z->right = T->nil;
    z->color = RED;

    rb_insert_correction(T, z);


}


node* search(tree* T, int key){


    node* curr = T->root;

    while(curr != T->nil && curr->key != key){

        if(curr->key > key){

            curr = curr->left;
        }else {

            curr = curr->right;
        }

    }

    return curr;
}


void rb_transplant(tree* T, node* u, node *v){


    if(u->parent == T->nil){

        T->root = v;

    }else if(u == u->parent->left){

        u->parent->left = v;

    }else{

        u->parent->right = v;
    }

    v->parent = u->parent;

}

void rb_delete_correction(tree* T, node*x){
    node* w;
    while(x != T->root && x->color == BLACK){
        if(x ==  x->parent->left){
            w = x->parent->right;

            if(w->color == RED){//case 1
                w->color = BLACK;
                x->parent->color = RED;
                rotate_left(T, x->parent);
                w = x->parent->right;
            }
            if(w->left->color == BLACK && w->right->color == BLACK){//case 2
                w->color = RED;
                x = x->parent;

            }else{

                if(w->right->color == BLACK){//case 3
                    w->left->color = BLACK;
                    w->color = RED;
                    rotate_right(T,w);
                    w = x->parent->right;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotate_left(T, x->parent);
                x = T->root;
            }




        }else{
            w = x->parent->left;

            if(w->color == RED){//case 1
                w->color = BLACK;
                x->parent->color = RED;
                rotate_right(T, x->parent);
                w = x->parent->left;
            }
            if(w->right->color == BLACK && w->left->color == BLACK){//case 2
                w->color = RED;
                x = x->parent;

            }else{

                if(w->left->color == BLACK){//case 3
                    w->right->color = BLACK;
                    w->color = RED;
                    rotate_left(T,w);
                    w = x->parent->left;
                }

                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotate_right(T, x->parent);
                x = T->root;
            }
        }



    }

    x->color = BLACK;
}

void rb_delete(tree* T, node* z){

	T->nb_node--;

    node* y = z;
    node* x;
    int y_original_col = y->color;

    if(z->left == T->nil){
        x = z->right;
        rb_transplant(T, z, z->right);
    }else if(z->right == T->nil){
        x = z->left;
        rb_transplant(T,z, z->left);

    }else{
        y_original_col = y->color;
        y = tree_minimum(T->nil,z->right);
        x = y->right;

        if(y->parent == z){
            x->parent = y;

        }else{
            rb_transplant(T,y,y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        rb_transplant(T, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;

    }

    if(y_original_col == BLACK){

        rb_delete_correction(T, x);
    }

}


void delete_in_tree(tree* t, int key){

    node* z = search(t, key);
    rb_delete(t, z);

}

void insert_in_tree(tree* t,int key, struct pcb_s* proc){

    node* n = (node *) kAlloc(sizeof(node));
    n->color=RED;
    n->key=key;
    n->left=t->nil;
    n->right=t->nil;
    n->parent=t->nil;
    n->process=proc; 

    insert(t, n);
}


