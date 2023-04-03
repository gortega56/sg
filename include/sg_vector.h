#pragma once
#include "sg_types.h"

typedef struct sg_allocator
{
    void* (*allocate)(u64, void*);
    void  (*free)(void*, void*);
    void* (*realloc)(void*, u64, void*);  
    void* p_user_data;
} sg_allocator;


typedef struct sg_buffer
{
    sg_allocator* _allocator;
    u8* _allocation;
    u64 _size;
} sg_buffer;

sg_buffer sg_buffer_create(u64 size, sg_allocator* p_allocator);

void sg_buffer_destroy(sg_buffer* p_buffer);

void* sg_buffer_data(sg_buffer* p_buffer, u32 offset);

void sg_buffer_expand(sg_buffer* p_buffer, u64 size);


typedef struct sg_slice
{
    u8* _data;
    u64 _size;
    u32 _count;
    u32 _stride;
} sg_slice;

sg_slice sg_slice_make(void* p_data, u32 count, u32 stride);

u32 sg_slice_size(sg_slice* p_slice);

void* sg_slice_data(sg_slice* p_slice);

sg_slice sg_slice_to_slice(sg_slice* p_slice, u32 offset, u32 count);

#define sg_slice_data_as(slice_ptr, type) (type*)sg_slice_data(slice_ptr)

#define sg_slice_at(slice_ptr, type, index) ((type*)sg_slice_data(slice_ptr))[index]


typedef struct sg_vector
{
    sg_buffer _buffer;
    u32 _capacity;
    u32 _size;
    u32 _stride;
} sg_vector;

sg_vector sg_vector_create(u32 size, u32 stride, sg_allocator* p_allocator);

void sg_vector_destroy(sg_vector* p_vector);

void sg_vector_resize(sg_vector* p_vector, u32 size);

void sg_vector_reserve(sg_vector* p_vector, u32 size);

void* sg_vector_emplace(sg_vector* p_vector);

u32 sg_vector_push(sg_vector* p_vector, void* p_element);

u32 sg_vector_size(sg_vector* p_vector);

u8 sg_vector_any(sg_vector* p_vector);

void* sg_vector_data(sg_vector* p_vector, u32 index);

sg_slice sg_vector_to_slice(sg_vector* p_vector, u32 offset, u32 size);


typedef struct sg_hash_table
{
    sg_allocator* p_allocator;
    u32* _keys;
    u8* _data;
    u32 _capacity;
    u32 _size;
    u32 _stride;
    float _load_factor;

} sg_hash_table;

sg_hash_table sg_hash_table_create(u32 size, u32 stride, float load_factor, sg_allocator* p_allocator);

void sg_hash_table_destroy(sg_hash_table* p_table);

u8 sg_hash_table_find(sg_hash_table* p_table, u32 key, void** pp_value);

void sg_hash_table_insert(sg_hash_table* p_table, u32 key, void* p_value);

void* sg_hash_table_insert_inline(sg_hash_table* p_table, u32 key);

void sg_hash_table_remove(sg_hash_table* p_table, u32 key);
