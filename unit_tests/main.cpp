#include <functional>
#include <gtest/gtest.h>

#define SG_VECTOR_DEFINE_IMPLEMENTATION
#define SG_VECTOR_DEFINE_STANDARD_INTERFACES
#define SG_VECTOR_DEFINE_STANDARD_IMPLEMENTATIONS
#include "../include/sg_vector.h"

#define DEBUG_TEST_ENABLE_CREATE    1
#define DEBUG_TEST_ENABLE_DESTROY   1
#define DEBUG_TEST_ENABLE_RESERVE   1
#define DEBUG_TEST_ENABLE_RESIZE    1
#define DEBUG_TEST_ENABLE_AT        1
#define DEBUG_TEST_ENABLE_DATA      1
#define DEBUG_TEST_ENABLE_BACK      1
#define DEBUG_TEST_ENABLE_PUSH      1

enum e_vector_element_types
{
    VE_BOOL,
    VE_SIZE,
    VE_UINT8,
    VE_UINT16,
    VE_UINT32,
    VE_UINT64,
    VE_INT8,
    VE_INT16,
    VE_INT32,
    VE_INT64,
    VE_STRUCT,
    VE_COUNT
};

#define define_function_array(arr, function)         \
            arr[VE_BOOL]    = &function ## _bool;    \
            arr[VE_SIZE]    = &function ## _size_t;  \
            arr[VE_UINT8]   = &function ## _uint8_t; \
            arr[VE_UINT16]  = &function ## _uint16_t;\
            arr[VE_UINT32]  = &function ## _uint32_t;\
            arr[VE_UINT64]  = &function ## _uint64_t;\
            arr[VE_INT8]    = &function ## _int8_t;  \
            arr[VE_INT16]   = &function ## _int16_t; \
            arr[VE_INT32]   = &function ## _int32_t; \
            arr[VE_INT64]   = &function ## _int64_t; \
            arr[VE_STRUCT]  = &function ## _user_struct

struct user_struct
{
    const char* sz_message = nullptr;
    uint32_t id = ~0U;
};

extern "C" 
{
    sg_vector_define_type_interface(user_struct);
    sg_vector_define_type_implementation(user_struct);
}

namespace sg
{
    namespace unit_tests
    {
        static constexpr unsigned int s_sg_vector_type_count = VE_COUNT;
        
        static constexpr unsigned int s_sg_vector_size_count = 8;
        
        static sg_vec s_arr_vectors[s_sg_vector_type_count][s_sg_vector_size_count];
        
        static const char* s_arr_types[s_sg_vector_type_count] = 
        {
            "bool",
            "size_t",
            "uint8_t",
            "uint16_t",
            "uint32_t",
            "uint64_t",
            "int8_t",
            "int16_t",
            "int32_t",
            "int64_t",
            "user_struct"
        };

        static uint32_t s_arr_strides[s_sg_vector_type_count] = 
        {
            sizeof(bool),
            sizeof(size_t),
            sizeof(uint8_t),
            sizeof(uint16_t),
            sizeof(uint32_t),
            sizeof(uint64_t),
            sizeof(int8_t),
            sizeof(int16_t),
            sizeof(int32_t),
            sizeof(int64_t),
            sizeof(user_struct)
        };

        static uint32_t s_arr_sizes[s_sg_vector_size_count] =
        {
            0,
            1 << 1,
            1 << 16,
            1 << 24,
            (1 << 1) | 1,
            (1 << 7) | 1,
            (1 << 19)| 1,
            (1 << 24)| 1,
        };

        #if DEBUG_TEST_ENABLE_CREATE
        TEST(sg_vec, create)
        {
            typedef void(create)(sg_vec*, sg_vector_allocator*, uint32_t);
            create* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_create);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    sg_vec vector;
                    std::memset(&vector, 0, sizeof(sg_vec));

                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    arr_function[type](&vector, nullptr, element_count);

                    if (element_count)
                    {
                        ASSERT_TRUE(vector.mp_data != nullptr);
                        ASSERT_TRUE(vector.m_data_size != 0);
                        ASSERT_TRUE(element_stride == vector.m_data_size / vector.m_element_count);
                    }
                    else
                    {
                        ASSERT_TRUE(vector.mp_data == nullptr);
                        ASSERT_TRUE(vector.m_data_size == 0);
                    }
            
                    ASSERT_TRUE(vector.m_element_count == element_count);
                    ASSERT_TRUE(vector.m_element_stride == element_stride);
                    ASSERT_TRUE(vector.m_data_size == vector.m_element_stride * vector.m_element_count);
                
                    if (vector.mp_data)
                        free(vector.mp_data);
                }
            }
        }
        #endif

        #if DEBUG_TEST_ENABLE_DESTROY
        TEST(sg_vec, destroy)
        {
            typedef void(destroy)(sg_vec*);
            destroy* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_destroy);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_data_size = element_stride * element_count;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;

                    arr_function[type](&vector);

                    ASSERT_TRUE(vector.mp_data == nullptr);
                    ASSERT_TRUE(vector.m_data_size == 0);
                    ASSERT_TRUE(vector.m_element_stride == 0);
                    ASSERT_TRUE(vector.m_element_count == 0);
                }
            }
        }
        #endif

        #if DEBUG_TEST_ENABLE_RESERVE
        TEST(sg_vec, reserve_greater_than)
        {
            typedef void(reserve)(sg_vec*, uint32_t);
            reserve* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_reserve);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    uint32_t reserve_count = (element_count + 1) * 2;
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_data_size = element_stride * element_count;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;

                    arr_function[type](&vector, reserve_count);

                    ASSERT_TRUE(vector.m_data_size == element_stride * reserve_count);
                    ASSERT_TRUE(vector.m_element_stride == element_stride);
                    ASSERT_TRUE(vector.m_element_count == element_count);

                    // p_data will be freed by sg_vector_reserve

                    if (vector.mp_data)
                        free(vector.mp_data);
                }
            }
        }
        #endif

        #if DEBUG_TEST_ENABLE_RESERVE
        TEST(sg_vec, reserve_less_than_or_equal_to)
        {
            typedef void(reserve)(sg_vec*, uint32_t);
            reserve* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_reserve);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    uint32_t reserve_count = element_count - (element_count % 2);
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_data_size = element_stride * element_count;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;

                    arr_function[type](&vector, reserve_count);

                    if (vector.mp_data != p_data)
                        int xxx = 0;

                    ASSERT_TRUE(vector.mp_data == p_data);
                    ASSERT_TRUE(vector.m_data_size == element_stride * element_count);
                    ASSERT_TRUE(vector.m_element_stride == element_stride);
                    ASSERT_TRUE(vector.m_element_count == element_count);

                    if (p_data)
                        free(p_data);
                }
            }
        }
       #endif

        #if DEBUG_TEST_ENABLE_RESIZE
        TEST(sg_vec, resize_greater_than)
        {
            typedef void(resize)(sg_vec*, uint32_t);
            resize* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_resize);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    uint32_t resize_count = (element_count + 1) * 2;
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_data_size = element_stride * element_count;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;

                    arr_function[type](&vector, resize_count);

                    ASSERT_TRUE(vector.m_data_size == element_stride * resize_count);
                    ASSERT_TRUE(vector.m_element_stride == element_stride);
                    ASSERT_TRUE(vector.m_element_count == resize_count);

                    // p_data will be freed by sg_vector_resize

                    if (vector.mp_data)
                        free(vector.mp_data);                }
            }
        }
        #endif
        
        #if DEBUG_TEST_ENABLE_RESIZE
        TEST(sg_vec, resize_less_than_or_equal_to)
        {
            typedef void(resize)(sg_vec*, uint32_t);
            resize* arr_function[s_sg_vector_type_count];
            define_function_array(arr_function, sg_vector_resize);

            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size];
                    uint32_t resize_count = element_count - (element_count % 2);
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_data_size = element_stride * element_count;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;

                    arr_function[type](&vector, resize_count);

                    ASSERT_TRUE(vector.mp_data == p_data);
                    ASSERT_TRUE(vector.m_data_size == element_stride * element_count);
                    ASSERT_TRUE(vector.m_element_stride == element_stride);
                    ASSERT_TRUE(vector.m_element_count == resize_count);

                    if (p_data)
                        free(p_data);
                }
            }
        }
        #endif

        #if DEBUG_TEST_ENABLE_AT
        TEST(sg_vec, at)
        {
            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    uint32_t element_stride = s_arr_strides[type];
                    uint32_t element_count = s_arr_sizes[size] + 1;
                    void* p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;
                    vector.m_data_size = element_stride * element_count;

                    for (uint32_t element = 0; element < element_count; ++element)
                    {
                        char buffer[256];
                        const char* sz_element = itoa(element, buffer, 10);
                        switch (type)
                        {
                            case VE_BOOL:   *sg_vector_at_bool(&vector, element) = (bool)(element % 2); break;
                            case VE_SIZE:   *sg_vector_at_size_t(&vector, element) = (size_t)element; break;
                            case VE_UINT8:  *sg_vector_at_uint8_t(&vector, element) = (uint8_t)element % UINT8_MAX; break;
                            case VE_UINT16: *sg_vector_at_uint16_t(&vector, element) = (uint16_t)element % UINT16_MAX; break;
                            case VE_UINT32: *sg_vector_at_uint32_t(&vector, element) = (uint32_t)element % UINT32_MAX; break;
                            case VE_UINT64: *sg_vector_at_uint64_t(&vector, element) = (uint64_t)element; break;
                            case VE_INT8:   *sg_vector_at_int8_t(&vector, element) = (int8_t)element % INT8_MAX; break;
                            case VE_INT16:  *sg_vector_at_int16_t(&vector, element) = (int16_t)element % INT16_MAX; break;
                            case VE_INT32:  *sg_vector_at_int32_t(&vector, element) = (int32_t)element % INT32_MAX; break;
                            case VE_INT64:  *sg_vector_at_int64_t(&vector, element) = (int64_t)element; break;
                            case VE_STRUCT: *sg_vector_at_user_struct(&vector, element) = { sz_element, (uint32_t)element % UINT32_MAX }; break;
                            default: break;
                        }
                    }

                    for (uint32_t element = 0; element < element_count; ++element)
                    {
                        char buffer[256];
                        const char* sz_element = itoa(element, buffer, 10);
                        bool b_equals = false;
                        switch (type)
                        {
                            case VE_BOOL:   b_equals = (((bool*)p_data)[element] == (bool)(element % 2)); break;
                            case VE_SIZE:   b_equals = (((size_t*)p_data)[element] == (size_t)element); break;
                            case VE_UINT8:  b_equals = (((uint8_t*)p_data)[element] == (uint8_t)element % UINT8_MAX); break;
                            case VE_UINT16: b_equals = (((uint16_t*)p_data)[element] == (uint16_t)element % UINT16_MAX); break;
                            case VE_UINT32: b_equals = (((uint32_t*)p_data)[element] == (uint32_t)element % UINT32_MAX); break;
                            case VE_UINT64: b_equals = (((uint64_t*)p_data)[element] == (uint64_t)element); break;
                            case VE_INT8:   b_equals = (((int8_t*)p_data)[element] == (int8_t)element % INT8_MAX); break;
                            case VE_INT16:  b_equals = (((int16_t*)p_data)[element] == (int16_t)element % INT16_MAX); break;
                            case VE_INT32:  b_equals = (((int32_t*)p_data)[element] == (int32_t)element % INT32_MAX); break;
                            case VE_INT64:  b_equals = (((int64_t*)p_data)[element] == (int64_t)element); break;
                            case VE_STRUCT: 
                            {
                                user_struct* p_struct = &((user_struct*)p_data)[element];
                                b_equals = (strcmp(sz_element, p_struct->sz_message) == 0);
                                b_equals = (p_struct->id == (uint32_t)element % UINT32_MAX); 
                            } break;
                            default: break;
                        }
                        
                        ASSERT_TRUE(b_equals);
                    }

                    if (p_data)
                        free(p_data);
                }
            }
        }
        #endif
    
        #if DEBUG_TEST_ENABLE_DATA
        TEST(sg_vec, data)
        {
            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                size_t element_stride = s_arr_strides[type];
                size_t element_count = s_arr_sizes[1] + 1;
                void* p_data = malloc(element_stride * element_count);

                sg_vec vector;
                vector.m_allocator.p_malloc = &malloc;
                vector.m_allocator.p_realloc = &realloc;
                vector.m_allocator.p_free = &free;
                vector.mp_data = (uint8_t*)p_data;
                vector.m_element_hash = element_hash;
                vector.m_element_stride = element_stride;
                vector.m_element_count = element_count;
                vector.m_data_size = element_stride * element_count;

                for (uint32_t element = 0; element < element_count; ++element)
                {
                    char buffer[256];
                    const char* sz_element = itoa(element, buffer, 10);
                    switch (type)
                    {
                        case VE_BOOL:   ASSERT_TRUE(sg_vector_data_bool(&vector) == p_data); break;
                        case VE_SIZE:   ASSERT_TRUE(sg_vector_data_size_t(&vector) == p_data); break;
                        case VE_UINT8:  ASSERT_TRUE(sg_vector_data_uint8_t(&vector) == p_data); break;
                        case VE_UINT16: ASSERT_TRUE(sg_vector_data_uint16_t(&vector) == p_data); break;
                        case VE_UINT32: ASSERT_TRUE(sg_vector_data_uint32_t(&vector) == p_data); break;
                        case VE_UINT64: ASSERT_TRUE(sg_vector_data_uint64_t(&vector) == p_data); break;
                        case VE_INT8:   ASSERT_TRUE(sg_vector_data_int8_t(&vector) == p_data); break;
                        case VE_INT16:  ASSERT_TRUE(sg_vector_data_int16_t(&vector) == p_data); break;
                        case VE_INT32:  ASSERT_TRUE(sg_vector_data_int32_t(&vector) == p_data); break;
                        case VE_INT64:  ASSERT_TRUE(sg_vector_data_int64_t(&vector) == p_data); break;
                        case VE_STRUCT: ASSERT_TRUE(sg_vector_data_user_struct(&vector) == p_data); break;
                        default: break;
                    }
                }

                if (p_data)
                    free(p_data);
            }
        }
        #endif
    
        #if DEBUG_TEST_ENABLE_BACK
        TEST(sg_vec, back)
        {
            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                size_t element_stride = s_arr_strides[type];
                size_t element_count = s_arr_sizes[1] + 1;
                void* p_data = malloc(element_stride * element_count);

                sg_vec vector;
                vector.m_allocator.p_malloc = &malloc;
                vector.m_allocator.p_realloc = &realloc;
                vector.m_allocator.p_free = &free;
                vector.mp_data = (uint8_t*)p_data;
                vector.m_element_hash = element_hash;
                vector.m_element_stride = element_stride;
                vector.m_element_count = element_count;
                vector.m_data_size = element_stride * element_count;

                for (uint32_t element = 0; element < element_count; ++element)
                {
                    char buffer[256];
                    const char* sz_element = itoa(element, buffer, 10);
                    bool b_equals = false;
                    switch (type)
                    {
                        case VE_BOOL:   b_equals = (sg_vector_back_bool(&vector) == (bool*)p_data + (element_count - 1)); break;
                        case VE_SIZE:   b_equals = (sg_vector_back_size_t(&vector) == (size_t*)p_data + (element_count - 1)); break;
                        case VE_UINT8:  b_equals = (sg_vector_back_uint8_t(&vector) == (uint8_t*)p_data + (element_count - 1)); break;
                        case VE_UINT16: b_equals = (sg_vector_back_uint16_t(&vector) == (uint16_t*)p_data + (element_count - 1)); break;
                        case VE_UINT32: b_equals = (sg_vector_back_uint32_t(&vector) == (uint32_t*)p_data + (element_count - 1)); break;
                        case VE_UINT64: b_equals = (sg_vector_back_uint64_t(&vector) == (uint64_t*)p_data + (element_count - 1)); break;
                        case VE_INT8:   b_equals = (sg_vector_back_int8_t(&vector) == (int8_t*)p_data + (element_count - 1)); break;
                        case VE_INT16:  b_equals = (sg_vector_back_int16_t(&vector) == (int16_t*)p_data + (element_count - 1)); break;
                        case VE_INT32:  b_equals = (sg_vector_back_int32_t(&vector) == (int32_t*)p_data + (element_count - 1)); break;
                        case VE_INT64:  b_equals = (sg_vector_back_int64_t(&vector) == (int64_t*)p_data + (element_count - 1)); break;
                        case VE_STRUCT: b_equals = (sg_vector_back_user_struct(&vector) == (user_struct*)p_data + (element_count - 1)); break;
                        default: break;
                    }

                    ASSERT_TRUE(b_equals);
                }

                if (p_data)
                    free(p_data);
            }
        }
        #endif

        #if DEBUG_TEST_ENABLE_PUSH
        TEST(sg_vec, push)
        {
            for (int type = 0; type < s_sg_vector_type_count; ++type)
            {
                for (int size = 0; size < s_sg_vector_size_count; ++size)
                {
                    uint64_t element_hash = sg_vector_internal_element_type_hash(s_arr_types[type]);
                    size_t element_stride = s_arr_strides[type];
                    size_t element_count = s_arr_sizes[size];
                    void* p_data = nullptr;
                    if (element_count)
                        p_data = malloc(element_stride * element_count);

                    sg_vec vector;
                    vector.m_allocator.p_malloc = &malloc;
                    vector.m_allocator.p_realloc = &realloc;
                    vector.m_allocator.p_free = &free;
                    vector.mp_data = (uint8_t*)p_data;
                    vector.m_element_hash = element_hash;
                    vector.m_element_stride = element_stride;
                    vector.m_element_count = element_count;
                    vector.m_data_size = element_stride * element_count;

                    size_t element_start = element_count;
                    for (uint32_t element = 0; element < element_count; ++element)
                    {
                        size_t push_element = element_start + element;
                        char buffer[256];
                        const char* sz_element = itoa(push_element, buffer, 10);
                        switch (type)
                        {
                            case VE_BOOL:   sg_vector_push_bool(&vector, (bool)(push_element % 2)); break;
                            case VE_SIZE:   sg_vector_push_size_t(&vector, (size_t)push_element); break;
                            case VE_UINT8:  sg_vector_push_uint8_t(&vector, (uint8_t)push_element % UINT8_MAX); break;
                            case VE_UINT16: sg_vector_push_uint16_t(&vector, (uint16_t)push_element % UINT16_MAX); break;
                            case VE_UINT32: sg_vector_push_uint32_t(&vector, (uint32_t)push_element % UINT32_MAX); break;
                            case VE_UINT64: sg_vector_push_uint64_t(&vector, (uint64_t)push_element); break;
                            case VE_INT8:   sg_vector_push_int8_t(&vector, (int8_t)push_element % INT8_MAX); break;
                            case VE_INT16:  sg_vector_push_int16_t(&vector, (int16_t)push_element % INT16_MAX); break;
                            case VE_INT32:  sg_vector_push_int32_t(&vector, (int32_t)push_element % INT32_MAX); break;
                            case VE_INT64:  sg_vector_push_int64_t(&vector, (int64_t)push_element); break;
                            case VE_STRUCT: sg_vector_push_user_struct(&vector, { sz_element, (uint32_t)push_element % UINT32_MAX }); break;
                            default: break;
                        }
                    }

                    for (uint32_t element = 0; element < element_count; ++element)
                    {
                        size_t push_element = element_start + element;
                        char buffer[256];
                        const char* sz_element = itoa(element, buffer, 10);
                        bool b_equals = false;
                        switch (type)
                        {
                            case VE_BOOL:   b_equals = (((bool*)vector.mp_data)[push_element] == (bool)(push_element % 2)); break;
                            case VE_SIZE:   b_equals = (((size_t*)vector.mp_data)[push_element] == (size_t)push_element); break;
                            case VE_UINT8:  b_equals = (((uint8_t*)vector.mp_data)[push_element] == (uint8_t)push_element % UINT8_MAX); break;
                            case VE_UINT16: b_equals = (((uint16_t*)vector.mp_data)[push_element] == (uint16_t)push_element % UINT16_MAX); break;
                            case VE_UINT32: b_equals = (((uint32_t*)vector.mp_data)[push_element] == (uint32_t)push_element % UINT32_MAX); break;
                            case VE_UINT64: b_equals = (((uint64_t*)vector.mp_data)[push_element] == (uint64_t)push_element); break;
                            case VE_INT8:   b_equals = (((int8_t*)vector.mp_data)[push_element] == (int8_t)push_element % INT8_MAX); break;
                            case VE_INT16:  b_equals = (((int16_t*)vector.mp_data)[push_element] == (int16_t)push_element % INT16_MAX); break;
                            case VE_INT32:  b_equals = (((int32_t*)vector.mp_data)[push_element] == (int32_t)push_element % INT32_MAX); break;
                            case VE_INT64:  b_equals = (((int64_t*)vector.mp_data)[push_element] == (int64_t)push_element); break;
                            case VE_STRUCT: 
                            {
                                user_struct* p_struct = &((user_struct*)vector.mp_data)[push_element];
                                b_equals = (strcmp(sz_element, p_struct->sz_message) == 0);
                                b_equals = (p_struct->id == (uint32_t)push_element % UINT32_MAX); 
                            } break;
                            default: break;
                        }

                        ASSERT_TRUE(b_equals);
                    }

                    ASSERT_TRUE(vector.m_element_count == element_start + element_count);

                    if (vector.mp_data)
                        free(vector.mp_data);
                }
            }
        }
        #endif
    }
}
