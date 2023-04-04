#pragma once
#include "sg_types.h"

typedef struct sg_allocator
{
    void* (*allocate)(u64, void*);
    void  (*free)(void*, void*);
    void* (*realloc)(void*, u64, void*);
    void* p_user_data;
} sg_allocator;

extern sg_allocator s_allocator_default;
