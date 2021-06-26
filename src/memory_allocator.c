
#if DEBUG_MODE
#define max_num_frame_allocations 10000
#endif

typedef enum Allocation_type {
    perm_alloc,
    frame_alloc
} Allocation_type;

typedef struct Allocator {
    u8 *perm_memory_base;
    size_t perm_memory_used;
    size_t perm_memory_size;
    
    u8 *frame_memory_base;
    size_t frame_memory_used;
    size_t frame_memory_size;
    
#if DEBUG_MODE
    u8 *frame_allocations[max_num_frame_allocations];
    u32 num_frame_allocations;
#endif
    
    bool init;
} Allocator;

Allocator g_allocator;

u8 *allocate_perm_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    assert(allocator->init);
    assert(allocation_size >= 0);
    
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->perm_memory_used <= allocator->perm_memory_size);
    //memory += offset;
    allocator->perm_memory_used += allocation_size;
    
    return memory + offset;
}

u8 *allocate_frame_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    assert(allocator->init);
    assert(allocation_size >= 0);
    
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->frame_memory_base + allocator->frame_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->frame_memory_used <= allocator->frame_memory_size);
    //memory += offset;
    allocator->frame_memory_used += allocation_size;
    
    return memory + offset;
}

u8 *allocate_perm(size_t allocation_size) {
    assert(g_allocator.init);
    assert(allocation_size > 0);
    
#if DEBUG_MODE
    u8 *memory = malloc(allocation_size);
#else
    u8 *memory = g_allocator.perm_memory_base + g_allocator.perm_memory_used;
    
    uintptr_t alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + g_allocator.perm_memory_used <= g_allocator.perm_memory_size);
    assert(memory + allocation_size <= g_allocator.perm_memory_base + g_allocator.perm_memory_size);
    
    g_allocator.perm_memory_used += allocation_size;
#endif
    
    return memory;
}

u8 *allocate_frame(size_t allocation_size) {
    
    assert(g_allocator.init);
    assert(allocation_size > 0);
    
#if !DEBUG_MODE
    u8 *memory = g_allocator.frame_memory_base + g_allocator.frame_memory_used;
    
    uintptr_t alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = alignment - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + g_allocator.frame_memory_used <= g_allocator.frame_memory_size);
    
    g_allocator.frame_memory_used += allocation_size;
#else
    u8 *memory = malloc(allocation_size);
    g_allocator.frame_allocations[g_allocator.num_frame_allocations++] = memory;
#endif
    
    return memory;
}

void clear_frame_memory() {
    assert(g_allocator.init);
    
#if !DEBUG_MODE
    g_allocator.frame_memory_used = 0;
#else
    for (int i = 0; i < g_allocator.num_frame_allocations; ++i) {
        free(g_allocator.frame_allocations[i]);
    }
    g_allocator.num_frame_allocations = 0;
#endif
}

void init_allocator(float perm_memory_size_in_gigabytes, float frame_memory_size_in_gigabytes) {
#if !DEBUG_MODE
    size_t perm_memory_size = convert_gibibytes_to_bytes(perm_memory_size_in_gigabytes);
    size_t frame_memory_size =  convert_gibibytes_to_bytes(frame_memory_size_in_gigabytes);
    
    u8 *memory = malloc(perm_memory_size + frame_memory_size);
    assert(memory);
    
    g_allocator.perm_memory_size = perm_memory_size;
    g_allocator.frame_memory_size = frame_memory_size;
    g_allocator.perm_memory_base = memory;
    g_allocator.frame_memory_base = memory + perm_memory_size;
    g_allocator.frame_memory_used = 0;
    g_allocator.perm_memory_used = 0;
    //g_allocator.memory = memory;
#endif
    
    g_allocator.init = true;
    
}

void free_allocator() {
    //free(g_allocator);
}

void *allocate(size_t size, Allocation_type type) {
    u8 *result = null;
    if (type == perm_alloc) {
        result = allocate_perm(size);
    }
    else if (type == frame_alloc) {
        result = allocate_frame(size);
    }
    
    return result;
}

u8 *allocate_aligned(Allocation_type type, u8 size, u32 alignment) {
    u8 *result;
    if (type == perm_alloc) {
        result = allocate_perm_aligned(&g_allocator, size, alignment);
    }
    else if (type == frame_alloc) {
        result = allocate_frame_aligned(&g_allocator, size, alignment);
    }
    
    return result;
}

void *allocate_and_zero_out(size_t size, Allocation_type type) {
    u8 *result = allocate(size, type);
    memset(result, 0, size);
    
    return result;
}