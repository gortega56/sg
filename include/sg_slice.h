#pragma once
#include "sg_types.h"

typedef struct sg_slice
{
    u8* _data;
    u64 _size;
    u32 _count;
    u32 _stride;
} sg_slice;

sg_slice sg_slice_make(void* p_data, u32 offset, u32 count, u32 stride);

u32 sg_slice_size(sg_slice* p_slice);

void* sg_slice_data(sg_slice* p_slice);

sg_slice sg_slice_to_slice(sg_slice* p_slice, u32 offset, u32 count);

#define sg_slice_data_as(slice_ptr, type) (type*)sg_slice_data(slice_ptr)

#define sg_slice_at(slice_ptr, type, index) ((type*)sg_slice_data(slice_ptr))[index]
