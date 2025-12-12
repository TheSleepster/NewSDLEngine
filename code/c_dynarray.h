#if !defined(C_DYNARRAY_H)
/* ========================================================================
   $File: c_dynarray.h $
   $Date: November 30 2025 08:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_DYNARRAY_H
#include <stdio.h>
#include <string.h>

#include <c_base.h>
#include <c_types.h>

typedef struct dynarray_header 
{
    u32   flags;
    u32   size;
    u32   capacity;
    u32   _reserved;
}dynarray_header_t;

StaticAssert(sizeof(dynarray_header_t) % 16 == 0, "Dynamic Array header must be 16 byte aligned");


#define DYNARRAY_INITIAL_SIZE  (4)
#define DYNARRAY_GROWTH_FACTOR (2)

#define DynArray(type) TypeOf((type*)null)

void* _dynarray_create_impl(u32 element_size);
void  _dynarray_destroy_impl(void **array);
void* _dynarray_grow_impl(void **array, u32 element_size, u32 new_capacity);
void  _dynarray_insert_impl(void **array, void *element, u32 element_size, u32 index);
void  _dynarray_remove_impl(void **array, u32 element_size, u32 index);

#define _dynarray_header(d_array_ptr) \
    ((dynarray_header_t*)(d_array_ptr ? ((byte*)(d_array_ptr) - sizeof(dynarray_header_t)) : null))

#define c_dynarray_create(type) ({              \
    (type*)_dynarray_create_impl(sizeof(type)); \
 })

#define c_dynarray_destroy(d_array) ({        \
    _dynarray_destroy_impl((void**)&d_array); \
    d_array = null;                           \
                                              \
    d_array;                                  \
})

#define c_dynarray_reserve(d_array, to_reserve) ({                                                                                     \
    TypeOf(d_array)   *p_array = &(d_array);                                                                                           \
    dynarray_header_t *header  = (dynarray_header_t *)_dynarray_header(d_array);                                                       \
    Expect(header, "D_array header is invalid for macro reserve()...\n");                                                              \
    if(header->capacity < to_reserve) {                                                                                                \
        *p_array = (TypeOf(d_array))_dynarray_grow_impl((void**)p_array, sizeof(*d_array), header->capacity * DYNARRAY_GROWTH_FACTOR); \
        header = _dynarray_header(*p_array);                                                                                           \
    }                                                                                                                                  \
                                                                                                                                       \
    p_array;                                                                                                                           \
})

#define c_dynarray_add_element(d_array, element, index) ({                                                                             \
    TypeOf(d_array) *p_first = &(d_array);                                                                                             \
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(d_array);                                                         \
    if(header == null) {                                                                                                               \
        *p_first = (TypeOf(d_array))_dynarray_create_impl(sizeof(*d_array));                                                           \
        header = (dynarray_header_t*)_dynarray_header(*p_first);                                                                       \
    }                                                                                                                                  \
                                                                                                                                       \
    if(header->size + 1 >= header->capacity) {                                                                                         \
        *p_first = (TypeOf(d_array))_dynarray_grow_impl((void**)p_first, sizeof(*d_array), header->capacity * DYNARRAY_GROWTH_FACTOR); \
        header = (dynarray_header_t*)_dynarray_header(*p_first);                                                                       \
    }                                                                                                                                  \
    Expect(header, "Header for macro add_element is invalid...\n");                                                                    \
    Expect(index < header->capacity, "index is >= capacity in add\n");                                                                 \
    _dynarray_insert_impl((void**)p_first, (void*)&(element), sizeof(*d_array), index);                                                \
    header->size += 1;                                                                                                                 \
                                                                                                                                       \
    p_first;                                                                                                                           \
})

#define c_dynarray_remove_element(d_array, index) ({                    \
    TypeOf(d_array) *p_first = &(d_array);                              \
    TypeOf(*(d_array)) value = c_dynarray_get_at_index(d_array, index); \
    _dynarray_remove_impl((void**)p_first, sizeof(d_array[0]), index);  \
    value;                                                              \
})

#define c_dynarray_push(d_array, value) ({                                       \
    TypeOf(d_array) *p_first = &(d_array);                                       \
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header((d_array)); \
    if(!header) {                                                                \
        *p_first = (TypeOf(d_array))_dynarray_create_impl(sizeof(*d_array));     \
        header = (dynarray_header_t*)_dynarray_header((d_array));                \
    }                                                                            \
    u32 index = header->size;                                                    \
    p_first = c_dynarray_add_element(d_array, value, index);                     \
    p_first;                                                                     \
})

#define c_dynarray_pop(d_array) ({                                                     \
        dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(d_array);     \
        Expect(header, "DArray header is invalid...\n");                               \
        TypeOf(*(d_array)) r_value = c_dynarray_remove_element(d_array, header->size); \
                                                                                       \
        r_value;                                                                       \
    })

#define c_dynarray_get_at_index(d_array, index) ({                             \
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(d_array); \
    TypeOf(*(d_array)) value = {};                                             \
    Expect(header, "Invalid d_array header...\n");                             \
    if(header && (index) < header->size) {                                     \
        value = d_array[(index)];                                              \
    }                                                                          \
    value;                                                                     \
})

#define c_dynarray_clear(d_array) ({                                           \
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(d_array); \
    Expect(header != null, "Invalid d_array header...\n");                     \
    memset(d_array, 0, header->capacity * sizeof(*d_array));                   \
    header->size = 0;                                                          \
})

#define c_dynarray_copy(A, B) ({ \
    StaticAssert(TypesCompatible(*(A), *(B)), "arrays are not of equal type");       \
    Expect(A, "First argument to c_dynarray_copy is invalid...\n")                   \
    dynarray_header_t *header = (dynarray_header_t*)_dynarray_header(A);             \
    if(!B) {                                                                         \
        B =  c_dynarray_create(TypeOf(*A));                                          \
        B = *c_dynarray_reserve(B, header->capacity);                                \
    }                                                                                \
    Expect(B, "Second argument is still invalid...\n");                              \
    Expect(header != null, "Invalid d_array header...\n");                           \
    byte *A_data = (byte*)A - sizeof(dynarray_header_t);                             \
    byte *B_data = (byte*)B - sizeof(dynarray_header_t);                             \
    memcpy(B_data, A_data, (header->size * sizeof(*A)) + sizeof(dynarray_header_t)); \
})

#define c_dynarray_for(d_array, iterator)                                   \
dynarray_header_t *header = (dynarray_header_t *)_dynarray_header(d_array); \
for(u32 iterator = 0; iterator < header->size; ++iterator)

#endif // C_DYNARRAY_H
