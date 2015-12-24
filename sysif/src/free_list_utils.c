#include "free_list_utils.h"




void* allocate_pages(int nbPages, struct block* block ){


    //On recherche une Page libre : TODO si plus de block libre ?
    struct block * current_block = 0;
    struct block * iterate_block =  block;
    int foundBlock = 0;

    while(iterate_block != 0 && foundBlock == 0){//on cherche un bloc

        if(iterate_block->block_size >= nbPages){
            current_block = iterate_block;
            foundBlock = 1;
        }
        iterate_block = iterate_block->next;

    }
    if(current_block == 0){ //pas de bloc disponible
        return 0;
    }


    void* retour = current_block->first_page;

    current_block->block_size -= nbPages; //on décrémente la taille du bloc


    /**if(current_block->block_size == 0){ //le bloc est vide, on branche sur le suivant TODO libération mémoire
        process->first_empty_block = process->first_empty_block->next;
        process->first_empty_block->previous = 0;
    }else{
        current_block->first_page = (int*) ((unsigned int)(current_block->first_page) + PAGE_SIZE);
    }*/


    return retour;

}




void free_pages(int nbPages, void* first_page, struct block* block){






}
