#include "sg_buffer.h"
#include "sg_allocator.h"
#include "sg_assert.h"

sg_buffer sg_buffer_create(sg_u64 size, sg_allocator* p_allocator)
{
    if (p_allocator == NULL)
        p_allocator = &s_allocator_default;

    void* p_allocation = NULL;
    if (size != 0)
        p_allocation = (sg_u8*)p_allocator->allocate(size, p_allocator->p_user_data);

    sg_buffer buffer;
    buffer.allocator = (sg_allocator* const)p_allocator;
    buffer.allocation = p_allocation;
    buffer.size = size;
    return buffer;
}

void sg_buffer_destroy(sg_buffer* p_buffer)
{
    void* p_allocation = p_buffer->allocation;
    if (p_allocation)
        p_buffer->allocator->free(p_allocation, p_buffer->allocator->p_user_data);

    p_buffer->allocator = NULL;
    p_buffer->allocation = NULL;
    p_buffer->size = 0;
}

void sg_buffer_resize(sg_buffer* p_buffer, sg_u64 size)
{
    if (p_buffer->size < size)
    {
        p_buffer->allocation = p_buffer->allocator->realloc(p_buffer->allocation, size, p_buffer->allocator->p_user_data);
        p_buffer->size = size;
    }
}

void* sg_buffer_data(sg_buffer* p_buffer, sg_u32 offset)
{
    SG_ASSERT(p_buffer->allocation != NULL);

    return p_buffer->allocation + offset;
}