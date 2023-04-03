#include "..\include\sg_vector.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define SG_ASSERT(expression) assert(expression)

static inline void memclear(void* p_data, u64 size)
{
    memset(p_data, 0U, size);
}

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

static sg_allocator s_allocator_default = 
{
    &sg_allocator_malloc,
    &sg_allocator_free,
    &sg_allocator_realloc,
    NULL
};

//=============================================================================================================================

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

//=============================================================================================================================

sg_slice sg_slice_make(u8* p_data, u32 count, u32 stride)
{
    sg_slice slice;
    slice._data = p_data;
    slice._size = count * stride;
    slice._stride = stride;
    slice._count = count;
    return slice;
}

u32 sg_slice_size(sg_slice* p_slice)
{
    return p_slice->_count;
}

void* sg_slice_data(sg_slice* p_slice)
{
    return p_slice->_data;
}

sg_slice sg_slice_to_slice(sg_slice* p_slice, u32 offset, u32 count)
{
    SG_ASSERT(p_slice);
    SG_ASSERT(p_slice->_count >= offset + count);

    u8* p_data = p_slice->_data;
    if (p_data != NULL)
        p_data += offset * p_slice->_stride;

    return sg_slice_make(p_data, count, p_slice->_stride);
}

//=============================================================================================================================

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

sg_slice sg_vector_to_slice(sg_vector* p_vector, u32 offset, u32 size)
{
    SG_ASSERT(p_vector != NULL);
    SG_ASSERT(p_vector->_size >= offset + size);

    return sg_slice_make(sg_vector_data(p_vector, offset), size, p_vector->_stride);
}

#define SG_HASH_TABLE_IDX_NULL ~0U
#define SG_HASH_TABLE_KEY_NULL ~0U
#define SG_HASH_TABLE_VAL_NULL 0U
#define SG_HASH_TABLE_GROWTH_FACTOR 0.25f

static inline u32 sg_hash(u32 key, u32 table_size)
{
    // multiplication hash floor( n( kA mod 1 ) )
    float A = 1.61803398875f;
    float a = (float)key * A;
    float b = a - floorf(a);
    float c = (float)table_size * b;
    u32 h = (u32)floorf(c);
    return h;
}

static inline u8 sg_hash_table_search(sg_hash_table* p_table, u32 hash, u32 key, u32* p_index)
{
    u32 slot_start = hash;
    u32 slot_offset = 0;

    while (slot_offset < p_table->_capacity)
    {
        u32 index = (slot_start + slot_offset) % p_table->_capacity;
        if (p_table->_keys[index] == key)
        {
            *p_index = index;        
            return 1;
        }

        if (p_table->_keys[index] == SG_HASH_TABLE_KEY_NULL)
        {
            *p_index = index;
            return 0;
        }

        slot_offset += 1;
    }

    *p_index = SG_HASH_TABLE_IDX_NULL;

    return 0;
}

static inline sg_hash_table_rehash(sg_hash_table* p_table, u32* p_keys, u8* p_data, u32 capacity)
{
    u32 index_prev = 0; 
    while (index_prev < capacity)
    {
        if (p_keys[index_prev] != SG_HASH_TABLE_KEY_NULL)
        {
            u32 key = p_keys[index_prev];
            u32 hash = sg_hash(key, p_table->_capacity);
            u32 index_curr = SG_HASH_TABLE_IDX_NULL;
            sg_hash_table_search(p_table, hash, key, &index_curr);

            SG_ASSERT(index_curr != SG_HASH_TABLE_IDX_NULL);
    
            p_table->_keys[index_curr] = key;
            memcpy_s(p_table->_data + index_curr * p_table->_stride, p_table->_stride, p_data + index_prev * p_table->_stride, p_table->_stride);
        }

        index_prev += 1;        
    }
}

static inline sg_hash_table_grow_if_necessary(sg_hash_table* p_table)
{
    float load_factor = (p_table->_capacity == 0) 
                      ? 1.0f 
                      : (float)(p_table->_size + 1) / (float)p_table->_capacity;

    if (load_factor > p_table->_load_factor)
    {
        float growth_factor = p_table->_load_factor * SG_HASH_TABLE_GROWTH_FACTOR;
        u64 capacity_curr = (u64)ceilf((1.0f / growth_factor) * (float)(p_table->_size + 1));
        u64 capacity_prev = p_table->_capacity;

        u32* p_keys = p_table->_keys;
        u8* p_data = p_table->_data;

        u64 key_data_length = capacity_curr * sizeof(u32);
        u64 val_data_length = capacity_curr * p_table->_stride;

        p_table->_keys = (u32*)p_table->p_allocator->allocate(key_data_length, p_table->p_allocator->p_user_data);
        p_table->_data = (u8*)p_table->p_allocator->allocate(val_data_length, p_table->p_allocator->p_user_data);
        p_table->_capacity = capacity_curr;

        memset(p_table->_keys, SG_HASH_TABLE_KEY_NULL, key_data_length);
        memset(p_table->_data, SG_HASH_TABLE_VAL_NULL, val_data_length);

        if (p_table->_size)
            sg_hash_table_rehash(p_table, p_keys, p_data, capacity_prev);

        if (p_keys) p_table->p_allocator->free(p_keys, p_table->p_allocator->p_user_data);
        if (p_data) p_table->p_allocator->free(p_data, p_table->p_allocator->p_user_data);
    }
}

sg_hash_table sg_hash_table_create(u32 size, u32 stride, float load_factor, sg_allocator* p_allocator)
{
    if (p_allocator == NULL)
        p_allocator = &s_allocator_default;

    void* p_keys = NULL;
    void* p_data = NULL;
    if (size != 0)
    {
        u64 key_data_length = size * sizeof(u32);
        u64 val_data_length = size * stride;
        p_keys = (u32*)p_allocator->allocate(key_data_length, p_allocator->p_user_data);
        p_data = (u8*)p_allocator->allocate(val_data_length, p_allocator->p_user_data);
        memset(p_keys, SG_HASH_TABLE_KEY_NULL, key_data_length);
        memset(p_data, SG_HASH_TABLE_VAL_NULL, val_data_length);
    }

    sg_hash_table table;
    table.p_allocator = p_allocator;
    table._keys = p_keys;
    table._data = p_data;
    table._capacity = size;
    table._size = 0;
    table._stride = stride;
    table._load_factor = load_factor;
    return table;
}

void sg_hash_table_destroy(sg_hash_table* p_table)
{
    if (p_table->_keys)
        p_table->p_allocator->free(p_table->_keys, p_table->p_allocator->p_user_data);

    if (p_table->_data)
        p_table->p_allocator->free(p_table->_data, p_table->p_allocator->p_user_data);

    p_table->p_allocator = NULL;
    p_table->_keys = NULL;
    p_table->_data = NULL;
    p_table->_capacity = 0;
    p_table->_size = 0;
    p_table->_stride = 0;
    p_table->_load_factor = 0.0f;
}

u8 sg_hash_table_find(sg_hash_table* p_table, u32 key, void** pp_data)
{
    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    u8 found = sg_hash_table_search(p_table, hash, key, &index);
    if (found)
    {
        if (pp_data)
            *pp_data = p_table->_data + index * p_table->_stride;
    }    

    return found;    
}

void sg_hash_table_insert(sg_hash_table* p_table, u32 key, void* p_value)
{
    sg_hash_table_grow_if_necessary(p_table);

    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    sg_hash_table_search(p_table, hash, key, &index);

    SG_ASSERT(index != SG_HASH_TABLE_IDX_NULL);
    
    p_table->_keys[index] = key;
    memcpy_s(p_table->_data + index * p_table->_stride, p_table->_stride, p_value, p_table->_stride);

    p_table->_size += 1;
}

void* sg_hash_table_insert_inline(sg_hash_table* p_table, u32 key)
{
    sg_hash_table_grow_if_necessary(p_table);
    
    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    sg_hash_table_search(p_table, hash, key, &index);

    SG_ASSERT(index != SG_HASH_TABLE_IDX_NULL);

    p_table->_keys[index] = key;
    return p_table->_data + index * p_table->_stride;
}

void sg_hash_table_remove(sg_hash_table* p_table, u32 key)
{
    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    u8 found = sg_hash_table_search(p_table, hash, key, &index);
    if (found)
    {
        u32 slot_start = index;
        u32 slot_count = 0;

        // Probe for last slot
        while (slot_count < p_table->_capacity)
        {
            u32 i = (slot_start + slot_count) % p_table->_capacity;
            if (hash != sg_hash(p_table->_keys[i], p_table->_capacity))
                break;
        
            slot_count += 1;
        }

        // Swap current slot with last slot
        u32 slot_end = slot_start + slot_count - 1;
        if (slot_start != slot_end)
        {
            p_table->_keys[slot_start] = p_table->_keys[slot_end];
            memcpy_s(p_table->_data + slot_start * p_table->_stride, p_table->_stride, p_table->_data + slot_end * p_table->_stride, p_table->_stride);
        }

        // Set last slot null
        p_table->_keys[slot_end] = SG_HASH_TABLE_KEY_NULL;
        memset(p_table->_data + slot_end * p_table->_stride, SG_HASH_TABLE_VAL_NULL, p_table->_stride);
    }
}