#include "vmem.h"
#include "kheap.h"
#include "sched.h"
#include "uart.h"
#include "syscall.h"
#include "free_list_utils.h"

#define DEBUG 0


#define BEGIN_IO_MAPPED_MEM 0x20000000
#define END_IO_MAPPED_MEM 0x20FFFFFF
#define END_KERNEL_MEM 0x1000000

/** champs de bits pour descripteurs**/
#define TABLE_2_NORMAL_PERIPH  0b000000110110
#define TABLE_2_NORMAL_MEM  0b000001110010
#define TABLE_1_NORMAL  0b0000000001
#define TABLE_2_NORMAL_MEM_RO 0b001001110010

/** Fautes d'accès aux données **/
// Descripteur de niveau 2 (deuxième table des pages) invalide (bits 00 dans l'entrée)
#define TRANSLATION_FAULT_PAGE 0b0111
#define ACCESS_FAULT 0b0110
#define PERMISSION_FAULT 0b1111
// Descripteur de niveau 1 (première table des pages) invalide (bits 00 dans l'entrée)
#define TRANSLATION_FAULT_SECTION 0b0101

uint8_t * occupation_table;

unsigned int * table_kernel;

/**
 * @brief data_handler (pour fautes de traductions/accès)
 */
void data_handler(){
    uint32_t dataFaultStatus;
    uint32_t faultAddress;
    uint32_t dfsr;

    // Cause de la faute
    __asm volatile("MRC p15, 0, %0, c5, c0, 0" : "=r"(dfsr));

    // On récupère les bits [3:0] pour avoir la cause
    dataFaultStatus = dfsr & 0xF;

    // Adresse virtuelle qui a causé la faute
    __asm volatile("MRC p15, 0, %0, c6, c0, 0" : "=r"(faultAddress));

    //On configure la MMU avec la table des pages système
    invalidate_TLB();
    configure_mmu_kernel();

    if(dataFaultStatus == TRANSLATION_FAULT_PAGE) {
        uart_send_str("Data fault -- translation fault, page level\n");
    }
    else if(dataFaultStatus == ACCESS_FAULT) {
        uart_send_str("Data fault -- access fault\n");
    }
    else if(dataFaultStatus == PERMISSION_FAULT) {
        uart_send_str("Data fault -- permission fault\n");
    }
    else if(dataFaultStatus == TRANSLATION_FAULT_SECTION) {
        uart_send_str("Data fault -- translation fault, section level\n");
    }
    else {
        uart_send_str("Data fault -- Undefined data fault\n");
    }
    //uart_send_str("at address %d", faultAddress);

    // Stop current process
    uart_send_str("Stopping current process\n");
    sys_exit(- dataFaultStatus);
}


void start_mmu_C()
{
    register unsigned int control;

    __asm("mcr p15, 0, %[zero], c1, c0, 0" :: [zero]"r"(0)); // Disable cache
    __asm("mcr p15, 0, r0, c7, c7, 0"); // Invalidate cache (data and instructions)
    
    invalidate_TLB();
    
    // Enable ARMv6 MMU features (disable sub-page AP)
    control = (1 << 23) | (1 << 15) | (1 << 4) | 1;
    
    // Write control register
    __asm volatile("mcr p15, 0, %[control], c1, c0, 0" :: [control]"r" (control));
}

void invalidate_TLB() 
{
    __asm("mcr p15, 0, r0, c8, c6, 0"); // Invalidate TLB entries

    // Invalidate the translation lookaside buffer (TLB)
    //__asm volatile("mcr p15, 0, %[data], c8, c7, 0" :: [data]"r"(0));
}

void configure_mmu_kernel() {
    configure_mmu_C((unsigned int)table_kernel);
}

void configure_mmu_C(register unsigned int pt_addr)
{
    //total++; ?????

    // Translation table 0
    __asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt_addr));

    // Translation table 1
    __asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));

    // Use translation table 0 for everything
    __asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));

    /* Set Domain 0 ACL to "Manager", not enforcing memory permissions
         * Evert mapped section/page is in domain 0
         */
    __asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x3));
}



/**
 * @brief init_kernel_table_page
 * @return
 */
unsigned int * init_kernel_table_page()
{
    /** On alloue la table des pages de niveau 1 alignée sur 16384 **/
    unsigned int * table1 = (unsigned int *) kAlloc_aligned(FIRST_LVL_TT_SIZE, 14);

    unsigned int TABLE_2_BITSET;

    /** Pour chaque entrée de la table de niveau 1 **/
    for(unsigned int i = 0; i < FIRST_LVL_TT_COUNT; i++) {
        /** Faute si i n'est pas entre 200 et 20F ou entre 0 et la fin du kernel **/
        if( !(i >= 0x200 && i <= 0x20F) && !(i < (((unsigned int)&__kernel_heap_end__) >> 20)) )
        {
            table1[i] = 0x0;
            continue;
        }

        /** peripherique mappe en memoire **/
        if(i>=0x200 && i<=0x20F)
        {

            TABLE_2_BITSET = TABLE_2_NORMAL_PERIPH;
        }
        else
        {
            TABLE_2_BITSET = TABLE_2_NORMAL_MEM;
        }

        /** On alloue une table de niveau 2 alignée sur 1024 **/
        unsigned int * table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

        /** Pour chaque entrée dans cette table **/
        for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){
            /** concat i|j|champ de bits **/
            table2[j] = (i<<20) | (j<<12) | TABLE_2_BITSET;
        }
        /** On place l'adresse de la table de niveau 2 dans la table de niveau 1 **/
        table1[i] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits
    }
    
    return table1;
}
/**
 * @brief init_table_page
 * @return
 */
unsigned int * init_table_page()
{

    /** On alloue la table des pages de niveau 1 alignée sur 16384 **/
    unsigned int * table1 = (unsigned int *) kAlloc_aligned(FIRST_LVL_TT_SIZE, 14);

    unsigned int TABLE_2_BITSET;



    /** Pour chaque entrée de la table de niveau 1 **/
    for(unsigned int i = 0; i < FIRST_LVL_TT_COUNT; i++) {
        /** Faute si i n'est pas entre 200 et 20F ou entre 0 et la fin du kernel **/
        if(!(i < (((unsigned int)&__kernel_heap_end__) >> 20)) )
        {
            table1[i] = 0x0;

        }else if(i > (((unsigned int)&__kernel_heap_start__) >> 20))//on heap, aucun accès
        {
            table1[i] = 0x0;//table_kernel[i] ;


        }else{ //reste du noyau, en RO


            unsigned int kernel_heap_begin = (((unsigned int)&__kernel_heap_start__) >> 12); //adresse de la page contenant le début du heap noyau

            TABLE_2_BITSET = TABLE_2_NORMAL_MEM_RO;


            /** On alloue une table de niveau 2 alignée sur 1024 **/
            unsigned int * table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

            /** Pour chaque entrée dans cette table **/
            for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){

                if((i<<10|j) >= kernel_heap_begin){ //la page est au dela du début du heap, faute de traduction WARNING DEBUG

                        table2[j]=0x0;

                }else{
                    /** concat i|j|champ de bits **/
                    table2[j] = (i<<20) | (j<<12) | TABLE_2_BITSET;
                }
            }
            /** On place l'adresse de la table de niveau 2 dans la table de niveau 1**/
            table1[i] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits
        }
    }

    return table1;

}


/**
 * @brief free_process_memory
 * @param process
 */
void free_process_memory(struct pcb_s* process){


    unsigned int* table1 = process->page_table;


    for(int i=0; i<FIRST_LVL_TT_COUNT;i++){//pour chaque entrée de la table des pages de niveau 1

        if(!(i < (END_KERNEL_MEM >> 20)) )//on va a la table de niveau 2
        {
            unsigned int * table2;
            if(table1[i]!=0x0){
                table2 = (unsigned int*)((((unsigned int)table1[i])>>10)<<10); //22 premiers bits, alignés sur 1024

                for(int j=0; j<SECON_LVL_TT_COUNT; j++){//pour chaque entrée dans la table de niveau 2

                    if(table2[j]!=0x0){//on libère

                        int address = ((table2[j]>>12)<<12);

                        int offset = (address - END_KERNEL_MEM) / PAGE_SIZE;

                        occupation_table[offset]=0;

                    }
                }


                kFree((uint8_t*)table2, SECON_LVL_TT_SIZE);
            }
        }


    }


    //libération de la free list pages ;
    struct block* current_block = process->first_empty_block;
    while(current_block!=0){

        struct block* tampon = current_block;
        current_block = current_block->next;
        kFree((uint8_t*)tampon, sizeof(struct block));


    }

    //libération de la free list octets ;
    current_block = process->first_empty_block_heap;
    while(current_block!=0){

        struct block* tampon = current_block;
        current_block = current_block->next;
        kFree((uint8_t*)tampon, sizeof(struct block));


    }



    kFree((uint8_t*)table1, FIRST_LVL_TT_SIZE);


    //libération des tables d'allocations
    for(int i=0; i<process->allocated_adresses_size; i++){

        kFree((void*)process->allocated_adresses[i], sizeof(int)*2);


    }
    kFree((void*)process->allocated_adresses, sizeof(int)*process->allocated_adresses_size);






}



/**
 * @brief init_kern_translation_table
 * @return
 */
int init_kern_translation_table(void)
{
    table_kernel = init_kernel_table_page();

    configure_mmu_C((unsigned int)table_kernel);

    return 0;
}

/**
 * @brief init_occupation_table
 */
void init_occupation_table(void)
{

    occupation_table = (uint8_t *) kAlloc(OCCUPATION_TABLE_SIZE); //TODO allouer une table avec 1 bit/frame == bool

    for(int i=0; i<OCCUPATION_TABLE_SIZE; i++){
        occupation_table[i] = 0;
    }


}
/**
 * @brief vmem_init
 */
void vmem_init()
{

    init_occupation_table();
    init_kern_translation_table();
    start_mmu_C();

}

/**
 * @brief allocate_stack_for_process
 * @param process
 * @param nbPages
 * @return
 */
void* allocate_stack_for_process(struct pcb_s* process, int nbPages){

    //on alloue dans le premier(et donc dernier) bloc
    struct block * current_block = process->first_empty_block;
    int* page = (int*)0xFFFFFFFF; //adresse de la première page (fin de la RAM, pile descendante)
    unsigned int * table1 = process->page_table;


    //On cherche nbPages Frames libres;
    int indexOccup = 0;
    int* addresses  = (int*) kAlloc(sizeof(int)*nbPages);//tableau qui contiendra les adresses des frames
    int indexAddr = 0;

    while(indexAddr < nbPages && indexOccup < OCCUPATION_TABLE_SIZE)//tant que le tableau n'est pas rempli
    {

        if(occupation_table[indexOccup] == 0){


            addresses[indexAddr] = indexOccup;
            indexAddr++;
        }
        indexOccup++;
    }


    if(indexAddr < nbPages){//pas assez de frames
        kFree((void *)addresses, sizeof(int)*nbPages);//libération du tableau
        return 0;

    }

    for(int i=0; i<nbPages; i++){//on alloue les frames

        //addresse de la frame
        int addressFrame = END_KERNEL_MEM + addresses[i]*PAGE_SIZE; // calcul de l'offset

        //adresse de la page
        int addressPage = (int)(page) - i*PAGE_SIZE;

        int offsetFirstTable =((unsigned int)addressPage)>>20; //12 premiers bits => offset dans la première table
        int offsetSecondTable = (((unsigned int)addressPage)<<12)>>24 ; //10 bits suivants => offset dans la deuxième table;


        unsigned int * table2;


        if(table1[offsetFirstTable] == 0x0){//pas de table de niveau 2, il faut allouer

            // On alloue une table de niveau 2 alignée sur 1024
            table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

            // Pour chaque entrée dans cette table
            for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){

                table2[j]=0x0;

            }
            //On place l'adresse de la table de niveau 2 dans la table de niveau 1
            table1[offsetFirstTable] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits


        }else{

            table2 = (unsigned int*)((((unsigned int)table1[offsetFirstTable])>>10)<<10); //22 premiers bits, alignés sur 1024

        }
        table2[offsetSecondTable] = ((addressFrame>>12)<<12) | TABLE_2_NORMAL_MEM;//descripteur de niveau 2


        current_block->block_size--;//une page de moins dans le bloc

        table2[offsetSecondTable] = ((addressFrame>>12)<<12) | TABLE_2_NORMAL_MEM;//descripteur de niveau 2

        if(current_block->block_size == 0){ //le bloc est vide, on branche sur le suivant TODO libération mémoire
            return 0;
        }

        occupation_table[addresses[i]] = 1; //la frame est marquée occupée

    }




    kFree((void *)addresses, sizeof(int)*nbPages);//libération du tableau

    return page;
}




/**
 * @brief vmem_alloc_for_userland_single
 * @param process
 * @return
 */
void* vmem_alloc_for_userland_single(struct pcb_s* process)
{

    //On recherche une Page libre : TODO si plus de block libre ?
    struct block * first_block = process->first_empty_block;
    int* page = first_block->first_page; //adresse de la première page
    unsigned int * table1 = process->page_table;



    //On cherche une Frame libre;
    int found = 0;
    int i = 0;
    int address  = 0;

    while(found == 0 && i < OCCUPATION_TABLE_SIZE)
    {

        if(occupation_table[i] == 0){

            //la Frame est libre
            address = END_KERNEL_MEM + i*PAGE_SIZE; // calcul de l'offset
            found = 1;

            int offsetFirstTable =((unsigned int)page)>>20; //12 premiers bits => offset dans la première table
            int offsetSecondTable = (((unsigned int)page)<<12)>>24 ; //10 bits suivants => offset dans la deuxième table;


            unsigned int * table2;


            if(table1[offsetFirstTable] == 0x0){//pas de table de niveau 2, il faut allouer

                /** On alloue une table de niveau 2 alignée sur 1024 **/
                table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

                /** Pour chaque entrée dans cette table **/
                for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){

                    table2[j]=0x0;

                }
                /** On place l'adresse de la table de niveau 2 dans la table de niveau 1 **/
                table1[offsetFirstTable] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits


            }else{

                table2 = (unsigned int*)((((unsigned int)table1[offsetFirstTable])>>10)<<10); //22 premiers bits, alignés sur 1024

            }

            table2[offsetSecondTable] = ((address>>12)<<12) | TABLE_2_NORMAL_MEM;//descripteur de niveau 2


            first_block->block_size--;//une page de moins dans le bloc

            if(first_block->block_size == 0){ //le bloc est vide, on branche sur le suivant TODO libération mémoire
                process->first_empty_block = process->first_empty_block->next;
                process->first_empty_block->previous = 0;
            }else{
                first_block->first_page = (int*) ((unsigned int)(first_block->first_page) + PAGE_SIZE);
            }

            occupation_table[i] = 1; //la frame est marquée occupée

        }
        i++;
    }

    return page;

}


void* vmem_alloc_for_userland(struct pcb_s* process, int nbPages)
{

    //On recherche une Page libre : TODO si plus de block libre ?
    struct block * current_block = 0;
    struct block * iterate_block =  process->first_empty_block;
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

    unsigned int * table1 = process->page_table;
    void* retour = current_block->first_page;


    //On cherche nbPages Frames libres;
    int indexOccup = 0;
    int* addresses  = (int*) kAlloc(sizeof(int)*nbPages+32);//tableau qui contiendra les adresses des frames
    int indexAddr = 0;

    while(indexAddr < nbPages && indexOccup < OCCUPATION_TABLE_SIZE)//tant que le tableau n'est pas rempli
    {

        if(occupation_table[indexOccup] == 0){


            addresses[indexAddr] = indexOccup;
            indexAddr++;
        }
        indexOccup++;
    }


    if(indexAddr < nbPages){//pas assez de frames
        kFree((void *)addresses, sizeof(int)*nbPages);//libération du tableau
        return 0;

    }

    for(int i=0; i<nbPages; i++){//on alloue les frames

        //addresse de la frame
        int addressFrame = END_KERNEL_MEM + addresses[i]*PAGE_SIZE; // calcul de l'offset

        //adresse de la page
        int addressPage = ((int) retour )+ i*PAGE_SIZE;

        int offsetFirstTable =((unsigned int)addressPage)>>20; //12 premiers bits => offset dans la première table
        int offsetSecondTable = (((unsigned int)addressPage)<<12)>>24 ; //10 bits suivants => offset dans la deuxième table;


        unsigned int * table2;


        if(table1[offsetFirstTable] == 0x0){//pas de table de niveau 2, il faut allouer

            /** On alloue une table de niveau 2 alignée sur 1024 **/
            table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

            /** Pour chaque entrée dans cette table **/
            for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){

                table2[j]=0x0;

            }
            /** On place l'adresse de la table de niveau 2 dans la table de niveau 1 **/
            table1[offsetFirstTable] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits


        }else{

            table2 = (unsigned int*)((((unsigned int)table1[offsetFirstTable])>>10)<<10); //22 premiers bits, alignés sur 1024

        }

        table2[offsetSecondTable] = ((addressFrame>>12)<<12) | TABLE_2_NORMAL_MEM;//descripteur de niveau 2


        current_block->block_size--;//une page de moins dans le bloc

        if(current_block->block_size == 0){ //le bloc est vide, on branche sur le suivant TODO libération mémoire
            if(current_block==process->first_empty_block){

                process->first_empty_block = process->first_empty_block->next;
                process->first_empty_block->previous = 0;

            }else{

                current_block->previous->next = current_block->next;
                current_block->next->previous = current_block->previous;

            }

            kFree((void *)current_block, sizeof(struct block));//libération du tableau

        }else{
            current_block->first_page = (int*) ((unsigned int)(current_block->first_page) + PAGE_SIZE);
        }

        occupation_table[addresses[i]] = 1; //la frame est marquée occupée

    }



    kFree((void *)addresses, sizeof(int)*nbPages+32);//libération du tableau
    return retour;

}






/**
 * @brief vmem_desalloc_for_userland_single
 * @param process
 * @param page
 */
void vmem_desalloc_for_userland_single(struct pcb_s* process, void* page)
{


    //on modifie la table des pages
    int offsetFirstTable =((unsigned int)page)>>20; //12 premiers bits => offset dans la première table
    int offsetSecondTable = (((unsigned int)page)<<12)>>24 ; //10 bits suivants => offset dans la deuxième table;

    unsigned int * table1 =  process->page_table;
    unsigned int * table2;

    table2 = (unsigned int*)((((unsigned int)table1[offsetFirstTable])>>10)<<10); //22 premiers bits, alignés sur 1024

    unsigned int address = (table2[offsetSecondTable]>>12)<<12;
    table2[offsetSecondTable] = 0x0; //libération de la page

    int offset =  (address - END_KERNEL_MEM) / PAGE_SIZE;

    occupation_table[offset] = 0;//libération de la frame physique

    /** Libération de la page pour le processus **/

    unsigned int currentPage ;
    unsigned int pageToFree = (unsigned int) page;
    struct block* currentBlock = process->first_empty_block;
    struct block* precedingBlock;

    /** 7 cas différents pour l'insertion de la page **/

    if(currentBlock !=0 ){
        currentPage = (unsigned int) currentBlock->first_page;

    }else{//il n'y a pas de bloc CAS 1
        //Allocation du bloc
        struct block * block = (struct block *) kAlloc(sizeof(struct block));
        block->block_size = 1;
        block->first_page = (int*) pageToFree;
        block->next = 0;
        block->previous = 0;
        process->first_empty_block = block;
    }


    while(currentPage < pageToFree && currentBlock != 0 ){

        precedingBlock = currentBlock;
        currentBlock = currentBlock->next;
        if(currentBlock != 0){
            currentPage = (unsigned int) currentBlock->first_page;
        }


    }

    if(currentPage >= pageToFree && currentBlock != 0){ //on regarde le bloc courant


        if( (unsigned int)(pageToFree) + PAGE_SIZE == (unsigned int) currentBlock->first_page){ //insertion juste avant le bloc courant CAS 2

            currentBlock->first_page = (int*) pageToFree;
            currentBlock->block_size++;

        }else if(currentBlock->previous !=0){ //insertion au niveau du bloc précédent

            unsigned int lastPage = (unsigned int) currentBlock->previous->first_page + currentBlock->previous->block_size * PAGE_SIZE; //premiere page apres le bloc

            if(lastPage == ((unsigned int)(pageToFree))){ //insertion juste après le bloc précédent CAS 3
                currentBlock->previous->block_size++;

            }else{ //création d'un nouveau bloc CAS 4
                //Allocation du bloc
                struct block * block = (struct block *) kAlloc(sizeof(struct block));
                block->block_size = 1;
                block->first_page = (int*) pageToFree;
                block->next = currentBlock;
                block->previous = currentBlock->previous;
                currentBlock->previous->next = block;
                currentBlock->previous = block;

            }


        }else{ //allocation d'un bloc en début de liste CAS 5
            //Allocation du bloc
            struct block * block = (struct block *) kAlloc(sizeof(struct block));
            block->block_size = 1;
            block->first_page = (int*) pageToFree;
            block->next = currentBlock; //un seul bloc disponible
            block->previous = 0;
            currentBlock->previous = block;
            process->first_empty_block = block;

        }
    }else{//on était au dernier block, insertion après le dernier bloc

        unsigned int lastPage = (unsigned int) precedingBlock->first_page + precedingBlock->block_size * PAGE_SIZE;//premiere page apres le  dernier bloc

        if(lastPage ==  ((unsigned int)(pageToFree))){ //insertion après le bloc précédent CAS 6
            precedingBlock->block_size++;

        }else{ //nouveau bloc en fin de liste CAS 7
            //Allocation du bloc
            struct block * block = (struct block *) kAlloc(sizeof(struct block));
            block->block_size = 1;
            block->first_page = (int*) pageToFree;
            block->next = 0;
            block->previous = precedingBlock;
            precedingBlock->next = block;


        }



    }



}

/**
 * @brief vmem_desalloc_for_userland permet de désallouer nbPages commençant à page.
 * Le comportement de cette méthode n'est garanti que si nbPages ont été allouées a partir de page par vmem_alloc_for_userland
 * @param process
 * @param page
 * @param nbPages
 */
void vmem_desalloc_for_userland(struct pcb_s* process, void* page, int nbPages){

    for(int i=0; i<nbPages; i++){
        void* pageToFree = (void*)( ((unsigned int)page)+ i*PAGE_SIZE);
        vmem_desalloc_for_userland_single(process, pageToFree);


    }


}






/**
 * @brief vmem_translate
 * @param va
 * @param process
 * @return
 */
uint32_t vmem_translate(uint32_t va, struct pcb_s* process)
{
    uint32_t pa; /*The result*/

    /* 1st and 2nd table addresses */
    uint32_t table_base;
    uint32_t second_level_table;

    /* Indexes */
    uint32_t first_level_index;
    uint32_t second_level_index;
    uint32_t page_index;

    /* Descriptors */
    uint32_t first_level_descriptor;
    uint32_t *first_level_descriptor_address;
    uint32_t second_level_decriptor;
    uint32_t *second_level_decriptor_adress;

    if(process == 0)
    {
        __asm("mrc p15, 0, %[tb], c2, c0, 0":[tb]"=r"(table_base));
    } else
    {
        table_base = (uint32_t) process-> page_table;
    }

    table_base = table_base & 0xFFFFC000;

    /* Indexes */
    first_level_index = (va>>20);
    second_level_index = ((va<<12)>>24);
    page_index = (va & 0x00000FFF);

    /* First level descriptor */
    first_level_descriptor_address = (uint32_t*) (table_base|(first_level_index<<2));
    first_level_descriptor = *(first_level_descriptor_address);

    /* Translation fault */
    if(! (first_level_descriptor & 0x3))
    {
        return (uint32_t) FORBIDDEN_ADDRESS;
    }


    /* Second level descriptor */
    second_level_table = first_level_descriptor & 0xFFFFFC00;
    second_level_decriptor_adress = (uint32_t*) (second_level_table | (second_level_index << 2));
    second_level_decriptor = *((uint32_t*) second_level_decriptor_adress);

    /* Translation fault */
    if (!(second_level_decriptor & 0x3))
    {
        return (uint32_t) FORBIDDEN_ADDRESS;
    }

    /* Physical addreseap_init(s */
    pa = (second_level_decriptor & 0xFFFFF000) | page_index;

    return pa;

}

void* do_gmalloc(struct pcb_s* process, int size){

    void * retour = 0x0;
    if(process->first_empty_block_heap == 0){
        //first allocation
        int nbPages = size / PAGE_SIZE + 1;


        //Allocation du bloc
        struct block * block = (struct block *) kAlloc(sizeof(struct block));
        block->block_size = 1337;


        void * first_byte = vmem_alloc_for_userland(process, nbPages); //on alloue assez de pages pour le processus.


        free_pages(nbPages*PAGE_SIZE, first_byte, &process->first_empty_block_heap, 1);

        retour = allocate_pages(size, &process->first_empty_block_heap, 1);

    }else{

        void * first_byte =  allocate_pages(size, &process->first_empty_block_heap, 1); //on alloue assez de pages pour le processus.

        if(first_byte == 0x0){//pas assez de place, on réalloue

            int nbPages = size / PAGE_SIZE + 1;

            void * first_page = vmem_alloc_for_userland(process, nbPages); //on alloue assez de pages


            free_pages(size, first_page, &process->first_empty_block_heap, 1);

            retour = allocate_pages(size, &process->first_empty_block_heap, 1);
            //on parcourt la free_list pour insérer au bon endroit

        }else{

            retour =  first_byte;

        }




    }

    int found = 0;
    int i =0;
    while(i<process->allocated_adresses_size && found == 0){

        if(process->allocated_adresses[i][0]== 0){
            process->allocated_adresses[i][0] = (int)retour;
            process->allocated_adresses[i][1] = size;
            found = 1;
        }
        i++;

    }

    if(found==0 && i==process->allocated_adresses_size){ //il faut réallouer

        int ** allocated_adresses = (int**) kAlloc(sizeof(int)*process->allocated_adresses_size*2); //on recopie tout dans un tableau plus grand

        for(int i=0; i<process->allocated_adresses_size*2;i++){


            allocated_adresses[i]= (int*) kAlloc(sizeof(int)*2);

            allocated_adresses[i][0] = 0;

            if(i<process->allocated_adresses_size){
                allocated_adresses[i][0] = process->allocated_adresses[i][0];
                allocated_adresses[i][1] = process->allocated_adresses[i][1];

            }
        }


        for(int i=0; i<process->allocated_adresses_size; i++){ //libération du tableau précédent

            kFree((void*)process->allocated_adresses[i], sizeof(int)*2);


        }
        kFree((void*)process->allocated_adresses, sizeof(int)*process->allocated_adresses_size);



        process->allocated_adresses_size = process->allocated_adresses_size*2;
        process->allocated_adresses =  allocated_adresses;


    }


    return retour;


}



void do_gfree_sized(struct pcb_s* process, void* pointer, int size){


    free_pages(size, pointer, &process->first_empty_block_heap, 1);


    //on cherche ensuite les pages en question, pour la libérer éventuellement
    int nbPages = size/PAGE_SIZE+1;


    for(int i=0; i<nbPages; i++){ //pour chaque page, on vérifie si on ne peut pas la désallouer

        int pageAddress = (((int)(pointer)>>20)<<20) + i*PAGE_SIZE;

        int found = 0;
        struct block* block = process->first_empty_block_heap;
        while(found == 0 && block != 0x0 && pageAddress <= (((int)(block->first_page)>>20)<<20)){

            if(pageAddress == ((int)(block->first_page)/ PAGE_SIZE)){
                found = 1;
            }
            block = block->next;

        }

        if(found==0){ //on peut désallouer la page, rien ne pointe dessus



            //vmem_desalloc_for_userland_single(process, (void*)pageAddress);

        }
    }


}

void do_gfree(struct pcb_s* process, void* pointer){

    int found = 0;
    int i =0;
    int size = 0;
    while(i<process->allocated_adresses_size && found == 0){

        if(process->allocated_adresses[i][0] == (int)pointer){
            size = process->allocated_adresses[i][1];
            process->allocated_adresses[i][0] = 0;
            found = 1;
        }
        i++;

    }

    if(found == 1){

        do_gfree_sized(process, pointer, size);
    }




}





