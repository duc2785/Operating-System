#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

#define VM_BIN 0 // 바이너리 파일로부터 데이터 로드
#define VM_ANON 1 // swapping된 영역에서 로드
#define MAX_STACK_SIZE (1 << 23)

struct virtual_entry{

    uint8_t type;  // 위에 정의된 entry type
    void *vaddr;  // virtaul address
    bool ok_write; // writable?
    bool phy_loaded;  // physical memory에 있는지 확인

    struct file* file; // mapping된 파일

    size_t offset; // 읽을 파일 크기
    size_t read_bytes; // vm에 쓰인 데이터 크기
    size_t zero_bytes; // 남은 페이지 크기

    size_t swap_arr; 

    struct hash_elem elem; 
};

struct page{

    void *paddr; // page의 물리주소
    struct virtual_entry *page_ve; // page에 mapping된 virtual entry
    struct thread *thread; // 해당 thread
    struct list_elem swap; // swap algorithm을 위한 elem
};

void init_virtual(struct hash *virtual);
static unsigned hash_func(const struct hash_elem *e, void *aux);
static bool compare_hash(const struct hash_elem *a, const struct hash_elem *b, void *aux);
bool insert_hash(struct hash *virtual, struct virtual_entry *ve);
bool delete_hash(struct hash *virtual, struct virtual_entry *ve);
struct virtual_entry *search_ve(void *vaddr);
void kill_hash(struct hash *ve);
static void kill_func(struct hash_elem *e, void *aux);
struct virtual_entry* check_and_growth(void *addr, void *esp);
//bool handle_mm_fault(struct virtual_entry *ve);
bool load_from_disk(void *addr, struct virtual_entry *ve);
bool check_addr(void *addr, void *esp);
#endif