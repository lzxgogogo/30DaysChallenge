#include <stdio.h>
#include <stdlib.h>

#define MEM_PAGE_SIZE 4096

typedef struct mp_node_s
{
    char *free_ptr;
    char *end; // 哨兵, 仅作标识，不会实际访问
    struct mp_node_s *next;
} mp_node_t;

typedef struct mp_pool_s
{
    struct mp_node_s *first;
    struct mp_node_s *current;
    int max;
} mp_pool_t;

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

// int main()
// {
//     mp_pool_t m;
    
//     mp_init(&m, MEM_PAGE_SIZE);
    
//     void *p1 = mp_alloc(&m,1);
//     printf("1: mp_alloc: %p\n", p1);

//     void *p2 = mp_alloc(&m,2);
//     printf("2: mp_alloc: %p\n", p2);

//     void *p3 = mp_alloc(&m,8);
//     printf("3: mp_alloc: %p\n", p3);

//     void *p4 = mp_alloc(&m,16);
//     printf("4: mp_alloc: %p\n", p4);
    
//     void *p5 = mp_alloc(&m,4070);
//     printf("5: mp_alloc: %p\n", p5);

//     mp_dest(&m);
    
}