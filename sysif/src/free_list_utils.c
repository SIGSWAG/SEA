#include "free_list_utils.h"
#include "kheap.h"
#include "vmem.h"



void* allocate_pages(int nbPages, struct block** block, int page_size ){


    //On recherche une Page libre : TODO si plus de block libre ?
    struct block * current_block = 0;
    struct block * iterate_block =  *block;
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


    if(current_block->block_size == 0){ //le bloc est vide, on branche sur le suivant TODO libération mémoire

        if(current_block==*block){//on est sur le premier bloc

            *block = current_block->next;
            (*block)->previous = current_block->previous;



        }else{

            current_block->previous->next = current_block->next;
            current_block->next->previous = current_block->previous;

        }

         kFree((void *)current_block, sizeof(struct block));//libération du tableau
    }else{
        current_block->first_page = (int*) ((unsigned int)(current_block->first_page) + nbPages*page_size);
    }


    return retour;

}




void free_pages(int nbPages, void* first_page, struct block** block_process, int page_size){


    unsigned int currentPage ;
    unsigned int pageToFree = (unsigned int) first_page;
    struct block* currentBlock = *block_process;
    struct block* precedingBlock;

    /** 7 cas différents pour l'insertion de la page **/

    if(currentBlock !=0 ){
        currentPage = (unsigned int) currentBlock->first_page;

    }else{//il n'y a pas de bloc CAS 1
        //Allocation du bloc
        struct block * block = (struct block *) kAlloc(sizeof(struct block));
        block->block_size = nbPages;
        block->first_page = (int*) first_page;
        block->next = 0;
        block->previous = 0;
        *block_process = block;

        return;
    }

    //on parcourt les blocs pour trouver le premier bloc dans lequel la première page ne
    while(currentPage < pageToFree && currentBlock != 0 ){

        precedingBlock = currentBlock;
        currentBlock = currentBlock->next;
        if(currentBlock != 0){
            currentPage = (unsigned int) currentBlock->first_page;
        }


    }

    if(currentPage >= pageToFree && currentBlock != 0){ //on regarde le bloc courant


        if( (unsigned int)(pageToFree) + nbPages*page_size == (unsigned int) currentBlock->first_page){ //insertion juste avant le bloc courant CAS 2

            currentBlock->first_page = (int*) pageToFree;
            currentBlock->block_size += nbPages;

        }else if(currentBlock->previous !=0){ //insertion au niveau du bloc précédent

            unsigned int lastPage = (unsigned int) currentBlock->previous->first_page + currentBlock->previous->block_size * page_size; //premiere page apres le bloc

            if(lastPage == ((unsigned int)(pageToFree))){ //insertion juste après le bloc précédent CAS 3
                currentBlock->previous->block_size+=nbPages;

            }else{ //création d'un nouveau bloc CAS 4
                //Allocation du bloc
                struct block * block = (struct block *) kAlloc(sizeof(struct block));
                block->block_size = nbPages;
                block->first_page = (int*) pageToFree;
                block->next = currentBlock;
                block->previous = currentBlock->previous;
                currentBlock->previous->next = block;
                currentBlock->previous = block;

            }


        }else{ //allocation d'un bloc en début de liste CAS 5
            //Allocation du bloc
            struct block * block = (struct block *) kAlloc(sizeof(struct block));
            block->block_size = nbPages;
            block->first_page = (int*) pageToFree;
            block->next = currentBlock; //un seul bloc disponible
            block->previous = 0;
            currentBlock->previous = block;
            (*block_process) = block;

        }
    }else{//on était au dernier block, insertion après le dernier bloc

        unsigned int lastPage = (unsigned int) precedingBlock->first_page + precedingBlock->block_size * page_size;//premiere page apres le  dernier bloc

        if(lastPage ==  ((unsigned int)(pageToFree))){ //insertion après le bloc précédent CAS 6
            precedingBlock->block_size+=nbPages;

        }else{ //nouveau bloc en fin de liste CAS 7
            //Allocation du bloc
            struct block * block = (struct block *) kAlloc(sizeof(struct block));
            block->block_size = nbPages;
            block->first_page = (int*) pageToFree;
            block->next = 0;
            block->previous = precedingBlock;
            precedingBlock->next = block;


        }



    }



}
