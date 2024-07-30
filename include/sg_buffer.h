#pragma once
#include "sg_types.h"

typedef struct sg_allocator sg_allocator;

typedef struct sg_buffer
{
    sg_allocator* allocator;
    sg_u8* allocation;
    sg_u64 size;
} sg_buffer;

sg_buffer sg_buffer_create(sg_u64 size, sg_allocator* p_allocator);

void sg_buffer_destroy(sg_buffer* p_buffer);

void sg_buffer_resize(sg_buffer* p_buffer, sg_u64 size);

void* sg_buffer_data(sg_buffer* p_buffer, sg_u32 offset);

