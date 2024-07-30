#include "sg_slice.h"
#include "sg_assert.h"

sg_slice sg_slice_make(void* p_data, sg_u32 offset, sg_u32 count, sg_u32 stride)
{
    if (p_data != NULL)
        p_data = ((sg_u8*)p_data) + offset * stride;

    sg_slice slice;
    slice._data = p_data;
    slice._size = count * stride;
    slice._stride = stride;
    slice._count = count;
    return slice;
}

sg_u32 sg_slice_size(sg_slice* p_slice)
{
    return p_slice->_count;
}

void* sg_slice_data(sg_slice* p_slice, sg_u32 index)
{
    return p_slice->_data + index * p_slice->_stride;
}

sg_slice sg_slice_to_slice(sg_slice* p_slice, sg_u32 offset, sg_u32 count)
{
    SG_ASSERT(p_slice);
    SG_ASSERT(p_slice->_count >= offset + count);

    return sg_slice_make(p_slice->_data, offset, count, p_slice->_stride);
}
