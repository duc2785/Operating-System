#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/page.h"
#include "vm/frame.h"
#include <list.h>
#include <bitmap.h>


void init_swap();
void swap_in(size_t index, void *kaddr);
size_t swap_out(void *kaddr);

#endif