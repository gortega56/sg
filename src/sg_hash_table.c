#include "sg_hash_table.h"
#include "sg_allocator.h"
#include "sg_assert.h"
#include <math.h>

#define SG_HASH_TABLE_IDX_NULL ~0U
#define SG_HASH_TABLE_KEY_NULL ~0U
#define SG_HASH_TABLE_VAL_NULL 0U

static const sg_u32 s_minimum_capacity = 4;

static inline sg_f32 sg_load_factor(sg_u32 size, sg_u32 capacity)
{
    return (sg_f32)size / (sg_f32)capacity;
}

static inline sg_u32 sg_probe_length(sg_u32 idx_start, sg_u32 idx, sg_u32 capacity)
{
    if (idx_start <= idx) return idx - idx_start;
    return idx + capacity - idx_start;
}

static inline sg_u32 sg_idx_start(sg_u32 key, sg_u32 table_size)
{
    // https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
    sg_u64 fib = (11400714819323198485ull * (sg_u64)key) & 0xffffffff;
    return (sg_u32)((fib * (sg_u64)table_size) >> 32ull);
}

static inline sg_u8 sg_search(sg_u32* p_keys, sg_u32 key, sg_u32 capacity, sg_u32 probe_length, sg_u32* p_idx)
{
    /*
    1. find the start idx
    2. loop(max probe_lenth) until matching hash has been found
    */
    sg_u32 idx_start = sg_idx_start(key, capacity);
    sg_u32 idx = idx_start;
    sg_u32 probe = 0;
    while (probe <= probe_length)
    {
        if (idx > capacity)
            idx = idx - capacity;

        if (p_keys[idx] == key)
        {
            if (p_idx)
                *p_idx = idx;

            return 1;
        }

        idx += 1;
        probe += 1;
    }

    return 0;
}

static inline void sg_erase_key(sg_u32* p_keys, sg_u32 idx)
{
    p_keys[idx] = SG_HASH_TABLE_KEY_NULL;
}

static inline void sg_erase_val(sg_u8* p_data, sg_u32 idx, sg_u32 stride)
{
    memset(p_data + idx * stride, SG_HASH_TABLE_VAL_NULL, stride);
}

static inline void sg_hash_table_rehash(sg_hash_table* p_table, sg_u32* p_keys, sg_u8* p_data, sg_u32 capacity)
{
    sg_u32 idx = 0;
    while (idx < capacity)
    {
        sg_u32 key = p_keys[idx];
        if (key != SG_HASH_TABLE_KEY_NULL)
        {
            sg_hash_table_insert(p_table, key, p_data + idx * p_table->_stride);
        }

        idx += 1;
    }
}

static inline void sg_hash_table_resize(sg_hash_table* p_table, sg_u32 capacity)
{
    if (p_table->_capacity < capacity)
    {
        sg_u64 capacity_prev = p_table->_capacity;
        sg_u64 capacity_curr = capacity;

        sg_u32* p_keys = p_table->_keys;
        sg_u8* p_data = p_table->_data;

        // Alloc 1 space more than necessary for capacity and use that to swap during remove
        sg_u64 key_data_length = capacity_curr * sizeof(sg_u32);
        sg_u64 val_data_length = (1 + capacity_curr) * p_table->_stride; 

        p_table->_keys = (sg_u32*)p_table->p_allocator->allocate(key_data_length, p_table->p_allocator->p_user_data);
        p_table->_data = (sg_u8*)p_table->p_allocator->allocate(val_data_length, p_table->p_allocator->p_user_data);
        p_table->_capacity = capacity_curr;

        memset(p_table->_keys, SG_HASH_TABLE_KEY_NULL, key_data_length);
        memset(p_table->_data, SG_HASH_TABLE_VAL_NULL, val_data_length);

        if (p_table->_size)
            sg_hash_table_rehash(p_table, p_keys, p_data, capacity_prev);

        if (p_keys) p_table->p_allocator->free(p_keys, p_table->p_allocator->p_user_data);
        if (p_data) p_table->p_allocator->free(p_data, p_table->p_allocator->p_user_data);
    }
}

static inline void sg_hash_table_resize_if_necessary(sg_hash_table* p_table)
{
    sg_u8 resize = 0;
    if (p_table->_load_factor < sg_load_factor(p_table->_size, p_table->_capacity))
        resize = 1;

    if (p_table->_capacity < s_minimum_capacity)
        resize = 1;

    if (resize)
    {
        sg_u32 capacity = p_table->_capacity * 2U;
        if (capacity < s_minimum_capacity)
            capacity = s_minimum_capacity;

        sg_hash_table_resize(p_table, capacity);
    }
}


sg_hash_table sg_hash_table_create(sg_u32 capacity, sg_u32 stride, sg_f32 load_factor, sg_allocator* p_allocator)
{
    if (p_allocator == NULL)
        p_allocator = &s_allocator_default;

    sg_hash_table table;
    table.p_allocator = p_allocator;
    table._keys = NULL;
    table._data = NULL;
    table._capacity = 0;
    table._size = 0;
    table._stride = stride;
    table._probe_length = 0;
    table._load_factor = load_factor;

    if (capacity < s_minimum_capacity)
        capacity = s_minimum_capacity;

    sg_hash_table_resize(&table, capacity);

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
    p_table->_probe_length = 0;
}

void sg_hash_table_reserve(sg_hash_table* p_table, sg_u32 size)
{
    sg_u32 capacity = (sg_u32)((sg_f32)size * (1.0f / p_table->_load_factor));
    if (capacity < s_minimum_capacity)
        capacity = s_minimum_capacity;

    sg_hash_table_resize(p_table, capacity);
}   

sg_u32 sg_hash_table_size(sg_hash_table* p_table, sg_u32 size)
{
    return p_table->_size;
}

sg_u8 sg_hash_table_find(sg_hash_table* p_table, sg_u32 key, void** pp_data)
{
    sg_u32 idx = SG_HASH_TABLE_IDX_NULL;
    sg_u8 found = sg_search(p_table->_keys, key, p_table->_capacity, p_table->_probe_length, &idx);
    if (found)
    {
        if (pp_data)
            *pp_data = p_table->_data + idx * p_table->_stride;
    }

    return found;
}


/*
    1. Find start slot
    2. Loop (capacity) until null hash is found
    3. Insert
*/
void* sg_hash_table_emplace(sg_hash_table* p_table, sg_u32 key)
{
    sg_hash_table_resize_if_necessary(p_table);

    sg_u32 idx_start = sg_idx_start(key, p_table->_capacity);
    sg_u32 idx = idx_start;
    sg_u32 probe = 0;
    while (1)
    {
        if (p_table->_keys[idx] == SG_HASH_TABLE_KEY_NULL)
        {
            p_table->_keys[idx] = key;
            p_table->_size += 1;
            if (p_table->_probe_length < probe)
                p_table->_probe_length = probe;

            return p_table->_data + idx * p_table->_stride;
        }

        idx = (idx + 1) % p_table->_capacity;
        probe += 1;
    }
}

void sg_hash_table_insert(sg_hash_table* p_table, sg_u32 key, void* p_value)
{
    void* p_val = sg_hash_table_emplace(p_table, key);
    memcpy_s(p_val, p_table->_stride, p_value, p_table->_stride);
}

void sg_hash_table_remove(sg_hash_table* p_table, sg_u32 key)
{
    sg_u32 idx = SG_HASH_TABLE_IDX_NULL;
    sg_u8 found = sg_search(p_table->_keys, key, p_table->_capacity, p_table->_probe_length, &idx);
    if (found)
    {
        sg_erase_key(p_table->_keys, idx);
        sg_erase_val(p_table->_data, idx, p_table->_stride);
        p_table->_size -= 1;

        idx = (idx + 1) % p_table->_capacity;

        while (p_table->_keys[idx] != SG_HASH_TABLE_KEY_NULL)
        {
            sg_u32 key = p_table->_keys[idx];
            sg_u8* p_val = p_table->_data + idx * p_table->_stride;
            sg_u8* p_temp = p_table->_data + p_table->_capacity * p_table->_stride;
            memcpy_s(p_temp, p_table->_stride, p_val, p_table->_stride);

            sg_erase_key(p_table->_keys, idx);
            sg_erase_val(p_table->_data, idx, p_table->_stride);
            p_table->_size -= 1;

            sg_hash_table_insert(p_table, key, p_temp);

            idx = (idx + 1) % p_table->_capacity;
        }

        if (p_table->_size == 0)
            p_table->_probe_length = 0;
    }
}

void sg_hash_table_clear(sg_hash_table* p_table)
{
    memset(p_table->_keys, SG_HASH_TABLE_KEY_NULL, sizeof(sg_u32) * p_table->_capacity);
    memset(p_table->_data, SG_HASH_TABLE_VAL_NULL, p_table->_stride * p_table->_capacity);
    p_table->_size = 0;
    p_table->_probe_length = 0;
}

sg_u32 sg_hash_table_key_null()
{
    return SG_HASH_TABLE_KEY_NULL;
}