#pragma once
#include "sg_types.h"
#include "sg_buffer.h"
#include "sg_slice.h"

typedef struct sg_allocator sg_allocator;

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
