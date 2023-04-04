#pragma once
#include "sg_types.h"

typedef struct sg_allocator sg_allocator;

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
