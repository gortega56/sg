#include <functional>
#include <gtest/gtest.h>
#include <stdio.h>

extern "C" 
{
#include "sg_slice.h"
#include "sg_vector.h"
#include "sg_hash_table.h"    
}

#define VECTOR_SIZE 4096

struct custom_type
{
    enum { MAX_MSG = 256 };
    char sz_message[MAX_MSG];
    u32 hash = 0;
};

sg_slice_define_type_ext(custom_type_slice, custom_type)
sg_vector_define_type_ext(custom_type_vector, custom_type)
sg_hash_table_define_type_ext(custom_type_table, custom_type);

#define HASH_TABLE_LOAD_FACTOR 0.6f
#define HASH_TABLE_SIZE 4096

namespace sg
{
    namespace unit_tests
    {
        TEST(sg_slice, usage)
        {
            u32* p_arr = new u32[VECTOR_SIZE];
            for (u32 i = 0; i < VECTOR_SIZE; ++i)
                p_arr[i] = i;

            sg_slice slice = sg_slice_make(p_arr, VECTOR_SIZE / 2, VECTOR_SIZE / 2, sizeof(u32));
            for (u32 i = 0; i < slice._count; ++i)
                ASSERT_TRUE(*(u32*)sg_slice_data(&slice, i) == VECTOR_SIZE / 2 + i);

            sg_slice sub = sg_slice_to_slice(&slice, slice._count / 2, slice._count / 2);
            for (u32 i = 0; i < sub._count; ++i)
                ASSERT_TRUE(*(u32*)sg_slice_data(&sub, i) == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
            delete[] p_arr;
        }

        TEST(sg_slice, type_ext)
        {
            custom_type* p_arr = new custom_type[VECTOR_SIZE];
            for (u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                snprintf(p_arr[i].sz_message, custom_type::MAX_MSG, "%u", i);
                p_arr[i].hash = i;
            }

            custom_type_slice slice = custom_type_slice_make(p_arr, VECTOR_SIZE / 2, VECTOR_SIZE / 2);
            for (u32 i = 0; i < slice._count; ++i)
            {
                custom_type* p_type = custom_type_slice_data(&slice, i);
                ASSERT_TRUE((u32)atoi(p_type->sz_message) == VECTOR_SIZE / 2 + i);
                ASSERT_TRUE(p_type->hash == VECTOR_SIZE / 2 + i);
            }

            custom_type_slice sub = custom_type_slice_to_slice(&slice, slice._count / 2, slice._count / 2);
            for (u32 i = 0; i < sub._count; ++i)
            {
                custom_type* p_type = custom_type_slice_data(&sub, i);
                ASSERT_TRUE((u32)atoi(p_type->sz_message) == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
                ASSERT_TRUE(p_type->hash == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
            }

            delete[] p_arr;
        }

        TEST(sg_vector, create)
        {
            sg_vector vector = sg_vector_create(VECTOR_SIZE, sizeof(uint32_t), 0);

            ASSERT_FALSE(vector._buffer._allocation == 0);
            ASSERT_TRUE(vector._buffer._size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == VECTOR_SIZE);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            for (u32 i = 0; i < VECTOR_SIZE; ++i)
                ASSERT_TRUE(*(u32*)sg_vector_data(&vector, i) == 0);

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, create_empty)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);

            ASSERT_TRUE(vector._buffer._allocation == 0);
            ASSERT_TRUE(vector._buffer._size == 0);
            ASSERT_TRUE(vector._capacity == 0);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, destroy)
        {
            sg_vector vector = sg_vector_create(VECTOR_SIZE, sizeof(uint32_t), 0);
            sg_vector_destroy(&vector);

            ASSERT_TRUE(vector._buffer._allocation == 0);
            ASSERT_TRUE(vector._buffer._size == 0);
            ASSERT_TRUE(vector._capacity == 0);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == 0);
        }

        TEST(sg_vector, reserve)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            sg_vector_reserve(&vector, VECTOR_SIZE);

            ASSERT_FALSE(vector._buffer._allocation == 0);
            ASSERT_TRUE(vector._buffer._size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, resize)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            sg_vector_resize(&vector, VECTOR_SIZE);

            ASSERT_FALSE(vector._buffer._allocation == 0);
            ASSERT_TRUE(vector._buffer._size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == VECTOR_SIZE);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, emplace)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            for (u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                u32* ptr = (u32*)sg_vector_emplace(&vector);
                *ptr = i;
                ASSERT_TRUE(ptr == (u32*)sg_vector_data(&vector, i));
                ASSERT_TRUE(i == *(u32*)sg_vector_data(&vector, i));
            }

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, push)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            for (u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                u32 j = sg_vector_push(&vector, &i);
                ASSERT_TRUE(j == i);
                ASSERT_TRUE(i == *(u32*)sg_vector_data(&vector, j));
            }

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, type_ext)
        {
            custom_type_vector vector = custom_type_vector_create(0, 0);
            for (u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                if (i % 2 == 0)
                {
                    custom_type* p_type = custom_type_vector_emplace(&vector);
                    snprintf(p_type->sz_message, custom_type::MAX_MSG, "%u", i);
                    p_type->hash = i;
                }
                else
                {
                    custom_type type;
                    snprintf(type.sz_message, custom_type::MAX_MSG, "%u", i);
                    type.hash = i;
                    custom_type_vector_push(&vector, type);
                }
            }

            for (u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                custom_type* p_type = custom_type_vector_data(&vector, i);
                ASSERT_TRUE((u32)atoi(p_type->sz_message) == i);
                ASSERT_TRUE(p_type->hash == i);
            }

            custom_type_vector_destroy(&vector);
        }

        TEST(sg_hash_table, create)
        {
            sg_hash_table table = sg_hash_table_create(HASH_TABLE_SIZE, sizeof(uint32_t), HASH_TABLE_LOAD_FACTOR, NULL);

            ASSERT_FALSE(table._keys == NULL);
            ASSERT_FALSE(table._data == NULL);
            ASSERT_TRUE(table._capacity == HASH_TABLE_SIZE);
            ASSERT_TRUE(table._size == 0);
            ASSERT_TRUE(table._stride == sizeof(uint32_t));
            ASSERT_TRUE(table._load_factor == HASH_TABLE_LOAD_FACTOR);

            sg_hash_table_destroy(&table);
        }

        TEST(sg_hash_table, create_empty)
        {
            sg_hash_table table = sg_hash_table_create(0, sizeof(uint32_t), HASH_TABLE_LOAD_FACTOR, NULL);

            ASSERT_TRUE(table._keys == NULL);
            ASSERT_TRUE(table._data == NULL);
            ASSERT_TRUE(table._capacity == 0);
            ASSERT_TRUE(table._size == 0);
            ASSERT_TRUE(table._stride == sizeof(uint32_t));
            ASSERT_TRUE(table._load_factor == HASH_TABLE_LOAD_FACTOR);

            sg_hash_table_destroy(&table);
        }

        TEST(sg_hash_table, destroy)
        {
            sg_hash_table table = sg_hash_table_create(HASH_TABLE_SIZE, sizeof(uint32_t), HASH_TABLE_LOAD_FACTOR, NULL);

            sg_hash_table_destroy(&table);

            ASSERT_TRUE(table._keys == NULL);
            ASSERT_TRUE(table._data == NULL);
            ASSERT_TRUE(table._capacity == 0);
            ASSERT_TRUE(table._size == 0);
            ASSERT_TRUE(table._stride == 0);
            ASSERT_TRUE(table._load_factor == 0);
        }

        TEST(sg_hash_table, insert_find)
        {
            sg_hash_table table = sg_hash_table_create(0, sizeof(uint32_t), 0.6f, NULL);

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 2 == 0)
                {
                    sg_hash_table_insert(&table, i, &i);
                }
                else
                {
                    *(u32*)sg_hash_table_insert_inline(&table, i) = i;
                }
            }

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                u32* p = NULL;
                bool found = sg_hash_table_find(&table, i, (void**)&p);
                ASSERT_TRUE(found);
                ASSERT_TRUE(i == *p);
            }

            sg_hash_table_destroy(&table);
        }

        TEST(sg_hash_table, remove)
        {
            sg_hash_table table = sg_hash_table_create(0, sizeof(uint32_t), 0.6f, NULL);

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                sg_hash_table_insert(&table, i, &i);
            }

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 3 == 0)
                    sg_hash_table_remove(&table, i);
            }

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                u32* p = NULL;
                bool found = sg_hash_table_find(&table, i, (void**)&p);
                if (i % 3 == 0)
                {
                    ASSERT_FALSE(found);
                }
                else
                {
                    ASSERT_TRUE(found);
                    ASSERT_TRUE(i == *p);
                }
            }

            sg_hash_table_destroy(&table);
        }

        TEST(sg_hash_table, type_ext)
        {
            custom_type_table table = custom_type_table_create(0, 0.6f, NULL);

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 2 == 0)
                {
                    custom_type* p_type = custom_type_table_insert_inline(&table, i);
                    snprintf(p_type->sz_message, custom_type::MAX_MSG, "%u", i);
                    p_type->hash = i;
                }
                else
                {
                    custom_type type;
                    snprintf(type.sz_message, custom_type::MAX_MSG, "%u", i);
                    type.hash = i;
                    custom_type_table_insert(&table, i, type);
                }
            }

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 3 == 0)
                    custom_type_table_remove(&table, i);
            }

            for (u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                custom_type* p_type = NULL;
                bool found = custom_type_table_find(&table, i, &p_type);
                if (i % 3 == 0)
                {
                    ASSERT_FALSE(found);
                }
                else
                {
                    ASSERT_TRUE((u32)atoi(p_type->sz_message) == i);
                    ASSERT_TRUE(p_type->hash == i);
                }
            }

            custom_type_table_destroy(&table);
        }
    }
}
