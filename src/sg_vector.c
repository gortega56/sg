#include "sg_vector.h"
#include "sg_allocator.h"
#include "sg_assert.h"

static inline void memclear(void* p_data, u64 size)
{
    memset(p_data, 0U, size);
}

sg_vector sg_vector_create(u32 size, u32 stride, sg_allocator* p_allocator)
{
    SG_ASSERT(stride != 0);

    sg_buffer buffer = sg_buffer_create(stride * size, p_allocator);

    if (size != 0)
        memclear(buffer._allocation, stride * size);

    sg_vector vector;
    vector._buffer = buffer; 
    vector._capacity = buffer._size / stride;
    vector._size = size;
    vector._stride = stride;  
    return vector;
}

void sg_vector_destroy(sg_vector* p_vector)
{
    sg_buffer_destroy(&p_vector->_buffer);
    p_vector->_capacity = 0;
    p_vector->_size = 0;
    p_vector->_stride = 0;
}

void sg_vector_reserve(sg_vector* p_vector, u32 size)
{
    if (p_vector->_capacity < size)
    {
        sg_buffer_expand(&p_vector->_buffer, size * p_vector->_stride);
        p_vector->_capacity = p_vector->_buffer._size / p_vector->_stride;
    }
}

void sg_vector_resize(sg_vector* p_vector, u32 size)
{
    sg_vector_reserve(p_vector, size);
    p_vector->_size = size;
}

void* sg_vector_emplace(sg_vector* p_vector)
{
    u32 byte_offset = p_vector->_size * p_vector->_stride;

    if (p_vector->_capacity <= p_vector->_size)
    {
        u64 size = p_vector->_size;
        if (size == 0)
            size = 1;
        
        sg_vector_reserve(p_vector, size * 2ull);
    }

    p_vector->_size += 1;

    return &p_vector->_buffer._allocation[byte_offset];
}

u32 sg_vector_push(sg_vector* p_vector, void* p_element)
{
    u32 index = p_vector->_size;
    u32 byte_offset = index * p_vector->_stride;

    if (p_vector->_capacity <= p_vector->_size)
    {
        u64 size = p_vector->_size;
        if (size == 0)
            size = 1;
        
        sg_vector_reserve(p_vector, size * 2ull);
    }
    
    memcpy_s(p_vector->_buffer._allocation + byte_offset, p_vector->_stride, p_element, p_vector->_stride);

    p_vector->_size += 1;

    return index;
}

void sg_vector_erase(sg_vector* p_vector, u32 index)
{
    SG_ASSERT(index < p_vector->_size);

    u32 i = index + 1;
    while (i < p_vector->_size)
    {
        u32 curr = i * p_vector->_stride;
        u32 prev = (i - 1) * p_vector->_stride;
        memcpy_s(p_vector->_buffer._allocator + prev, p_vector->_stride, p_vector->_buffer._allocator + curr, p_vector->_stride);
        ++i;
    }

    p_vector->_size -= 1;
}

u32 sg_vector_size(sg_vector* p_vector)
{
    return p_vector->_size;
}

u8 sg_vector_any(sg_vector* p_vector)
{
    return p_vector->_size != 0;
}

void* sg_vector_data(sg_vector* p_vector, u32 index)
{
    SG_ASSERT(p_vector->_buffer._allocation);
    SG_ASSERT(p_vector->_size > index);

    return sg_buffer_data(&p_vector->_buffer, index * p_vector->_stride);
}

void* sg_vector_back(sg_vector* p_vector)
{
    SG_ASSERT(p_vector->_buffer._allocation);
    SG_ASSERT(p_vector->_size > 0);

    return sg_buffer_data(&p_vector->_buffer, (p_vector->_size - 1) * p_vector->_stride);
}

sg_slice sg_vector_to_slice(sg_vector* p_vector, u32 offset, u32 size)
{
    SG_ASSERT(p_vector != NULL);
    SG_ASSERT(p_vector->_size >= offset + size);

    return sg_slice_make(p_vector->_buffer._allocation, offset, size, p_vector->_stride);
}
