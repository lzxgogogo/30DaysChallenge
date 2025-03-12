// mempool_test.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 包含内存池头文件（假设上述代码保存为 mempool.h）
// #include "mempool.h"

// 为了简化演示，直接包含内存池实现代码（实际项目应分开）
// 这里直接使用用户提供的原始代码结构

#define MEM_PAGE_SIZE 4096

typedef struct mp_node_s {
    char *free_ptr;
    char *end;
    struct mp_node_s *next;
} mp_node_t;

typedef struct mp_pool_s {
    struct mp_node_s *first;
    struct mp_node_s *current;
    int max;
} mp_pool_t;

// 函数原型
int mp_init(mp_pool_t *m, int size);
void mp_dest(mp_pool_t *m);
void *mp_alloc(mp_pool_t *m, int size);
int mp_init(mp_pool_t *m, int size)
{
    if (!m)
        return -1;
    if (size < sizeof(mp_node_t))
        return -1; // 内存不足以存放管理结构

    void *addr = malloc(size); // 4096
    mp_node_t *node = (mp_node_t *)addr;

    node->free_ptr = (char *)addr + sizeof(mp_node_t);
    node->end = (char *)addr + size;
    node->next = NULL;

    m->first = node;
    m->current = node;
    m->max = size;

    return 0;
}

void mp_dest(mp_pool_t *m)
{
    if (!m)
        return ;

    while (!m->first)
    {
        void *addr = m->first;
        mp_node_t *node = (mp_node_t *)addr;

        m->first = node->next;
        free(addr);
    }
}

void *mp_alloc(mp_pool_t *m, int size)
{
    void *addr = m->current;
    mp_node_t *node = (mp_node_t *)addr;

    do
    {
        if (size <= (node->end - node->free_ptr))
        {
            char *ptr = node->free_ptr;
            node->free_ptr += size;

            return ptr;
        }
        node = node->next;

    } while (node);
    
    //new node 

    addr = malloc(m->max); // 4096
    node = (mp_node_t*) addr;

    node->free_ptr = (char*)addr + sizeof(mp_node_t);
    node->end = (char*) addr + m->max;
    
    node->next = m->current;
    m->current = node;
    
    char *ptr = node->free_ptr;
    node->free_ptr += size;

    return ptr;
}

void mp_free()
{
}

// 测试用例1：基本分配测试
void test_basic_allocation() {
    printf("=== Running Basic Allocation Test ===\n");
    mp_pool_t pool;
    
    // 初始化内存池（使用标准页大小）
    int ret = mp_init(&pool, MEM_PAGE_SIZE);
    assert(ret == 0 && "Initialization failed");
    
    // 第一次分配
    void *p1 = mp_alloc(&pool, 64);
    printf("Allocated 64 bytes at %p\n", p1);
    assert(p1 != NULL);
    
    // 第二次分配
    void *p2 = mp_alloc(&pool, 128);
    printf("Allocated 128 bytes at %p\n", p2);
    assert(p2 != NULL);
    assert((char*)p2 - (char*)p1 >= 64 && "Memory overlap detected");
    
    // 清理内存池
    mp_dest(&pool);
    printf("Test passed!\n\n");
}

// 测试用例2：跨页分配测试
void test_cross_page_allocation() {
    printf("=== Running Cross-Page Allocation Test ===\n");
    mp_pool_t pool;
    mp_init(&pool, MEM_PAGE_SIZE);
    
    // 计算单个页的可用空间
    size_t header_size = sizeof(mp_node_t);
    size_t available = MEM_PAGE_SIZE - header_size;
    
    // 分配到刚好不超过当前页
    void *p1 = mp_alloc(&pool, available - 64);
    printf("Allocated %zu bytes at %p\n", available-64, p1);
    
    // 下一个分配应该触发新页分配
    void *p2 = mp_alloc(&pool, 128);
    printf("Allocated 128 bytes at %p (new page)\n", p2);
    
    // 验证地址差异
    size_t page_diff = (char*)p2 - (char*)p1;
    assert(page_diff > MEM_PAGE_SIZE && "Should be in different pages");
    
    mp_dest(&pool);
    printf("Test passed!\n\n");
}

// 测试用例3：边界条件测试
void test_edge_cases() {
    printf("=== Running Edge Case Tests ===\n");
    mp_pool_t pool;
    
    // 测试初始化最小尺寸
    int ret = mp_init(&pool, sizeof(mp_node_t));
    assert(ret == 0 && "Minimum size initialization failed");
    
    // 尝试分配（应该失败并创建新页）
    void *p = mp_alloc(&pool, 1);
    assert(p != NULL && "Should allocate new page");
    
    // 测试超大分配（超过页尺寸）
    void *big = mp_alloc(&pool, MEM_PAGE_SIZE*2);
    printf("Allocated %d bytes at %p\n", MEM_PAGE_SIZE*2, big);
    assert(big != NULL && "Should handle oversized allocation");
    
    mp_dest(&pool);
    printf("Test passed!\n\n");
}

// 测试用例4：压力测试
void stress_test() {
    printf("=== Running Stress Test ===\n");
    mp_pool_t pool;
    mp_init(&pool, MEM_PAGE_SIZE);
    
    // 分配1000次随机大小
    void *ptrs[1000];
    for(int i=0; i<1000; i++) {
        size_t size = (rand() % 256) + 1; // 1-256字节
        ptrs[i] = mp_alloc(&pool, size);
        assert(ptrs[i] != NULL);
        
        // 写入测试数据
        memset(ptrs[i], 0xAA, size);
    }
    
    printf("Allocated 1000 blocks successfully\n");
    mp_dest(&pool);
    printf("Test passed!\n\n");
}

int main() {
    // 运行测试套件
    test_basic_allocation();
    test_cross_page_allocation();
    test_edge_cases();
    stress_test();
    
    printf("All tests completed successfully!\n");
    return 0;
}