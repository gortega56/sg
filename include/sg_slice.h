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

void* sg_slice_data(sg_slice* p_slice, u32 index);

sg_slice sg_slice_to_slice(sg_slice* p_slice, u32 offset, u32 count);

#define sg_slice_define_type_ext(slice_type, element_type)\
typedef sg_slice slice_type;\
inline slice_type slice_type##_make(element_type* p_data, u32 offset, u32 count) { return sg_slice_make(p_data, offset, count, sizeof(element_type)); }\
inline u32 slice_type##_size(slice_type* p_slice) { return sg_slice_size(p_slice); }\
inline element_type* slice_type##_data(slice_type* p_slice, u32 index) { return (element_type*)sg_slice_data(p_slice, index); }\
inline slice_type slice_type##_to_slice(slice_type* p_slice, u32 offset, u32 count) { return sg_slice_to_slice(p_slice, offset, count); }