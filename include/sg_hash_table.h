#pragma once
#include "sg_types.h"

typedef struct sg_allocator sg_allocator;

typedef struct sg_hash_table
{
    sg_allocator* p_allocator;
    u32* _keys;
    u8* _data;
    u32 _capacity;
    u32 _size;
    u32 _stride;
    float _load_factor;

} sg_hash_table;

sg_hash_table sg_hash_table_create(u32 size, u32 stride, float load_factor, sg_allocator* p_allocator);

void sg_hash_table_destroy(sg_hash_table* p_table);

u8 sg_hash_table_find(sg_hash_table* p_table, u32 key, void** pp_value);

void sg_hash_table_insert(sg_hash_table* p_table, u32 key, void* p_value);

void* sg_hash_table_insert_inline(sg_hash_table* p_table, u32 key);

void sg_hash_table_remove(sg_hash_table* p_table, u32 key);

void sg_hash_table_clear(sg_hash_table* p_table);

#define SG_HASH_TABLE_DEFINE_TYPE_EXT(hash_table_type, element_type)\
typedef sg_hash_table hash_table_type;\
inline hash_table_type hash_table_type##_create(u32 size, float load_factor, sg_allocator* p_allocator) { return sg_hash_table_create(size, sizeof(element_type), load_factor, p_allocator); }\
inline void hash_table_type##_destroy(hash_table_type* p_table) { sg_hash_table_destroy(p_table); }\
inline u8 hash_table_type##_find(hash_table_type* p_table, u32 key, element_type** pp_element) { return sg_hash_table_find(p_table, key, (void**)pp_element); }\
inline void hash_table_type##_insert(hash_table_type* p_table, u32 key, element_type element) { sg_hash_table_insert(p_table, key, &element); }\
inline element_type* hash_table_type##_insert_inline(hash_table_type* p_table, u32 key) { return (element_type*)sg_hash_table_insert_inline(p_table, key); }\
inline void hash_table_type##_remove(hash_table_type* p_table, u32 key) { sg_hash_table_remove(p_table, key); }