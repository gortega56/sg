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
    sg_u32 hash = 0;
};

struct edge
{
    sg_u32 _i0, _i1;

    edge(sg_u32 i0, sg_u32 i1) : _i0(i0), _i1(i1)
    {
        if (_i1 < _i0)
        {
            sg_u32 t = _i0;
            _i0 = _i1;
            _i1 = t;
        }
    }

    sg_u32 key()
    {
        sg_u64 k = ((sg_u64)_i0 << 32) | (sg_u64)_i1;

        static const sg_u32 basis = 0x811c9dc5;
        static const sg_u32 prime = 0x01000193;

        const sg_u8* p_bytes = reinterpret_cast<const sg_u8*>(&k);
        sg_u32 hash = basis;
        for (sg_u32 ibyte = 0; ibyte < sizeof(k); ++ibyte)
        {
            hash ^= p_bytes[ibyte];
            hash *= prime;
        }

        return hash;
    }
};

SG_SLICE_DEFINE_TYPE_EXT(custom_type_slice, custom_type)
SG_VECTOR_DEFINE_TYPE_EXT(custom_type_vector, custom_type)
SG_HASH_TABLE_DEFINE_TYPE_EXT(edge_type_table, edge);

#define HASH_TABLE_LOAD_FACTOR 0.6f
#define HASH_TABLE_SIZE 4096
#define PLANE_ROWS 1024
#define PLANE_COLS 1024

static const sg_u32 NUM_PLANE_IDX = PLANE_ROWS * PLANE_COLS * 3 * 2;
static const sg_u32 NUM_PLANE_TRI = NUM_PLANE_IDX / 3;

static sg_u32* create_idx_buf_plane(sg_u32 rows, sg_u32 cols)
{
    sg_u32* vtx_idx_data = new sg_u32[NUM_PLANE_IDX];
    sg_u32  vtx_idx = 0;
    for (sg_u32 vtx_idx_y = 0; vtx_idx_y < rows; ++vtx_idx_y)
    {
        for (sg_u32 vtx_idx_x = 0; vtx_idx_x < cols; ++vtx_idx_x)
        {
            sg_u32 i0 = vtx_idx_y * (cols + 1) + vtx_idx_x;
            sg_u32 i1 = i0 + 1;
            sg_u32 i2 = i0 + cols + 1;
            sg_u32 i3 = i2 + 1;

            vtx_idx_data[vtx_idx++] = i0;
            vtx_idx_data[vtx_idx++] = i3;
            vtx_idx_data[vtx_idx++] = i1;

            vtx_idx_data[vtx_idx++] = i0;
            vtx_idx_data[vtx_idx++] = i2;
            vtx_idx_data[vtx_idx++] = i3;
        }
    }

    return vtx_idx_data;
}

static void destroy_idx_buf_plane(sg_u32* vtx_idx_data)
{
    delete[] vtx_idx_data;
}

namespace sg
{
    namespace unit_tests
    {
        TEST(sg_slice, usage)
        {
            sg_u32* p_arr = new sg_u32[VECTOR_SIZE];
            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
                p_arr[i] = i;

            sg_slice slice = sg_slice_make(p_arr, VECTOR_SIZE / 2, VECTOR_SIZE / 2, sizeof(sg_u32));
            for (sg_u32 i = 0; i < slice._count; ++i)
                ASSERT_TRUE(*(sg_u32*)sg_slice_data(&slice, i) == VECTOR_SIZE / 2 + i);

            sg_slice sub = sg_slice_to_slice(&slice, slice._count / 2, slice._count / 2);
            for (sg_u32 i = 0; i < sub._count; ++i)
                ASSERT_TRUE(*(sg_u32*)sg_slice_data(&sub, i) == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
            delete[] p_arr;
        }

        TEST(sg_slice, type_ext)
        {
            custom_type* p_arr = new custom_type[VECTOR_SIZE];
            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                snprintf(p_arr[i].sz_message, custom_type::MAX_MSG, "%u", i);
                p_arr[i].hash = i;
            }

            custom_type_slice slice = custom_type_slice_make(p_arr, VECTOR_SIZE / 2, VECTOR_SIZE / 2);
            for (sg_u32 i = 0; i < slice._count; ++i)
            {
                custom_type* p_type = custom_type_slice_data(&slice, i);
                ASSERT_TRUE((sg_u32)atoi(p_type->sz_message) == VECTOR_SIZE / 2 + i);
                ASSERT_TRUE(p_type->hash == VECTOR_SIZE / 2 + i);
            }

            custom_type_slice sub = custom_type_slice_to_slice(&slice, slice._count / 2, slice._count / 2);
            for (sg_u32 i = 0; i < sub._count; ++i)
            {
                custom_type* p_type = custom_type_slice_data(&sub, i);
                ASSERT_TRUE((sg_u32)atoi(p_type->sz_message) == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
                ASSERT_TRUE(p_type->hash == (VECTOR_SIZE - VECTOR_SIZE / 4) + i);
            }

            delete[] p_arr;
        }

        TEST(sg_vector, create)
        {
            sg_vector vector = sg_vector_create(VECTOR_SIZE, sizeof(uint32_t), 0);

            ASSERT_FALSE(vector._buffer.allocation == 0);
            ASSERT_TRUE(vector._buffer.size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == VECTOR_SIZE);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
                ASSERT_TRUE(*(sg_u32*)sg_vector_data(&vector, i) == 0);

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, create_empty)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);

            ASSERT_TRUE(vector._buffer.allocation == 0);
            ASSERT_TRUE(vector._buffer.size == 0);
            ASSERT_TRUE(vector._capacity == 0);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, destroy)
        {
            sg_vector vector = sg_vector_create(VECTOR_SIZE, sizeof(uint32_t), 0);
            sg_vector_destroy(&vector);

            ASSERT_TRUE(vector._buffer.allocation == 0);
            ASSERT_TRUE(vector._buffer.size == 0);
            ASSERT_TRUE(vector._capacity == 0);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == 0);
        }

        TEST(sg_vector, reserve)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            sg_vector_reserve(&vector, VECTOR_SIZE);

            ASSERT_FALSE(vector._buffer.allocation == 0);
            ASSERT_TRUE(vector._buffer.size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == 0);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, resize)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            sg_vector_resize(&vector, VECTOR_SIZE);

            ASSERT_FALSE(vector._buffer.allocation == 0);
            ASSERT_TRUE(vector._buffer.size == vector._capacity * vector._stride);
            ASSERT_TRUE(vector._capacity == VECTOR_SIZE);
            ASSERT_TRUE(vector._size == VECTOR_SIZE);
            ASSERT_TRUE(vector._stride == sizeof(uint32_t));

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, emplace)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                sg_u32* ptr = (sg_u32*)sg_vector_emplace(&vector);
                *ptr = i;
                ASSERT_TRUE(ptr == (sg_u32*)sg_vector_data(&vector, i));
                ASSERT_TRUE(i == *(sg_u32*)sg_vector_data(&vector, i));
            }

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, push)
        {
            sg_vector vector = sg_vector_create(0, sizeof(uint32_t), 0);
            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                sg_u32 j = sg_vector_push(&vector, &i);
                ASSERT_TRUE(j == i);
                ASSERT_TRUE(i == *(sg_u32*)sg_vector_data(&vector, j));
            }

            sg_vector_destroy(&vector);
        }

        TEST(sg_vector, type_ext)
        {
            custom_type_vector vector = custom_type_vector_create(0, 0);
            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
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

            for (sg_u32 i = 0; i < VECTOR_SIZE; ++i)
            {
                custom_type* p_type = custom_type_vector_data(&vector, i);
                ASSERT_TRUE((sg_u32)atoi(p_type->sz_message) == i);
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

            ASSERT_TRUE(table._keys != NULL);
            ASSERT_TRUE(table._data != NULL);
            ASSERT_TRUE(table._capacity != 0);
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

            for (sg_u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 2 == 0)
                {
                    sg_hash_table_insert(&table, i, &i);
                }
                else
                {
                    *(sg_u32*)sg_hash_table_emplace(&table, i) = i;
                }
            }

            for (sg_u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                sg_u32* p = NULL;
                bool found = sg_hash_table_find(&table, i, (void**)&p);
                ASSERT_TRUE(found);
                ASSERT_TRUE(i == *p);
            }

            sg_hash_table_destroy(&table);
        }

        TEST(sg_hash_table, remove)
        {
            sg_hash_table table = sg_hash_table_create(0, sizeof(uint32_t), 0.6f, NULL);

            for (sg_u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                sg_hash_table_insert(&table, i, &i);
            }

            for (sg_u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                if (i % 3 == 0)
                    sg_hash_table_remove(&table, i);
            }

            for (sg_u32 i = 0; i < HASH_TABLE_SIZE; ++i)
            {
                sg_u32* p = NULL;
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
            sg_u32* vtx_idx_data = create_idx_buf_plane(PLANE_ROWS, PLANE_COLS);
            edge_type_table table = edge_type_table_create(0, 0.6f, NULL);

            for (sg_u32 tri_idx = 0; tri_idx < NUM_PLANE_TRI; ++tri_idx)
            {
                sg_u32 i0 = vtx_idx_data[tri_idx * 3 + 0];
                sg_u32 i1 = vtx_idx_data[tri_idx * 3 + 1];
                sg_u32 i2 = vtx_idx_data[tri_idx * 3 + 2];

                edge e0 = { i0, i1 };
                edge e1 = { i0, i2 };
                edge e2 = { i1, i2 };

                sg_u32 k0 = e0.key();
                sg_u32 k1 = e1.key();
                sg_u32 k2 = e2.key();

                if (!edge_type_table_find(&table, k0, 0)) edge_type_table_insert(&table, k0, e0);
                if (!edge_type_table_find(&table, k1, 0)) edge_type_table_insert(&table, k1, e1);
                if (!edge_type_table_find(&table, k2, 0)) edge_type_table_insert(&table, k2, e2);
            }

            for (sg_u32 tri_idx = 0; tri_idx < NUM_PLANE_TRI; ++tri_idx)
            {
                sg_u32 i0 = vtx_idx_data[tri_idx * 3 + 0];
                sg_u32 i1 = vtx_idx_data[tri_idx * 3 + 1];
                sg_u32 i2 = vtx_idx_data[tri_idx * 3 + 2];

                edge e0 = { i0, i1 };
                edge e1 = { i0, i2 };
                edge e2 = { i1, i2 };

                sg_u32 k0 = e0.key();
                sg_u32 k1 = e1.key();
                sg_u32 k2 = e2.key();

                edge* p0 = nullptr;
                edge* p1 = nullptr;
                edge* p2 = nullptr;

                bool f0 = edge_type_table_find(&table, k0, &p0);
                bool f1 = edge_type_table_find(&table, k1, &p1);
                bool f2 = edge_type_table_find(&table, k2, &p2);

                ASSERT_TRUE(f0);
                ASSERT_TRUE(f1);
                ASSERT_TRUE(f2);
            }

            edge_type_table_destroy(&table);
            destroy_idx_buf_plane(vtx_idx_data);
        }
    }
}
