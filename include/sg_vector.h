#pragma once
#include "sg_types.h"
#include "sg_buffer.h"
#include "sg_slice.h"

typedef struct sg_allocator sg_allocator;

typedef struct sg_vector
{
    sg_buffer _buffer;
    sg_u32 _capacity;
    sg_u32 _size;
    sg_u32 _stride;
} sg_vector;

sg_vector sg_vector_create(sg_u32 size, sg_u32 stride, sg_allocator* p_allocator);

void sg_vector_destroy(sg_vector* p_vector);

void sg_vector_resize(sg_vector* p_vector, sg_u32 size);

void sg_vector_reserve(sg_vector* p_vector, sg_u32 size);

void* sg_vector_emplace(sg_vector* p_vector);

sg_u32 sg_vector_push(sg_vector* p_vector, void* p_element);

void sg_vector_erase(sg_vector* p_vector, sg_u32 index);

sg_u32 sg_vector_size(sg_vector* p_vector);

sg_u8 sg_vector_any(sg_vector* p_vector);

void* sg_vector_data(sg_vector* p_vector, sg_u32 index);

void* sg_vector_back(sg_vector* p_vector);

sg_slice sg_vector_to_slice(sg_vector* p_vector, sg_u32 offset, sg_u32 size);

#define SG_VECTOR_DEFINE_TYPE_EXT(vector_type, element_type)\
typedef sg_vector vector_type;\
inline vector_type vector_type##_create(sg_u32 size, sg_allocator* p_allocator) { return sg_vector_create(size, sizeof(element_type), p_allocator); }\
inline void vector_type##_destroy(vector_type * p_vector) { sg_vector_destroy(p_vector); }\
inline void vector_type##_resize(vector_type * p_vector, sg_u32 size) { sg_vector_resize(p_vector, size); }\
inline void vector_type##_reserve(vector_type * p_vector, sg_u32 size) { sg_vector_reserve(p_vector, size); }\
inline element_type* vector_type##_emplace(vector_type * p_vector) { return (element_type*)sg_vector_emplace(p_vector); }\
inline sg_u32 vector_type##_push(vector_type * p_vector, element_type element){ return sg_vector_push(p_vector, &element); }\
inline sg_u32 vector_type##_size(vector_type * p_vector) { return sg_vector_size(p_vector); }\
inline sg_u8 vector_type##_any(vector_type * p_vector) { return sg_vector_any(p_vector); }\
inline element_type* vector_type##_data(vector_type * p_vector, sg_u32 index) { return (element_type*)sg_vector_data(p_vector, index); }\
inline element_type* vector_type##_back(vector_type * p_vector) { return (element_type*)sg_vector_back(p_vector); }\
inline sg_slice vector_type##_to_slice(vector_type * p_vector, sg_u32 offset, sg_u32 size) { return sg_vector_to_slice(p_vector, offset, size); }
