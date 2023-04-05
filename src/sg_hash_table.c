#include "sg_hash_table.h"
#include "sg_allocator.h"
#include "sg_assert.h"
#include <math.h>

#define SG_HASH_TABLE_IDX_NULL ~0U
#define SG_HASH_TABLE_KEY_NULL ~0U
#define SG_HASH_TABLE_VAL_NULL 0U

static const float s_golden_ratio = 1.61803398875f; // (sqrt(5) + 1)/2

static inline u32 sg_hash(u32 key, u32 table_size)
{
    // multiplication hash floor( n( kA mod 1 ) )
    float a = (float)key * s_golden_ratio;
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
        u64 capacity_prev = p_table->_capacity;
        u64 capacity_curr = p_table->_capacity * 2;
        if (capacity_curr == 0)
            capacity_curr = 4;

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
    static u8 s = 0;

void sg_hash_table_insert(sg_hash_table* p_table, u32 key, void* p_value)
{
    sg_hash_table_grow_if_necessary(p_table);

    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    sg_hash_table_search(p_table, hash, key, &index);

    SG_ASSERT(index != SG_HASH_TABLE_IDX_NULL);

    p_table->_size += 1;

    p_table->_keys[index] = key;
    memcpy_s(p_table->_data + index * p_table->_stride, p_table->_stride, p_value, p_table->_stride);
}

void* sg_hash_table_insert_inline(sg_hash_table* p_table, u32 key)
{
    sg_hash_table_grow_if_necessary(p_table);

    u32 hash = sg_hash(key, p_table->_capacity);
    u32 index = SG_HASH_TABLE_IDX_NULL;
    sg_hash_table_search(p_table, hash, key, &index);

    SG_ASSERT(index != SG_HASH_TABLE_IDX_NULL);

    p_table->_size += 1;

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