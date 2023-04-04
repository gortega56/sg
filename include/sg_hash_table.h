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
