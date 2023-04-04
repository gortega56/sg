#include "sg_buffer.h"
#include "sg_allocator.h"
#include "sg_assert.h"

sg_buffer sg_buffer_create(u64 size, sg_allocator* p_allocator)
{
    if (p_allocator == NULL)
        p_allocator = &s_allocator_default;

    void* p_allocation = NULL;
    if (size != 0)
        p_allocation = (u8*)p_allocator->allocate(size, p_allocator->p_user_data);

    sg_buffer buffer;
    buffer._allocator = (sg_allocator* const)p_allocator;
    buffer._allocation = p_allocation;
    buffer._size = size;
    return buffer;
}

void sg_buffer_destroy(sg_buffer* p_buffer)
{
    void* p_allocation = p_buffer->_allocation;
    if (p_allocation)
        p_buffer->_allocator->free(p_allocation, p_buffer->_allocator->p_user_data);

    p_buffer->_allocator = NULL;
    p_buffer->_allocation = NULL;
    p_buffer->_size = 0;
}

void* sg_buffer_data(sg_buffer* p_buffer, u32 offset)
{
    SG_ASSERT(p_buffer->_allocation != NULL);

    return p_buffer->_allocation + offset;
}

void sg_buffer_expand(sg_buffer* p_buffer, u64 size)
{
    if (p_buffer->_size < size)
    {
        p_buffer->_allocation = p_buffer->_allocator->realloc(p_buffer->_allocation, size, p_buffer->_allocator->p_user_data);
        p_buffer->_size = size;
    }
}