#include "sg_allocator.h"
#include <stdlib.h>

static inline void* sg_allocator_malloc(u64 size, void* p_user_data)
{
    return malloc(size);
}

inline void sg_allocator_free(void* p_allocation, void* p_user_data)
{
    free(p_allocation);
}

inline void* sg_allocator_realloc(void* p_allocation, u64 size, void* p_user_data)
{
    return realloc(p_allocation, size);
}

sg_allocator s_allocator_default =
{
    &sg_allocator_malloc,
    &sg_allocator_free,
    &sg_allocator_realloc,
    NULL
};