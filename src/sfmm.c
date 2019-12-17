/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

extern struct sf_block sf_free_list_heads[NUM_FREE_LISTS];

void create_epilogue(){
    void *endaddress = sf_mem_end()-8;
    sf_epilogue* epilogue = (sf_epilogue*)endaddress;
    epilogue->header = THIS_BLOCK_ALLOCATED;
}

void add_node(sf_block *block){
    int minSize = 32;
    size_t blockSize = block->header&BLOCK_SIZE_MASK;
    for(int i = 0;i<NUM_FREE_LISTS;i++){
        if(blockSize>minSize&&i!=8){
            minSize*=2;
            continue;
        }
        if(sf_free_list_heads[i].body.links.next==&sf_free_list_heads[i]){
            sf_free_list_heads[i].body.links.next = block;
            sf_free_list_heads[i].body.links.prev = block;
            block->body.links.next = &sf_free_list_heads[i];
            block->body.links.prev = &sf_free_list_heads[i];
            break;
        }else{
            sf_free_list_heads[i].body.links.next->body.links.prev = block;//first block in list.prev = block
            block->body.links.next = sf_free_list_heads[i].body.links.next;//blocks next = old first block
            sf_free_list_heads[i].body.links.next = block;//dummy.next = block
            block->body.links.prev = &sf_free_list_heads[i];//blocks prev = dummy
            break;
        }
    }
    return;
}

void remove_node(sf_block *block){
    sf_block* prevblock = block->body.links.prev;
    sf_block* nextblock = block->body.links.next;
    prevblock->body.links.next = nextblock;
    nextblock->body.links.prev = prevblock;
}

void coalesce(sf_block *block){
    size_t blockSize = block->header&BLOCK_SIZE_MASK;
    //Next block is not epilgue get block
    sf_block *nextBlock = NULL;
    sf_block *prevBlock = NULL;
    int nextAlloc = 1;
    int prevAlloc = 1;
    if((((void*)block+blockSize)+8)!=(sf_mem_end()-8)){
        nextBlock = (sf_block*)((void*)block+blockSize);
        if(nextBlock->header&(1<<1)){
            nextAlloc = 1;
        }else{
            nextAlloc = 0;
        }
    }
    //prev block not prolgue get block
    if((((void*)block)-32)!=(sf_mem_start())){
        sf_footer prevFooter = block->prev_footer;//seg fault here
        size_t prevHeader = prevFooter^sf_magic();
        size_t prevSize = prevHeader&BLOCK_SIZE_MASK;
        prevBlock = (sf_block*)((void*)block-prevSize);
        if(prevBlock->header&(1<<1)){
            prevAlloc = 1;
        }else{
            prevAlloc = 0;
        }
    }
    add_node(block);
    //Check if either block is allocated
    //Case 1 blocks before and after are alloced
    if(prevAlloc==1&&nextAlloc==1){
        block->header = block->header & (~(1 << 1));
        //Epilogue is after block now
        if((((void*)block+blockSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)block+blockSize);
            *footer = block->header^sf_magic();
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
            epilogue->header = (epilogue->header) & (~(1 << 0));
        }else{
            //change header of proceeding block and prev footer field
            sf_block *nextblock = (sf_block*)((void*)block+blockSize);
            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
            nextblock->header = (nextblock->header) & (~(1 << 0));
            nextblock->prev_footer=block->header^sf_magic();
            //change prev footer of block after proceding block
            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                //set manual footer
                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                *footer = nextblock->header^sf_magic();
            }else{
                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                afterblock->prev_footer = nextblock->header^sf_magic();
            }
        }
        //add_node(block);
        return;
    }else if(prevAlloc==1&&nextAlloc==0){//Case 2 prev block allocated and next block is free
        int thisAlloc = block->header&(1<<0);
        size_t totalSize = blockSize + (nextBlock->header&BLOCK_SIZE_MASK);
        remove_node(nextBlock);
        remove_node(block);
        if(thisAlloc==1){
            block->header = totalSize|PREV_BLOCK_ALLOCATED;
        }else{
            block->header = totalSize;
        }
        //Epilogue is after block now
        if((((void*)block+totalSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)block+totalSize);
            *footer = block->header^sf_magic();
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
            epilogue->header = (epilogue->header) & (~(1 << 0));
        }else{
            //change header of proceeding block and prev footer field
            sf_block *nextblock = (sf_block*)((void*)block+totalSize);
            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
            nextblock->header = nextblock->header & (~(1 << 0));
            nextblock->prev_footer=block->header^sf_magic();
            //change prev footer of block after proceding block
            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                //set manual footer
                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                *footer = nextblock->header^sf_magic();
            }else{
                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                afterblock->prev_footer = nextblock->header^sf_magic();
            }
        }
        add_node(block);
        return;
    }else if(prevAlloc==0&&nextAlloc==1){//Case 3 prev block free and next block allocated
        int thisAlloc = prevBlock->header&(1<<0);
        size_t totalSize = blockSize + (prevBlock->header&BLOCK_SIZE_MASK);
        remove_node(prevBlock);
        remove_node(block);
        if(thisAlloc==1){
            prevBlock->header = totalSize|PREV_BLOCK_ALLOCATED;
        }else{
            prevBlock->header = totalSize;
        }
        //Epilogue is after block now
        if((((void*)prevBlock+totalSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)prevBlock+totalSize);
            *footer = prevBlock->header^sf_magic();
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
            epilogue->header = (epilogue->header) & (~(1 << 0));
        }else{
            //change header of proceeding block and prev footer field
            sf_block *nextblock = (sf_block*)((void*)prevBlock+totalSize);
            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
            nextblock->header = nextblock->header & (~(1 << 0));
            nextblock->prev_footer=prevBlock->header^sf_magic();
            //change prev footer of block after proceding block
            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                //set manual footer
                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                *footer = nextblock->header^sf_magic();
            }else{
                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                afterblock->prev_footer = nextblock->header^sf_magic();
            }
        }
        add_node(prevBlock);
        return;
    }else{//Both blocks free
        int thisAlloc = prevBlock->header&(1<<0);
        size_t totalSize = blockSize + (prevBlock->header&BLOCK_SIZE_MASK) + (nextBlock->header&BLOCK_SIZE_MASK);
        remove_node(nextBlock);
        remove_node(prevBlock);
        remove_node(block);
        if(thisAlloc==1){
            prevBlock->header = totalSize|PREV_BLOCK_ALLOCATED;
        }else{
            prevBlock->header = totalSize;
        }
        //Epilogue is after block now
        if((((void*)prevBlock+totalSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)prevBlock+totalSize);
            *footer = prevBlock->header^sf_magic();
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
            epilogue->header = (epilogue->header) & (~(1 << 0));
        }else{
            //change header of proceeding block and prev footer field
            sf_block *nextblock = (sf_block*)((void*)prevBlock+totalSize);
            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
            nextblock->header = nextblock->header & (~(1 << 0));
            nextblock->prev_footer=prevBlock->header^sf_magic();
            //change prev footer of block after proceding block
            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                //set manual footer
                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                *footer = nextblock->header^sf_magic();
            }else{
                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                afterblock->prev_footer = nextblock->header^sf_magic();
            }
        }
        add_node(prevBlock);
        return;
    }

}

void *sf_malloc(size_t size) {
    if((int)size<=0) {
        return NULL;
    }
    size_t totalSize = size+16;
    //If size of block is a mult of 16 keep otherwise find padding
    if(totalSize%16!=0){
        totalSize += 16-(totalSize%16);
    }
    //no heap data yet
    if(sf_mem_start()==sf_mem_end()){
        for(int i = 0;i<NUM_FREE_LISTS;i++){
            sf_free_list_heads[i].body.links.next=&sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev=&sf_free_list_heads[i];
        }
        void *growaddress = sf_mem_grow();
        if(growaddress==NULL){
            sf_errno = ENOMEM;
            return NULL;
        }else{
            //Make prologue
            sf_prologue *prologue = (sf_prologue*)growaddress;
            prologue->header = 32|3;
            prologue->unused1 = NULL;
            prologue->unused2 = NULL;
            prologue->padding1 = 0;
            prologue->footer = prologue->header^sf_magic();

            // block for free page
            void *pageaddress = growaddress+32;

            sf_block *pageBlock = (sf_block*)((void*)pageaddress);
            pageBlock->header = (PAGE_SZ-48)|PREV_BLOCK_ALLOCATED;
            add_node(pageBlock);
            pageBlock->prev_footer = (prologue->footer);

            //footer for page
            sf_footer *footer = (sf_footer*)(sf_mem_end()-16);
            *footer = pageBlock->header^sf_magic();

            create_epilogue();
        }
    }
    //return NULL;
    for(int i = 0;i<NUM_FREE_LISTS;i++){
        //Blocks not big enough
        //No blocks available in list
        sf_block *block = sf_free_list_heads[i].body.links.next;
        if(block==&sf_free_list_heads[i]){
            //Final list with no blocks available call mem grow and coalesce
            if(i==8){
                void *growaddress = sf_mem_grow();
                if(growaddress==NULL){
                    sf_errno = ENOMEM;
                    return NULL;
                }
                //Get manual footers data
                sf_footer *manfooter = (sf_footer*)((void*)growaddress-16);
                size_t tempheader = *manfooter^sf_magic();
                int tempAlloc = tempheader&(1<<1);
                //Create page block
                sf_epilogue *epilogue = (sf_epilogue*)(growaddress-8);
                sf_block *pageBlock = (sf_block*)((void*)growaddress-16);
                //Change epilogue to header for block
                if(tempAlloc==1){
                    epilogue->header = PAGE_SZ|PREV_BLOCK_ALLOCATED;
                }else{
                    epilogue->header = PAGE_SZ;
                }
                pageBlock->header = epilogue->header;
                pageBlock->prev_footer = *manfooter;
                //footer for page
                sf_footer *footer = (sf_footer*)(sf_mem_end()-16);//(pageBlock+PAGE_SZ-48);//
                *footer = pageBlock->header^sf_magic();
                coalesce(pageBlock);
                create_epilogue();
                i-=2;
                continue;
            }
        }else{
            //Found available block for alloc
            while(block!=&sf_free_list_heads[i]){
                int prevAlloc = block->header&(1<<0);
                size_t blockSize = block->header&BLOCK_SIZE_MASK;
                //block size matches exactly
                if(blockSize==totalSize){
                    //THIS PART WORKS TRUST
                    remove_node(block);
                    //block->body.payload = char[size];
                    block->header = block->header|THIS_BLOCK_ALLOCATED;
                    if(prevAlloc==1){
                        block->header = block->header|PREV_BLOCK_ALLOCATED;
                    }
                    //Set epilogue data else set block proceeding
                    if((((void*)block+blockSize)+8)==(sf_mem_end()-8)){
                        //set manual footer
                        sf_footer *footer = (sf_footer*)((void*)block+blockSize);
                        *footer = block->header^sf_magic();
                        sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
                        epilogue->header = (epilogue->header)|PREV_BLOCK_ALLOCATED;
                    }else{
                        //change header of proceeding block and prev footer field
                        sf_block *nextblock = (sf_block*)((void*)block+totalSize);
                        size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
                        nextblock->header = nextblock->header|PREV_BLOCK_ALLOCATED;
                        nextblock->prev_footer=block->header^sf_magic();
                        //change prev footer of block after proceding block
                        if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                            //set manual footer
                            sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                            *footer = nextblock->header^sf_magic();
                        }else{
                            sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                            afterblock->prev_footer = nextblock->header^sf_magic();
                        }
                    }
                    return block->body.payload;
                }else if(blockSize>totalSize){//Block size bigger than needed
                    //sizes dont match exactly
                    remove_node(block);
                    //spliter will occur
                    if(blockSize-totalSize<32){
                        block->header = blockSize|THIS_BLOCK_ALLOCATED;
                        if(prevAlloc==1){
                            block->header = block->header|PREV_BLOCK_ALLOCATED;
                        }
                        //Set prev alloc bit for proceeding header
                        if((((void*)block+blockSize)+8)==(sf_mem_end()-16)){
                            //set manual footer
                            sf_footer *footer = (sf_footer*)((void*)block+blockSize);
                            *footer = block->header^sf_magic();
                            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
                            epilogue->header = (epilogue->header)|PREV_BLOCK_ALLOCATED;
                        }else{
                            //change header of proceeding block and prev footer field
                            sf_block *nextblock = (sf_block*)((void*)block+totalSize);
                            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
                            nextblock->header = nextblock->header|PREV_BLOCK_ALLOCATED;
                            nextblock->prev_footer=block->header^sf_magic();
                            //change prev footer of block after proceding block
                            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                                //set manual footer
                                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                                *footer = nextblock->header^sf_magic();
                            }else{
                                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                                afterblock->prev_footer = nextblock->header^sf_magic();
                            }
                        }
                        return block->body.payload;
                    }
                    void* address = block;
                    //Change header size to new size
                    block->header = totalSize|THIS_BLOCK_ALLOCATED;
                    if(prevAlloc==1){
                        block->header = block->header|PREV_BLOCK_ALLOCATED;
                    }
                    //split block
                    size_t newblockSize = blockSize-totalSize;
                    sf_block *newblock = (sf_block*)((void*)address+totalSize);
                    newblock->header = (newblockSize)|PREV_BLOCK_ALLOCATED;
                    //newblock->header = newblock->header&0x01;
                    newblock->prev_footer = block->header^sf_magic();
                    add_node(newblock);
                    if((((void*)block+blockSize)+8)==(sf_mem_end()-8)){
                        //set manual footer
                        sf_footer *footer = (sf_footer*)(sf_mem_end()-16);//(address+blockSize);
                        *footer = newblock->header^sf_magic();
                    }else{
                        //change prev footer of proceeding block
                        sf_block *nextblock = (sf_block*)((void*)newblock+newblockSize);
                        nextblock->prev_footer=newblock->header^sf_magic();
                    }
                    return block->body.payload;
                }else if(i==8&&block->body.links.next==&sf_free_list_heads[i]){
                    //last array and no blocks fit
                    void *growaddress = sf_mem_grow();
                    if(growaddress==NULL){
                        sf_errno = ENOMEM;
                        return NULL;
                    }
                    //Get manual footers data
                    sf_footer *manfooter = (sf_footer*)((void*)growaddress-16);
                    size_t tempheader = *manfooter^sf_magic();
                    int tempAlloc = tempheader&(1<<1);
                    //Create page block
                    sf_epilogue *epilogue = (sf_epilogue*)(growaddress-8);
                    sf_block *pageBlock = (sf_block*)((void*)growaddress-16);
                    //Change epilogue to header for block
                    if(tempAlloc==1){
                        epilogue->header = PAGE_SZ|PREV_BLOCK_ALLOCATED;
                    }else{
                        epilogue->header = PAGE_SZ;
                    }
                    pageBlock->header = epilogue->header;

                    //add_node(pageBlock);
                    pageBlock->prev_footer = *manfooter;
                    //footer for page
                    sf_footer *footer = (sf_footer*)(sf_mem_end()-16);
                    *footer = pageBlock->header^sf_magic();
                    coalesce(pageBlock);
                    create_epilogue();
                    i-=2;
                    break;
                }
                block = block->body.links.next;
            }
        }
    }
    return NULL;
}

void sf_free(void *pp) {
    if(pp==NULL) abort();
    pp -= 16;
    sf_block *block = (sf_block*)(void*)pp;
    sf_header header = block->header;
    int prevAlloc = block->header&(1<<0);
    size_t blockSize = header&BLOCK_SIZE_MASK;
    //Allocated bit isn't set || blocksize < 32 ||
    if((header&(1<<1))==0||(int)blockSize<32){
        abort();
    }
    //header is before prologue end || footer is after epilogue start
    if(((void*)block-32<sf_mem_start())||(((void*)block+blockSize)+8)>(sf_mem_end()-8)){
        abort();
    }
    //Check that next block isnt epilogue
    if((((void*)block+blockSize)+8)!=(sf_mem_end()-8)){
        sf_block *nextBlock = (sf_block*)((void*)block+blockSize);
        sf_footer prevFooter = nextBlock->prev_footer;
        //footer mismatch
        if((prevFooter^sf_magic())!=header){
            abort();
        }
    }else{
        sf_footer *footer = (sf_footer*)(sf_mem_end()-16);
        //footer mismatch
        if((*footer^sf_magic())!=header){
            abort();
        }
    }
    //check that previous block isnt prologue
    if(((void*)block-32!=sf_mem_start())){
        sf_footer prevFooter = block->prev_footer^sf_magic();
        size_t prevSize = prevFooter&BLOCK_SIZE_MASK;
        sf_block *prevBlock = (sf_block*)((void*)block-prevSize);
        //Prev block alloc is set but block's prev alloc bit isn't || Prev block alloc isn't set but block's prev alloc bit is
        if(((prevBlock->header&(1<<1))==1&&(header&(1<<0))==0)||((prevBlock->header&(1<<1))==0&&(header&(1<<0))==1)){
            abort();
        }
    }
    //Valid pointer confirmed
    //Change header bits
    if(prevAlloc==1){
        block->header = blockSize|PREV_BLOCK_ALLOCATED;
    }else{
        block->header = blockSize;
    }
    //Set footer data
    if((((void*)block+blockSize)+8)==(sf_mem_end()-8)){
        //set manual footer
        sf_footer *footer = (sf_footer*)((void*)block+blockSize);
        *footer = block->header^sf_magic();
        sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
        epilogue->header = (epilogue->header) & (~(1 << 0));
    }else{
        //change header of proceeding block and prev footer field
        sf_block *nextblock = (sf_block*)((void*)block+blockSize);
        size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
        nextblock->header = (nextblock->header) & (~(1 << 0));
        nextblock->prev_footer=block->header^sf_magic();
        //change prev footer of block after proceding block
        if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
            *footer = nextblock->header^sf_magic();
        }else{
            sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
            afterblock->prev_footer = nextblock->header^sf_magic();
        }
    }
    coalesce(block);
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    if(pp==NULL) {
        sf_errno = EINVAL;
        abort();
    }
    pp -= 16;
    sf_block *block = (sf_block*)(void*)pp;
    sf_header header = block->header;
    size_t blockSize = header&BLOCK_SIZE_MASK;
    //Allocated bit isn't set || blocksize < 32
    if((header&(1<<1))==0||(int)blockSize<32){
        sf_errno = EINVAL;
        return NULL;
    }
    //header is before prologue end || footer is after epilogue start
    if(((void*)block-32<sf_mem_start())||(((void*)block+blockSize)+8)>(sf_mem_end()-8)){
        sf_errno = EINVAL;
        return NULL;
    }
    //Check that next block isnt epilogue
    if((((void*)block+blockSize)+8)!=(sf_mem_end()-8)){
        sf_block *nextBlock = (sf_block*)((void*)block+blockSize);
        sf_footer prevFooter = nextBlock->prev_footer;
        //footer mismatch
        if((prevFooter^sf_magic())!=header){
            sf_errno = EINVAL;
            return NULL;
        }
    }else{
        sf_footer *footer = (sf_footer*)(sf_mem_end()-16);
        //footer mismatch
        if((*footer^sf_magic())!=header){
            sf_errno = EINVAL;
            return NULL;
        }
    }
    //check that previous block isnt prologue
    if(((void*)block-32!=sf_mem_start())){
        sf_footer prevFooter = block->prev_footer^sf_magic();
        size_t prevSize = prevFooter&BLOCK_SIZE_MASK;
        sf_block *prevBlock = (sf_block*)((void*)block-prevSize);
        //Prev block alloc is set but block's prev alloc bit isn't || Prev block alloc isn't set but block's prev alloc bit is
        if(((prevBlock->header&(1<<1))==1&&(header&(1<<0))==0)||((prevBlock->header&(1<<1))==0&&(header&(1<<0))==1)){
            sf_errno = EINVAL;
            return NULL;
        }
    }
    if((int)rsize==0){
        pp+=16;
        sf_free(pp);
        return NULL;
    }
    size_t totalRsize = rsize+16;
    //If size of block is a mult of 16 keep otherwise find padding
    if(totalRsize%16!=0){
        totalRsize += 16-(totalRsize%16);
    }
    //size_t payloadSize = blockSize-16;
    //allocating to bigger block
    if(blockSize<totalRsize){
        void* newBlock = sf_malloc(rsize);
        if(newBlock==NULL){
            return NULL;
        }
        pp+=16;
        memcpy(newBlock,  pp, rsize);
        sf_free(pp);
        return newBlock;
    }
    //allocating to smaller block
    else if(blockSize>totalRsize){
        //Spilinter will occur
        if(blockSize-totalRsize<32){
            return block->body.payload;
        }

        //Split the block
        int prevAlloc = block->header&(1<<0);
        void* address = block;
        //Change header size to new size
        block->header = totalRsize|THIS_BLOCK_ALLOCATED;
        if(prevAlloc==1){
            block->header = block->header|PREV_BLOCK_ALLOCATED;
        }
        //split new free block
        size_t newblockSize = blockSize-totalRsize;
        sf_block *newblock = (sf_block*)((void*)address+totalRsize);
        newblock->header = newblockSize|PREV_BLOCK_ALLOCATED;
        newblock->prev_footer = block->header^sf_magic();
        //add_node(newblock);
        if((((void*)block+blockSize)+8)==(sf_mem_end()-8)){
            //set manual footer
            sf_footer *footer = (sf_footer*)((void*)newblock+newblockSize);
            *footer = newblock->header^sf_magic();
            sf_epilogue *epilogue = (sf_epilogue*)(sf_mem_end()-8);
            epilogue->header = (epilogue->header) & (~(1 << 0));
        }else{
            //change header of proceeding block and prev footer field
            sf_block *nextblock = (sf_block*)((void*)newblock+newblockSize);
            size_t nextBlockSize = nextblock->header&BLOCK_SIZE_MASK;
            nextblock->header = nextblock->header & (~(1 << 0));
            nextblock->prev_footer=newblock->header^sf_magic();
            //change prev footer of block after proceding block
            if((((void*)nextblock+nextBlockSize)+8)==(sf_mem_end()-8)){
                //set manual footer
                sf_footer *footer = (sf_footer*)((void*)nextblock+nextBlockSize);
                *footer = nextblock->header^sf_magic();
            }else{
                sf_block *afterblock = (sf_block*)((void*)nextblock+nextBlockSize);
                afterblock->prev_footer = nextblock->header^sf_magic();
            }
        }
        coalesce(newblock);
        return block->body.payload;
    }
    //Sizes are the same
    else{
        pp+=16;
        return pp;
    }
    return NULL;
}
