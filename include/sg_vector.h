#ifndef SG_VECTOR_H
#define SG_VECTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#define SG_EXTERN_C_BEGIN extern "C" {
#define SG_EXTERN_C_END }
#else
#define SG_EXTERN_C_BEGIN
#define SG_EXTERN_C_END
#endif

#ifdef SG_DEFINE_STATIC_INTERFACE
#define SG_INTERFACE static
#else
#define SG_INTERFACE extern
#endif

SG_EXTERN_C_BEGIN

typedef struct sg_vector_allocator_interface
{
    void*(*p_malloc)(size_t);
    void*(*p_realloc)(void*, size_t);
    void(*p_free)(void*);
} sg_vector_allocator;

typedef struct sg_vec
{
    sg_vector_allocator m_allocator;
    uint8_t* mp_data;
    size_t m_data_size;
    uint64_t m_element_hash;
    uint32_t m_element_stride;
    uint32_t m_element_count;
} sg_vec;

SG_INTERFACE void sg_vector_internal_create(sg_vec* p_vector, sg_vector_allocator* p_allocator, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count);

SG_INTERFACE void sg_vector_internal_destroy(sg_vec* p_vector);

SG_INTERFACE void sg_vector_internal_reserve(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count);

SG_INTERFACE void sg_vector_internal_resize(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count);

SG_INTERFACE void* sg_vector_internal_at(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t index);

SG_INTERFACE void* sg_vector_internal_data(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes);

SG_INTERFACE void* sg_vector_internal_back(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes);

SG_INTERFACE void* sg_vector_internal_push(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes);

SG_INTERFACE size_t sg_vector_internal_size(sg_vec* p_vector);

SG_INTERFACE uint64_t sg_vector_internal_element_type_hash(const char* sz_element_type_name);

#define sg_vector_create(p_vector, p_allocator, element_hash, element_type, element_count) sg_vector_internal_create(p_vector, p_allocator, element_hash, sizeof(element_type), element_count)

#define sg_vector_destroy(p_vector) sg_vector_internal_destroy(p_vector)

#define sg_vector_reserve(p_vector, element_hash, element_type, element_count) sg_vector_internal_reserve(p_vector, element_hash, sizeof(element_type), element_count)

#define sg_vector_resize(p_vector, element_hash, element_type, element_count) sg_vector_internal_resize(p_vector, element_hash, sizeof(element_type), element_count)

#define sg_vector_at(p_vector, element_hash, element_type, index) (element_type*)sg_vector_internal_at(p_vector, element_hash, sizeof(element_type), index)

#define sg_vector_data(p_vector, element_hash, element_type) ((element_type*)sg_vector_internal_data(p_vector, element_hash, sizeof(element_type)))

#define sg_vector_back(p_vector, element_hash, element_type) (element_type*)sg_vector_internal_back(p_vector, element_hash, sizeof(element_type))

#define sg_vector_push(p_vector, element_hash, element_type, element) *(element_type*)sg_vector_internal_push(p_vector, element_hash, sizeof(element_type)) = element 

#define sg_vector_size(p_vector) sg_vector_internal_size(p_vector)

#define sg_hashof(element_type) static uint64_t s_sg_hash = sg_vector_internal_element_type_hash(#element_type)

#ifdef SG_VECTOR_DEFINE_IMPLEMENTATION

#include <cassert>
#include <malloc.h>
#include <memory.h>

uint64_t sg_vector_internal_element_type_hash(const char* sz_element_type_name)
{
    // djb2
    uint64_t hash = 5381;
    int32_t c;

    while (c = *sz_element_type_name++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

void sg_vector_internal_create(sg_vec* p_vector, sg_vector_allocator* p_allocator, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count)
{
    assert(p_vector);
    assert(p_vector->mp_data == NULL);
    assert(p_vector->m_element_hash == 0);
    assert(p_vector->m_element_stride == 0);
    assert(p_vector->m_element_count == 0);
    assert(element_size_in_bytes);

    sg_vector_allocator allocator;
    allocator.p_malloc = &malloc;
    allocator.p_realloc = &realloc;
    allocator.p_free = &free;

    if (p_allocator)
    {
        assert(p_allocator->p_malloc);
        assert(p_allocator->p_realloc);
        assert(p_allocator->p_free);
        allocator = *p_allocator;
    }

    p_vector->m_allocator = allocator;
    p_vector->mp_data = NULL;
    p_vector->m_data_size = 0;
    p_vector->m_element_hash = element_hash;
    p_vector->m_element_stride = element_size_in_bytes;
    p_vector->m_element_count = 0;
    
    sg_vector_internal_resize(p_vector, p_vector->m_element_hash, p_vector->m_element_stride, element_count);
}

void sg_vector_internal_destroy(sg_vec* p_vector)
{
    assert(p_vector);

    if (p_vector->mp_data)
        p_vector->m_allocator.p_free(p_vector->mp_data);

    memset(p_vector, 0, sizeof(sg_vec));
}

void sg_vector_internal_reserve(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count)
{
    assert(p_vector);
    assert(p_vector->m_allocator.p_malloc);
    assert(p_vector->m_allocator.p_realloc);
    assert(p_vector->m_allocator.p_free);
    assert(p_vector->m_element_hash == element_hash);
    assert(p_vector->m_element_stride == element_size_in_bytes);

    size_t required_data_size = p_vector->m_element_stride * element_count;
    if (p_vector->m_data_size < required_data_size)
    {
        #ifdef SG_VECTOR_NO_REALLOC
        
        uint8_t* p_prev = p_vector->mp_data;
        uint8_t* p_curr = (uint8_t*)p_vector->m_allocator.p_malloc(required_data_size);
        assert(p_curr);
        
        if (p_prev)
        {
            memcpy_s(p_curr, required_data_size, p_prev, p_vector->m_data_size);
            m_allocator.p_free(p_prev);
        }

        p_vector->mp_data = p_curr;
        p_vector->m_data_size = required_data_size;
        
        #else
        
        uint8_t* p_prev = p_vector->mp_data;
        uint8_t* p_curr = (uint8_t*)p_vector->m_allocator.p_realloc(p_prev, required_data_size);
        if (!p_curr)
        {
            p_vector->m_allocator.p_free(p_prev);
            assert(p_curr);
        }

        p_vector->mp_data = p_curr;
        p_vector->m_data_size = required_data_size;
        
        #endif
    }
}

void sg_vector_internal_resize(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t element_count)
{
    sg_vector_internal_reserve(p_vector, element_hash, element_size_in_bytes, element_count);
    p_vector->m_element_count = element_count;
}

void* sg_vector_internal_at(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes, uint32_t index)
{
    assert(p_vector);
    assert(p_vector->mp_data);
    assert(p_vector->m_element_hash == element_hash);
    assert(p_vector->m_element_stride == element_size_in_bytes);
    assert(p_vector->m_element_count > index);

    return p_vector->mp_data + p_vector->m_element_stride * index;
}

void* sg_vector_internal_data(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes)
{
    assert(p_vector);
    assert(p_vector->mp_data);
    assert(p_vector->m_element_hash == element_hash);
    assert(p_vector->m_element_stride == element_size_in_bytes);

    return p_vector->mp_data;
}

void* sg_vector_internal_back(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes)
{
    assert(p_vector);
    assert(p_vector->m_element_hash == element_hash);
    assert(p_vector->m_element_stride == element_size_in_bytes);
    assert(p_vector->m_element_count != 0);

    return sg_vector_internal_at(p_vector, element_hash, p_vector->m_element_stride, p_vector->m_element_count - 1);
}

void* sg_vector_internal_push(sg_vec* p_vector, uint64_t element_hash, uint32_t element_size_in_bytes)
{
    assert(p_vector);
    assert(p_vector->m_element_hash == element_hash);
    assert(p_vector->m_element_stride == element_size_in_bytes);

    size_t required_capacity = p_vector->m_element_stride * (p_vector->m_element_count + 1);
    if (p_vector->m_data_size < required_capacity)
        sg_vector_internal_reserve(p_vector, p_vector->m_element_hash, p_vector->m_element_stride, (p_vector->m_element_count) ? p_vector->m_element_count * 2 : 2);

    uint8_t* p_element = p_vector->mp_data + p_vector->m_element_stride * p_vector->m_element_count;
    p_vector->m_element_count++;
    return p_element;
}

size_t sg_vector_internal_size(sg_vec* p_vector)
{
    return p_vector->m_element_count;
}

#endif

#define sg_vector_define_type_interface(element_type)   \
extern uint64_t sg_vector_type_hash_##element_type;     \
void sg_vector_create_##element_type(sg_vec* p_vector, sg_vector_allocator* p_allocator, uint32_t element_count); \
void sg_vector_destroy_##element_type(sg_vec* p_vector);\
void sg_vector_reserve_##element_type(sg_vec* p_vector, uint32_t element_count);\
void sg_vector_resize_##element_type(sg_vec* p_vector, uint32_t element_count); \
element_type* sg_vector_at_##element_type(sg_vec* p_vector, size_t element_index);  \
element_type* sg_vector_data_##element_type(sg_vec* p_vector);  \
element_type* sg_vector_back_##element_type(sg_vec* p_vector);  \
void sg_vector_push_##element_type(sg_vec* p_vector, element_type element); \
size_t sg_vector_size_##element_type(sg_vec* p_vector);

#define sg_vector_define_type_implementation(element_type)  \
uint64_t sg_vector_type_hash_##element_type = sg_vector_internal_element_type_hash(#element_type);  \
void sg_vector_create_##element_type(sg_vec* p_vector, sg_vector_allocator* p_allocator, uint32_t element_count)  { sg_vector_create(p_vector, p_allocator, sg_vector_type_hash_##element_type, element_type, element_count); }\
void sg_vector_destroy_##element_type(sg_vec* p_vector)    { sg_vector_destroy(p_vector); } \
void sg_vector_reserve_##element_type(sg_vec* p_vector, uint32_t element_count)  { sg_vector_reserve(p_vector, sg_vector_type_hash_##element_type, element_type, element_count); }  \
void sg_vector_resize_##element_type(sg_vec* p_vector, uint32_t element_count)  { sg_vector_resize(p_vector, sg_vector_type_hash_##element_type, element_type, element_count); }    \
element_type* sg_vector_at_##element_type(sg_vec* p_vector, size_t element_index)  { return sg_vector_at(p_vector, sg_vector_type_hash_##element_type, element_type, element_index); }   \
element_type* sg_vector_data_##element_type(sg_vec* p_vector)    { return sg_vector_data(p_vector, sg_vector_type_hash_##element_type, element_type); }\
element_type* sg_vector_back_##element_type(sg_vec* p_vector)    { return sg_vector_back(p_vector, sg_vector_type_hash_##element_type, element_type); }\
void sg_vector_push_##element_type(sg_vec* p_vector, element_type element) { sg_vector_push(p_vector, sg_vector_type_hash_##element_type, element_type, element); }\
size_t sg_vector_size_##element_type(sg_vec* p_vector) { return sg_vector_size(p_vector);}

#ifdef SG_VECTOR_DEFINE_STANDARD_INTERFACES

sg_vector_define_type_interface(bool)
sg_vector_define_type_interface(uint8_t)
sg_vector_define_type_interface(uint16_t)
sg_vector_define_type_interface(uint32_t)
sg_vector_define_type_interface(uint64_t)
sg_vector_define_type_interface(size_t)
sg_vector_define_type_interface(int8_t)
sg_vector_define_type_interface(int16_t)
sg_vector_define_type_interface(int32_t)
sg_vector_define_type_interface(int64_t)

#endif

#ifdef SG_VECTOR_DEFINE_STANDARD_IMPLEMENTATIONS

sg_vector_define_type_implementation(bool)
sg_vector_define_type_implementation(uint8_t)
sg_vector_define_type_implementation(uint16_t)
sg_vector_define_type_implementation(uint32_t)
sg_vector_define_type_implementation(uint64_t)
sg_vector_define_type_implementation(size_t)
sg_vector_define_type_implementation(int8_t)
sg_vector_define_type_implementation(int16_t)
sg_vector_define_type_implementation(int32_t)
sg_vector_define_type_implementation(int64_t)

#endif

SG_EXTERN_C_END

#endif
