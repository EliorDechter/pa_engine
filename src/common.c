#ifndef PR_COMMON
#define PR_COMMON

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>

typedef uint8_t u8; 
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

typedef float f32;
typedef double f64;

#define null 0

#define standrad_name_length 50

#define align(x) __attribute__ ((aligned(x))) //use stdalign.h?

#define array_count(x) (sizeof(x) / sizeof(x[0]))

#define EPSILON 1e-5f
#define PI 3.1415927f

#define TO_RADIANS(degrees) ((PI / 180) * (degrees))
#define TO_DEGREES(radians) ((180 / PI) * (radians))

size_t convert_gibibytes_to_bytes(f32 size) {
    return (size_t)(size * (f32)pow(1024, 3));
}

#include "memory_allocator.c"

static unsigned char float_to_uchar(float value) {
    return (unsigned char)(value * 255);
}

typedef struct Buffer {
    u8 *data;
    u32 size;
    u32 max_size;
} Buffer;

typedef Buffer Array;

const char *load_file_to_buffer(const char  *file_name) {
    FILE *file = fopen(file_name, "r");
    assert(file);
    
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *data = malloc(size + 1);
    fread(data, size, 1, file);
    
    data[size] = '\0';
    
    fclose(file);
    
    return data;
}

#if 0

void fatal(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("FATAL: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    exit(1);
}

void *xcalloc(s32 num, s32 size) {
    void *ptr = calloc(num, size);
    if (!ptr) {
        perror("xcalloc failed");
        exit(1);
    }
}

void *xrealloc(void *ptr, s32 num_bytes) {
    ptr = realloc(ptr, num_bytes);
    if (!ptr) {
        perror("xcalloc failed");
        exit(1);
    }
    
    return ptr;
}

void *xmalloc(s32 num_bytes) {
    void *ptr = malloc(num_bytes);
    if (!ptr) {
        perror("xmalloc failed");
        exit(1);
    }
    
    return ptr;
}

void *memdup(void *src, s32 size) {
    void *dest = xmalloc(size);
    memcpy(dest, src, size);
    
    return dest;
}

char *strf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    s32 n = 1 + vsnprintf(null, 0, fmt, args);
    va_end(args);
    char *str = xmalloc(n);
    va_start(args, fmt);
    vsnprintf(str, n, fmt, args);
    va_end(args);
    
    return str;
}

#define arena_alignment 8
#define arena_block_size (1024 * 1024)

void arena_grow(Arena *arena, s32 min_size) {
    s32 size = align_up(clamp_min(min_size, arena_block_size), arena_alignment);
    arena->ptr = xmalloc(size);
    assert(arena->ptr == align_down_ptr(arena->ptr, arena_alignment));
    arena->end = arena->ptr + size;
    buf_push(arena->blocks, arena->ptr);
}

void *arena_alloc(Arena *arena, s32 size) {
    if (size > arena->end - arena->ptr)) {
        arena_grow(arena, size);
        assert(size <= (s32)(arena->end - arena->ptr));
    }
    
    void *ptr = arena->ptr;
    arena->ptr = align_up_ptr(arena->ptr + size, arena_alignment);
    assert(arena->ptr <= arena->end);
    assert(ptr == ALIGN_DOWN_PTR(ptr, ARENA_ALIGNMENT));
    
    return ptr;
}

bool write_file(const char *path, const char *buf, s32 len) {
    FILE *file = fopen(path, "w");
    if (!file) {
        return false;
    }
    
    s32 n = fwrite(buf, len, 1, file);
    fclose(file);
    return n == 1;
}

#endif

bool is_literal_string_equal(const char *a, const char *b, u32 size) {
    if (!a || !b)
        return false;
    if (strncmp(a, b, size) == 0)
        return true;
    return false;
}

int get_random_number(int min, int max) {
    int result = rand() % (max - min + 1)   + min;
    
    return result;
}

bool copy_string_to_buffer(const char *str, char *buffer, u32 buffer_size) {
    if (!str || !buffer || strlen(str) + 1 >=  buffer_size ) {
        return false;
    }
    strcpy(buffer, str);
    
    return true;
}

#endif
