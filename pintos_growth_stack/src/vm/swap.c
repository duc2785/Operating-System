
#include "vm/page.h"
#include "vm/frame.h"
#include <list.h>
#include <bitmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <list.h>
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/block.h"

struct bitmap *swap_map;
struct lock swap_lock1;
struct block *swap_block;
// swap 초기화
void init_swap(){

    //swap_block = block_get_role(BLOCK_SWAP);
    //if(swap_block != NULL){
        swap_map = bitmap_create(8*1024);
   // }
    
    lock_init(&swap_lock1);
}

//swap in
void swap_in(size_t index, void *kaddr){
    
    lock_acquire(&swap_lock1);
    swap_block = block_get_role(BLOCK_SWAP);

    if(bitmap_test(swap_map, index)){
        // bitmap이 존재하면
        for(int i= 0; i<(size_t)(PGSIZE/BLOCK_SECTOR_SIZE); i++){
            block_read(swap_block, (size_t)(PGSIZE/BLOCK_SECTOR_SIZE)*index+i, (uint8_t*)kaddr + BLOCK_SECTOR_SIZE*i);
            // block 읽어서 메모리로 load
        }
        bitmap_reset(swap_map, index);
    }
    lock_release(&swap_lock1);
}

// swap out
size_t swap_out(void *kaddr){

lock_acquire(&swap_lock1);
    swap_block = block_get_role(BLOCK_SWAP);
    //first fit algorithm -> 가장 앞 false
    size_t swap_num = bitmap_scan(swap_map, 0, 1, false);
    
    if(swap_num == BITMAP_ERROR){
        lock_release(&swap_lock1);
        return swap_num;
    }
    
    for(int i = 0; i< (size_t)(PGSIZE/BLOCK_SECTOR_SIZE); i ++){
        block_write(swap_block, (size_t)(PGSIZE/BLOCK_SECTOR_SIZE)*swap_num+i, (uint8_t *)kaddr+ BLOCK_SECTOR_SIZE*i);
    }
    bitmap_set(swap_map, swap_num, true);
    lock_release(&swap_lock1);
    return swap_num;
}

