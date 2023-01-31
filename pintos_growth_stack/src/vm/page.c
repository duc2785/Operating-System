#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <round.h>
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
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

/* khởi tạo bảng băm */
void init_virtual(struct hash *virtual)
{
   
   hash_init(virtual, hash_func, compare_hash, NULL);
}

/* Hàm lấy giá trị băm */
static unsigned hash_func(const struct hash_elem *e, void *aux)
{
    // Khám phá cấu trúc virtual_entry
    struct virtual_entry *ve = hash_entry(e, struct virtual_entry, elem);
    // Trả về giá trị băm cho vaddr
    return hash_int(ve->vaddr);
}

/* So sánh kích thước của các phần tử băm */
static bool compare_hash(const struct hash_elem *a, const struct hash_elem *b,void *aux)
{
    // Trả về false nếu vaddr của a nhỏ hơn, trả về true nếu ngược lại.
    struct virtual_entry *vir_a = hash_entry(a, struct virtual_entry, elem);
    struct virtual_entry *vir_b = hash_entry(b, struct virtual_entry, elem);

    return vir_a->vaddr < vir_b->vaddr;
}

/* Chèn mục nhập ảo vào bảng băm */
bool insert_hash(struct hash *virtual, struct virtual_entry *ve)
{
    
    
    // hash_insert trả về giá trị nguyên trạng nếu giá trị tồn tại trong bảng băm, ngược lại trả về NULL sau khi chèn
    if (hash_insert(virtual, &ve->elem) == NULL)
    {
        //printf("success!\n");
        return true;
    }
    return false;
}

/* Xóa mục nhập ảo tương ứng khỏi bảng băm */
bool delete_hash(struct hash *virtual, struct virtual_entry *ve)
{

    // hash_delete trả về giá trị đã xóa sau khi xóa nếu giá trị tồn tại trong bảng băm, ngược lại trả về NULL.
    if (hash_delete(virtual, &ve->elem) == NULL)
    {
        return false;
    }
    free_page(pagedir_get_page(thread_current()->pagedir, ve->vaddr));
    free(ve);
    return true;
}

// Quay lại sau khi tìm kiếm mục nhập ảo tương ứng với vaddr
struct virtual_entry *search_ve(void *vaddr)
{

    struct virtual_entry ve;
    // vaddr là một con trỏ, gán giá trị cho vaddr sẽ cấp phát toàn bộ cấu trúc theo cùng một cách
    ve.vaddr = pg_round_down(vaddr);
    //printf("search:%d\n", ve.vaddr);
    // Hàm hash_find tìm kiếm hàm băm cho phần tử khớp với hàm băm.
    struct hash_elem *elems = hash_find(&thread_current()->virtual, &ve.elem);

    // Nếu nó tồn tại, trả về mục nhập của hàm băm nếu không trả về NULL
    if (elems == NULL){
        
        return NULL;
    }
    else{
        return hash_entry(elems, struct virtual_entry, elem);
    }
}

// Xóa bảng băm
void kill_hash(struct hash *ve)
{

    // Hàm hash_destroy xóa hash_table.
    hash_destroy(ve, kill_func);
}

// kill_func cho hàm hash_destroy
static void kill_func(struct hash_elem *e, void *aux)
{

    struct virtual_entry *ve = hash_entry(e, struct virtual_entry, elem);

    // Nếu được nạp vào bộ nhớ vật lý
    if (ve->phy_loaded)
    {
        // xóa trang
        void *paddr = pagedir_get_page(thread_current()->pagedir, ve->vaddr);
        free_page(paddr);
        pagedir_clear_page(thread_current()->pagedir, ve->vaddr);
    }
    free(ve);
}

// Kiểm tra địa chỉ lỗi và kiểm tra sự phát triển của ngăn xếp
struct virtual_entry *check_and_growth(void *addr, void *esp)
{
    
    // Kiểm tra xem đó có phải là khu vực ngăn xếp của người dùng không
    if (addr < (void*)0x8048000 || addr >= (void*)0xc0000000)
    {
        //printf("not user stack\n");
        exit(-1);
    }
    // kiểm tra xem nó có trong bộ nhớ ảo không
    struct virtual_entry *ve = search_ve(addr);
    //printf("search done\n");
    if (ve != NULL)
    {
        // trả về nếu tồn tại, không cần tăng
        //printf("ve!=null\n");
        return ve;
    }
    //printf("error\n");
    // kiểm tra sự tăng trưởng ngăn xếp
    bool growth_check = check_addr(addr,esp);
    //printf("growth check done with %d\n", growth_check);
    if (growth_check)
    {
        //printf("success growth check\n");
        stack_growth(addr);
        ve = search_ve(addr);
    }
    else
    {
        //printf("fail\n");
        exit(-1);
    }
    return ve;
}

bool check_addr(void *addr, void *esp){
    
    return (is_user_vaddr(pg_round_down(addr)) && addr>=esp - 32 && addr >= (PHYS_BASE - 8*1024*1024));
}
// tải một tập tin từ đĩa vào bộ nhớ vật lý
bool load_from_disk(void *addr, struct virtual_entry *ve)
{
    if ((int)(ve->read_bytes) != file_read_at(ve->file, addr, ve->read_bytes, ve->offset))
    {
        //printf("%d %d\n",(int)(ve->read_bytes),file_read_at(ve->file, addr, ve->read_bytes, ve->offset));
        return false;
    }
    else
    {
        memset(addr + ve->read_bytes, 0, ve->zero_bytes);
    }
    return true;
}

    


