#include "os.h"

extern void *page_alloc(int n);
extern void page_free(void *ptr);

// | header | data | footer |
#define BLOCK_USED      (uint32_t)(1 << 0)              // 第0位表示有没有被使用
#define BLOCK_PREV_USED (uint32_t)(1 << 1)              // 第1位表示前一个块有没有被使用
#define BLOCK_FLAG (BLOCK_USED | BLOCK_PREV_USED)       // header 和 footer 的最后两位表示块的状态

// 获取block header的地址，header的大小为4字节
static inline void *_block_get_header(void *block_ptr) {
    return block_ptr - sizeof(uint32_t);
}

// 获取 header 的最后一位，表示块是否已经被使用
static inline int _block_is_used(void *block_ptr) {
    void *block_header = block_ptr - sizeof(uint32_t);
    return *(uint32_t *)block_header & BLOCK_USED;
}

// 设置 header 的最后一位为 1，表示块已经被使用
static inline void _block_set_used(void *block_ptr) {
    void *block_header = block_ptr - sizeof(uint32_t);
    *(uint32_t *)block_header |= BLOCK_USED;
}

// 设置 header 的最后一位为 0，表示块没有被使用
static inline void _block_set_unused(void *block_ptr) {
    void *block_header = block_ptr - sizeof(uint32_t);
    *(uint32_t *)block_header &= ~BLOCK_USED;
}

// header 去除最后一位后，就是 block 的大小，单位是 4 字节
static inline size_t _block_get_size(void *block_ptr) {
    void *block_header = block_ptr - sizeof(uint32_t);
    return *(uint32_t *)block_header & ~BLOCK_FLAG;
}

// 设置 block 的大小，单位是 4 字节
static inline void _block_set_size(void *block_ptr, size_t size) {
    void *block_header = block_ptr - sizeof(uint32_t);
    *(uint32_t *)block_header = size | (*(uint32_t *)block_header & BLOCK_FLAG);
}

// 获取整个块的大小，包括 header 和 footer，4 字节对齐
static inline size_t _get_alloc_size(size_t size) {
    size_t alloc_size = size + sizeof(uint32_t) * 2; // header + footer
    if (alloc_size % 4 != 0) {
        alloc_size += 4 - alloc_size % 4;
    }
    return alloc_size;
}

// 获取 block 的 footer 的地址
static inline void *_block_get_footer(void *block_ptr) {
    return block_ptr + _block_get_size(block_ptr) - sizeof(uint32_t) * 2;
}

// 设置 block 的 footer 的内容，与 header 的内容相同
static inline void *_block_set_footer(void *block_ptr) {
    void *footer = _block_get_footer(block_ptr);
    void *header = _block_get_header(block_ptr);
    *(uint32_t *)footer = *(uint32_t *)header;
    return footer;
}

// 获取 block 的下一个 block 的地址
static inline void *_block_get_next(void *block_ptr) {
    return block_ptr + _block_get_size(block_ptr);
}

// 获取 block 的上一个 block 的地址
static inline void *_block_get_prev(void *block_ptr) {
    void *prev_footer = block_ptr - sizeof(uint32_t) * 2;
    size_t prev_block_size = *(uint32_t *)prev_footer & ~BLOCK_FLAG;
    return block_ptr - prev_block_size;
}

/*
  内部维护一个页，记录当前页已经分配的情况，每次分配都从这个页中分配
  如果这个页不够用了，用 page_alloc 分配一个新的页
*/
#define PAGE_SIZE 4096
void *_mm_start = NULL;
void *_mm_end = NULL;
void *_mm_start_block = NULL;
void *_mm_end_block = NULL;

// 向 page 申请 1 个 page，返回 page 的起始地址
void block_init() {
    _mm_start = page_alloc(1);
    _mm_end = _mm_start + PAGE_SIZE; // _mm_end 为页的结束地址 + 4

    printf("block_init: _mm_start: %p, _mm_end: %p\n", _mm_start, _mm_end);

    // _mm_start 为 header 的地址，_mm_start_block 为第一个 block
    // 的地址，_mm_end_block 为最后一个 block 的地址
    _mm_start_block = _mm_start + sizeof(uint32_t);
    _mm_end_block = _mm_end - sizeof(uint32_t);

    printf("block_init: first_block: %p, last_block: %p\n", _mm_start_block, _mm_end_block);

    _block_set_size(_mm_start_block, PAGE_SIZE - sizeof(uint32_t) * 2);
    _block_set_unused(_mm_start_block);
    _block_set_footer(_mm_start_block);

    printf("block_init: first_block: %p, size: %d, used: %d\n", _mm_start_block,
                                                                (int)_block_get_size(_mm_start_block), 
                                                                _block_is_used(_mm_start_block));

    _block_set_size(_mm_end_block, sizeof(uint32_t) * 2);
    _block_set_used(_mm_end_block);
    _block_set_footer(_mm_end_block);

    printf("block_init: last_block: %p, size: %d, used: %d\n", _mm_end_block,
                                                                (int)_block_get_size(_mm_end_block), 
                                                                _block_is_used(_mm_end_block));

    // printf("next block: %p\n", _block_get_next(_mm_end_block));

}

// 最多分配 64 M 内存，超过这个大小的分配失败
#define MAX_MEM_ALLOC 64 * 1024 * 1024
// 每个内存块有一个 header，记录这个内存块的大小，以及是否已经分配
// 每个内存块有一个 footer，与 header 内容相同，合并内存块时使用

void mm_print_blocks() {
    void *block_ptr = _mm_start_block;
    printf("-- start to print blocks --\n");
    while (block_ptr <= _mm_end_block) {
        printf("\tblock: %p, size: %d, used: %d\n", block_ptr,
                                                    (int)_block_get_size(block_ptr), 
                                                    _block_is_used(block_ptr));
        block_ptr = _block_get_next(block_ptr);
    }
    printf("-- end to print blocks --\n");
}

void *block_alloc(size_t size) {
    // printf("malloc size: %d\n", (int)size);
    size_t alloc_size = _get_alloc_size(size);
    if (alloc_size > MAX_MEM_ALLOC) {
        printf("malloc too large: %d\n", (int)alloc_size);
        return NULL;
    }

    void *block_ptr = _mm_start_block;
    // 遍历所有的 block，找到第一个满足大小的 block，first fit 策略
    int found = 0;
    while (block_ptr < _mm_end_block) {
        if (!_block_is_used(block_ptr) && _block_get_size(block_ptr) >= alloc_size) {
            found = 1;
            break;
        }
        block_ptr = _block_get_next(block_ptr);
    }
    if (!found) {
        // printf("malloc failed: %d\n", (int)alloc_size);
        // return NULL;
        // 如果没有找到满足大小的 block，申请一个新的页
        void *cur_end = _mm_end;
        int num_of_new_alloc_pages = 0;
        while (!found) {
            void *new_page = page_alloc(1);
            num_of_new_alloc_pages++;
            if (new_page == NULL) {     // 如果申请失败，回收已经申请的页，返回 NULL
                printf("malloc failed: %d, already alloc %d pages\n", (int)alloc_size,
                    num_of_new_alloc_pages);
                // 找到 _mm_end_block 前一个 block，将这个 block 设置为最后一个 block
                void *last_block_ptr = _mm_start_block;
                last_block_ptr = _block_get_prev(_mm_end_block);
                _mm_end_block = cur_end - sizeof(uint32_t);
                _block_set_size(_mm_end_block, sizeof(uint32_t) * 2);
                _block_set_used(_mm_end_block);
                _block_set_footer(_mm_end_block);
                if (last_block_ptr != _mm_end_block) {
                    size_t last_block_size = _mm_end_block - last_block_ptr;
                    _block_set_size(last_block_ptr, last_block_size);
                    _block_set_footer(last_block_ptr);
                }
                // 回收新申请的页，恢复到 cur_end
                while (_mm_end > cur_end) {
                    _mm_end -= PAGE_SIZE;
                    page_free(_mm_end);
                }
                if (_mm_end_block != _mm_end - sizeof(uint32_t)) {
                    panic("_mm_end_block != _mm_end - sizeof(uint32_t)");
                }
                return NULL;
            }
            if (new_page != _mm_end) {
                panic("new_page != _mm_end");
            }

            _mm_end += PAGE_SIZE;
            // 尝试与当前的最后一个 block 合并
            void *last_block_ptr = _block_get_prev(_mm_end_block);
            if (!_block_is_used(last_block_ptr)) {
                size_t last_block_size = _block_get_size(last_block_ptr);
                _block_set_size(last_block_ptr, last_block_size + PAGE_SIZE);
                _block_set_unused(last_block_ptr);
                _block_set_footer(last_block_ptr);
            } else {
                last_block_ptr = _mm_end_block;
                _block_set_size(last_block_ptr, PAGE_SIZE);
                _block_set_unused(last_block_ptr);
                _block_set_footer(last_block_ptr);
            }
            // 设置新的 _mm_end_block
            _mm_end_block = _mm_end - sizeof(uint32_t);
            _block_set_size(_mm_end_block, sizeof(uint32_t) * 2);
            _block_set_used(_mm_end_block);
            _block_set_footer(_mm_end_block);
            // printf("alloc a new page: %p\n", new_page);
            // 检查新分配的块是否满足要求
            if (!_block_is_used(last_block_ptr) && _block_get_size(last_block_ptr) >= alloc_size) {
                found = 1;
                block_ptr = last_block_ptr;
                break;
            }
        }
        // printf("alloc %d pages\n", num_of_new_alloc_pages);
    }
    // 找到满足大小的 block，分配这个 block
    _block_set_used(block_ptr);
    _block_set_footer(block_ptr);
    size_t block_size = _block_get_size(block_ptr);
    if (block_size > alloc_size) {
        // 如果这个 block 大于需要的大小，分割这个 block
        _block_set_size(block_ptr, alloc_size);
        _block_set_used(block_ptr);
        _block_set_footer(block_ptr);
        void *next_block_ptr = _block_get_next(block_ptr);
        _block_set_size(next_block_ptr, block_size - alloc_size);
        _block_set_unused(next_block_ptr);
        _block_set_footer(next_block_ptr);
    }
    return block_ptr;
}

void block_free(void *ptr) {
    // printf("free: %p\n", ptr);
    if (ptr == NULL) {
        return;
    }
    _block_set_unused(ptr);
    
    // 尝试与前后的 block 合并

    // 获取下一个 block 的地址，大小，是否被使用
    void *next_block_ptr = _block_get_next(ptr);
    size_t next_block_size = _block_get_size(next_block_ptr);
    int next_block_used = _block_is_used(next_block_ptr);
    // printf("next_block_ptr: %p, size: %d, used: %d\n", next_block_ptr,
    // (int)next_block_size, next_block_used);
    
    // 如果下一个 block 没有被使用，合并这两个 block
    if (next_block_ptr != ptr && next_block_ptr <= _mm_end_block && !next_block_used) {
        _block_set_size(ptr, _block_get_size(ptr) + next_block_size);
        _block_set_footer(ptr);
    }

    // 获取上一个 block 的地址，大小，是否被使用
    void *prev_block_ptr = _block_get_prev(ptr);
    size_t prev_block_size = _block_get_size(prev_block_ptr);
    int prev_block_used = _block_is_used(prev_block_ptr);
    // printf("prev_block_ptr: %p, size: %d, used: %d\n", prev_block_ptr,
    // (int)prev_block_size, prev_block_used);

    // 如果上一个 block 没有被使用，合并这两个 block
    if (prev_block_ptr != ptr && prev_block_ptr >= _mm_start_block && !prev_block_used) {
        _block_set_size(prev_block_ptr, _block_get_size(ptr) + prev_block_size);
        _block_set_footer(prev_block_ptr);
    }
}

struct test_struct {
    int a;
    int b;
    int c;
};

void mm_test() {
    printf("mm_test:\n");
    // test alloc too large
    mm_print_blocks();
    void *p = block_alloc(MAX_MEM_ALLOC + 1);
    printf("malloc too large: %p\n", p);
    struct test_struct *test = block_alloc(sizeof(struct test_struct));
    printf("test_struct: %p\n", test);
    test->a = 1;
    test->b = 2;
    test->c = 3;
    printf("test_struct: %d %d %d\n", test->a, test->b, test->c);
    mm_print_blocks();
    void *block_4096 = block_alloc(4096);
    printf("block_4096: %p\n", block_4096);
    mm_print_blocks();
    void *test_2 = block_alloc(sizeof(struct test_struct));
    printf("test_2: %p\n", test_2);
    mm_print_blocks();
    void *block_4000 = block_alloc(4000);
    printf("block_4000: %p\n", block_4000);
    mm_print_blocks();
    void *block_4096x3 = block_alloc(4096 * 3);
    printf("block_4096x3: %p\n", block_4096x3);
    mm_print_blocks();

    printf("\n start to test free\n");
    block_free(test_2);
    printf("free test_2\n");
    mm_print_blocks();
    block_free(block_4000);
    printf("free block_4000\n");
    mm_print_blocks();
    block_free(block_4096);
    printf("free block_4096\n");
    mm_print_blocks();
    block_free(block_4096x3);
    printf("free block_4096x3\n");
    mm_print_blocks();
    block_free(test);
    printf("free test\n");
    mm_print_blocks();

    // test alloc too large
    printf("\n start to test alloc too large\n");
    void *block_32M = block_alloc(32 * 1024 * 1024);
    printf("block_32M: %p\n", block_32M);
    mm_print_blocks();
    void *block_32M_2 = block_alloc(32 * 1024 * 1024);
    printf("block_32M_2: %p\n", block_32M_2);
    mm_print_blocks();
    void *block_32M_3 = block_alloc(32 * 1024 * 1024);
    printf("block_32M_3: %p\n", block_32M_3);
    mm_print_blocks();
    // make last used
    void *block_4056 = block_alloc(4056);
    printf("block_4056: %p\n", block_4056);
    mm_print_blocks();
    void *block_32M_4 = block_alloc(32 * 1024 * 1024);
    printf("block_32M_4: %p\n", block_32M_4);
    mm_print_blocks();

    // test free
    printf("\n start to test free\n");
    block_free(block_32M_4);
    printf("free block_32M_4\n");
    mm_print_blocks();
    block_free(block_32M_3);
    printf("free block_32M_3\n");
    mm_print_blocks();
    block_free(block_32M_2);
    printf("free block_32M_2\n");
    mm_print_blocks();
    block_free(block_32M);
    printf("free block_32M\n");
    mm_print_blocks();
    block_free(block_4056);
    printf("free block_4056\n");
    mm_print_blocks();

    panic("test end\n");
}