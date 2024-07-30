#pragma once
#include "sg_types.h"

#define SG_HASH_TABLE_IDX_NULL ~0U
#define SG_HASH_TABLE_KEY_NULL ~0U
#define SG_HASH_TABLE_VAL_NULL 0U

typedef struct sg_allocator sg_allocator;

typedef struct sg_hash_table
{
    sg_allocator* p_allocator;
    sg_u32* _keys;
    sg_u8* _data;
    sg_u32 _capacity;
    sg_u32 _size;
    sg_u32 _stride;
    sg_u32 _probe_length;
    sg_f32 _load_factor;

} sg_hash_table;

sg_hash_table sg_hash_table_create(sg_u32 capacity, sg_u32 stride, sg_f32 load_factor, sg_allocator* p_allocator);

void sg_hash_table_destroy(sg_hash_table* p_table);

void sg_hash_table_reserve(sg_hash_table* p_table, sg_u32 size);

sg_u32 sg_hash_table_size(sg_hash_table* p_table);

sg_u32 sg_hash_table_capacity(sg_hash_table* p_table);

sg_u8 sg_hash_table_find(sg_hash_table* p_table, sg_u32 key);

sg_u8 sg_hash_table_find_index(sg_hash_table* p_table, sg_u32 key, sg_u32* p_idx);

sg_u8 sg_hash_table_find_value(sg_hash_table* p_table, sg_u32 key, void** pp_value);

void* sg_hash_table_emplace(sg_hash_table* p_table, sg_u32 key);

void sg_hash_table_insert(sg_hash_table* p_table, sg_u32 key, void* p_value);

void sg_hash_table_remove_at_index(sg_hash_table* p_table, sg_u32 idx);

void sg_hash_table_remove(sg_hash_table* p_table, sg_u32 key);

void sg_hash_table_clear(sg_hash_table* p_table);

#define SG_HASH_TABLE_DEFINE_TYPE_EXT(hash_table_type, element_type)\
typedef sg_hash_table hash_table_type;\
inline hash_table_type hash_table_type##_create(sg_u32 size, sg_f32 load_factor, sg_allocator* p_allocator) { return sg_hash_table_create(size, sizeof(element_type), load_factor, p_allocator); }\
inline void hash_table_type##_destroy(hash_table_type* p_table) { sg_hash_table_destroy(p_table); }\
inline sg_u8 hash_table_type##_find(hash_table_type* p_table, sg_u32 key) { return sg_hash_table_find(p_table, key); }\
inline sg_u8 hash_table_type##_find_index(hash_table_type* p_table, sg_u32 key, sg_u32* p_idx) { return sg_hash_table_find_index(p_table, key, p_idx); }\
inline sg_u8 hash_table_type##_find_value(hash_table_type* p_table, sg_u32 key, element_type** pp_element) { return sg_hash_table_find_value(p_table, key, (void**)pp_element); }\
inline void hash_table_type##_insert(hash_table_type* p_table, sg_u32 key, element_type element) { sg_hash_table_insert(p_table, key, &element); }\
inline element_type* hash_table_type##_emplace(hash_table_type* p_table, sg_u32 key) { return (element_type*)sg_hash_table_emplace(p_table, key); }\
inline void hash_table_type##_remove(hash_table_type* p_table, sg_u32 key) { sg_hash_table_remove(p_table, key); }