#include "memory.h"

// memory.c: Simple kernel heap allocator (Phase-2)
//
// Design:
// - Single fixed-size heap region located right after kernel image (__kernel_end).
// - Intrusive singly-linked free/used block list stored inside heap memory.
// - First-fit allocation.
// - Optional split when a free block is larger than requested size.
// - Free operation marks block free and coalesces adjacent free blocks.
//
// Notes:
// - This is intentionally simple and deterministic for early kernel phases.
// - No locking is used yet; allocator is expected to run in single-core context.

extern uint8_t __kernel_end;

#define KHEAP_SIZE_BYTES (512u * 1024u)
#define KHEAP_ALIGN      16u

typedef struct mem_block
{
    // Payload size in bytes (does not include metadata header).
    uint32_t size;
    // 1 = free block, 0 = allocated block.
    uint32_t free;
    // Next block in linear heap order.
    struct mem_block* next;
} mem_block_t;

// Heap boundaries and list head.
static uint8_t* g_HeapStart = 0;
static uint8_t* g_HeapEnd = 0;
static mem_block_t* g_BlockList = 0;

// Align a value upward to `align` bytes (align must be power-of-two).
static uint32_t align_up(uint32_t value, uint32_t align)
{
    return (value + (align - 1u)) & ~(align - 1u);
}

// Keep block headers aligned so payload pointers are aligned as well.
static uint32_t header_size_aligned(void)
{
    return align_up((uint32_t)sizeof(mem_block_t), KHEAP_ALIGN);
}

// Tiny memset(0) replacement for freestanding environment.
static void memory_zero(void* ptr, uint32_t size)
{
    uint8_t* p = (uint8_t*)ptr;
    for (uint32_t i = 0; i < size; i++)
        p[i] = 0;
}

// Merge neighboring free blocks to reduce fragmentation.
// Because list order follows memory order, adjacent list nodes are adjacent blocks.
static void memory_coalesce_all(void)
{
    mem_block_t* cur = g_BlockList;
    uint32_t hdr = header_size_aligned();

    while (cur && cur->next)
    {
        if (cur->free && cur->next->free)
        {
            cur->size += hdr + cur->next->size;
            cur->next = cur->next->next;
            continue;
        }

        cur = cur->next;
    }
}

// Initialize heap region and create one initial free block spanning the heap.
void memory_init(void)
{
    uint32_t heap_base = align_up((uint32_t)&__kernel_end, KHEAP_ALIGN);
    uint32_t hdr = header_size_aligned();

    g_HeapStart = (uint8_t*)heap_base;
    g_HeapEnd = g_HeapStart + KHEAP_SIZE_BYTES;

    g_BlockList = (mem_block_t*)g_HeapStart;
    g_BlockList->free = 1;
    g_BlockList->next = 0;
    g_BlockList->size = (KHEAP_SIZE_BYTES > hdr) ? (KHEAP_SIZE_BYTES - hdr) : 0;
}

// Allocate heap memory using first-fit strategy.
// Returns aligned payload pointer, or 0 on failure.
void* kmalloc(uint32_t size)
{
    uint32_t hdr = header_size_aligned();

    if (size == 0)
        return 0;

    size = align_up(size, KHEAP_ALIGN);

    if (g_HeapStart == 0 || g_HeapEnd == 0 || g_BlockList == 0)
        return 0;

    mem_block_t* cur = g_BlockList;
    while (cur)
    {
        if (cur->free && cur->size >= size)
        {
            // If current block is significantly larger than requested,
            // split it into [allocated block][remaining free block].
            if (cur->size >= (size + hdr + KHEAP_ALIGN))
            {
                uint8_t* raw_next = (uint8_t*)cur + hdr + size;
                mem_block_t* next = (mem_block_t*)raw_next;
                next->size = cur->size - size - hdr;
                next->free = 1;
                next->next = cur->next;
                cur->next = next;
                cur->size = size;
            }

            cur->free = 0;
            return (void*)((uint8_t*)cur + hdr);
        }

        cur = cur->next;
    }

    return 0;
}

// Allocate zero-initialized memory for an array.
void* kcalloc(uint32_t count, uint32_t size)
{
    if (count == 0 || size == 0)
        return 0;

    if (size > (0xFFFFFFFFu / count))
        return 0;

    uint32_t total = count * size;
    void* ptr = kmalloc(total);
    if (!ptr)
        return 0;

    memory_zero(ptr, total);
    return ptr;
}

// Free a previously allocated block.
// Invalid pointers are ignored to keep early kernel robust.
void kfree(void* ptr)
{
    uint32_t hdr = header_size_aligned();

    if (!ptr || g_BlockList == 0)
        return;

    uint8_t* p = (uint8_t*)ptr;
    if (p < g_HeapStart + hdr || p >= g_HeapEnd)
        return;

    mem_block_t* block = (mem_block_t*)(p - hdr);
    block->free = 1;
    memory_coalesce_all();
}

// Total heap bytes reserved for allocator (includes metadata overhead).
uint32_t memory_heap_total(void)
{
    if (g_HeapStart == 0 || g_HeapEnd == 0)
        return 0;

    return (uint32_t)(g_HeapEnd - g_HeapStart);
}

// Sum payload bytes currently allocated.
uint32_t memory_heap_used(void)
{
    if (g_BlockList == 0)
        return 0;

    uint32_t used = 0;
    mem_block_t* cur = g_BlockList;
    while (cur)
    {
        if (!cur->free)
            used += cur->size;
        cur = cur->next;
    }

    return used;
}

// Sum payload bytes currently available for future allocations.
uint32_t memory_heap_free(void)
{
    if (g_BlockList == 0)
        return 0;

    uint32_t free_bytes = 0;
    mem_block_t* cur = g_BlockList;
    while (cur)
    {
        if (cur->free)
            free_bytes += cur->size;
        cur = cur->next;
    }

    return free_bytes;
}

// Runtime allocator self-test:
// 1) allocate with kmalloc and kcalloc
// 2) verify write/read and zero-initialization
// 3) free blocks
// 4) confirm free space recovered
int memory_run_self_test(void)
{
    if (g_HeapStart == 0)
        return 0;

    uint32_t free_before = memory_heap_free();

    uint8_t* a = (uint8_t*)kmalloc(64);
    uint8_t* b = (uint8_t*)kcalloc(32, 4);

    if (!a || !b)
        return 0;

    for (uint32_t i = 0; i < 64; i++)
        a[i] = (uint8_t)(0xA0u + (i & 0x0Fu));

    for (uint32_t i = 0; i < 64; i++)
    {
        uint8_t expected = (uint8_t)(0xA0u + (i & 0x0Fu));
        if (a[i] != expected)
            return 0;
    }

    for (uint32_t i = 0; i < 128; i++)
    {
        if (b[i] != 0)
            return 0;
    }

    kfree(b);
    kfree(a);

    return memory_heap_free() >= free_before;
}
