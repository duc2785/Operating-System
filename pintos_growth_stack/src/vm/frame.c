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
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

struct list swap_list;         // LRU사용, page들이 들어가 있는 list
struct list_elem *swap_victim; // victim pointer
struct lock swap_lock;         // list사용을 위한 lock

// swap list와 lock 초기화 함수
void init_swap_list(void)
{

    // list와 lock 초기화 ->init.c에서 초기화 수행ㄱ
    list_init(&swap_list);
    lock_init(&swap_lock);
    swap_victim = NULL;
}

// swap_list에서 페이지 제거
void delete_swap_list(struct page *page)
{

    // 만약 제거하려는 page가 victim_pointer일 경우
    // victim pointer를 다음 page로 넘겨줌
    if (swap_victim == &page->swap)
    {
        // swap_victim = list_entry(list_remove(&page->swap), struct page, swap);
        swap_victim = list_next(swap_victim);
        // return;
    }

    list_remove(&page->swap);
}

struct page *alloc_page(enum palloc_flags flags)
{
    // lock_acquire(&swap_lock);
    //  페이지 할당
    uint8_t *page = palloc_get_page(flags);
    // 새 페이지 구조체 선언
    struct page *new = (struct page *)malloc(sizeof(struct page));
    /*if(new == NULL){
        return NULL;
    }*/

    // 할당할 페이지가 없다면 page evict
    while (page == NULL)
    {

        evict_page(flags);
        page = palloc_get_page(flags);
    }

    // 페이지 초기화 후 삽입
    new->paddr = page;
    new->thread = thread_current();
    // lock 걸어주기
    lock_acquire(&swap_lock);
    list_push_back(&swap_list, &new->swap);
    lock_release(&swap_lock);
    return new;
}
void free_page(void *paddr)
{

    lock_acquire(&swap_lock);
    // 해당하는 paddr을 가진 page탐색
    struct page *page;
    struct list_elem *list = list_begin(&swap_list);
    while (1)
    {
        page = list_entry(list, struct page, swap);
        if (page->paddr == paddr)
        {
            delete_swap_list(page);
            pagedir_clear_page(page->thread->pagedir, pg_round_down(page->page_ve->vaddr));
            palloc_free_page(page->paddr);

            free(page);
            break;
        }
        if (list == list_end(&swap_list))
        {
            break;
        }
        list = list_next(list);
    }
    lock_release(&swap_lock);
}

// swap_list안에서의 이동-> 다음 elem 반환
static struct list_elem *move_swap_list()
{
    struct list_elem *ret_elem;
    lock_acquire(&swap_lock);
    if (list_empty(&swap_list))
    {
        return NULL;
    }
    // 초기상태
    if (swap_victim == NULL || swap_victim == list_end(&swap_list))
    {
        // list의 끝이면
        ret_elem = list_begin(&swap_list);
        lock_release(&swap_lock);
        return ret_elem;
        // return list_begin(swap_victim);
    }

    // list의 끝이면
    if (list_next(swap_victim) == list_end(&swap_list))
    {
        ret_elem = list_begin(&swap_list);
        lock_release(&swap_lock);
        return ret_elem;
        // return list_begin(swap_victim);
    }
    else
    {
        ret_elem = list_next(&swap_list);
        lock_release(&swap_lock);
        return ret_elem;

        // return list_next(swap_victim);
    }
    // swap_victim;
}

// 페이지 공간 확보
void evict_page(enum palloc_flags flags)
{

    // lock_acquire(&swap_lock);

    if (!list_empty(&swap_list))
    {

        swap_victim = move_swap_list();

        // page의 reference bit check
        struct page *page = list_entry(swap_victim, struct page, swap);

        // bit이 1로 되어있으면
        while (pagedir_is_accessed(page->thread->pagedir, page->page_ve->vaddr))
        {

            // 0으로 설정
            pagedir_set_accessed(page->thread->pagedir, page->page_ve->vaddr, false);
            swap_victim = move_swap_list();
            page = list_entry(swap_victim, struct page, swap);
        }

        // 제거할 page
        struct page *victim = page;

        // page type에 따라
        switch (victim->page_ve->type)
        {

        case VM_BIN:
            // dirty bit 검사 -> 1 이면 swap에 저장
            if (pagedir_is_dirty(victim->thread->pagedir, victim->page_ve->vaddr))
            {
                victim->page_ve->swap_arr = swap_out(victim->paddr);
                victim->page_ve->type = VM_ANON; // for demand paging
            }
            break;
        case VM_ANON:
            // swap에 저장
            victim->page_ve->swap_arr = swap_out(victim->paddr);
            break;
        }
        // 메모리 해제
        victim->page_ve->phy_loaded = false;

        delete_swap_list(page);
        pagedir_clear_page(page->thread->pagedir, pg_round_down(page->page_ve->vaddr));
        palloc_free_page(page->paddr);

        free(page);
    }
    // lock_release(&swap_lock);
}
