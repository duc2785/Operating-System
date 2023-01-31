#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "vm/page.h"
#include <list.h>

void init_swap_list(void);
void insert_swap_list(struct page *page);
void delete_swap_list(struct page *page);
struct page* alloc_page(enum palloc_flags flags);
void free_page(void *paddr);
static struct list_elem* move_victim();
void evict_page(enum palloc_flags flags);
#endif