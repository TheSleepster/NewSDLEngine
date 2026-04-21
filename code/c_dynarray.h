#if !defined(C_DYNARRAY_H)
/* ========================================================================
   $File: c_dynarray.h $
   $Date: November 30 2025 08:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_DYNARRAY_H
#include <c_base.h>
#include <c_types.h>

#include <p_platform_data.h>

#ifdef DYNARRAY_IMPLEMENTATION
# define DYNARRAY_API
#else 
# define DYNARRAY_API extern
#endif

#define DYNARRAY_HEADER_DEBUG_ID (0xC0FFEE)

// TODO(Sleepster): 
// - [ ] Replace this with an STB style header file
// - [ ] Allow the user to specify custom allocate/free/resize functions
// - [ ] Allow the user to assign an "on_resize" callback

typedef struct dynarray_header 
{
    u32   flags;
    u32   header_id;
    u32   capacity;
    u32   element_size;
    u32   indices_used;
    u32   total_allocated_bytes;
    u32   __padding1;
    u32   __padding2;
}dynarray_header_t;

StaticAssert(sizeof(dynarray_header_t) % 16 == 0, "Dynamic Array header must be 16 byte aligned");

#define DYNARRAY_INITIAL_SIZE  (4)
#define DYNARRAY_GROWTH_FACTOR (2)

DYNARRAY_API void* _dynarray_create_impl(u32 element_size);
DYNARRAY_API void  _dynarray_destroy_impl(void **array);
DYNARRAY_API void  _dynarray_grow_impl(void **array, u32 element_size, u32 new_capacity);
DYNARRAY_API void  _dynarray_insert_impl(void **array, void *element, u32 element_size, u32 index);
DYNARRAY_API void  _dynarray_remove_impl(void **array, u32 element_size, u32 index);

#define DynArray_t(type) TypeOf((type*)null)

#define c_dynarray_header(d_array_ptr) \
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

#define c_dynarray_reserve(d_array, to_reserve) ({                                \
    TypeOf(d_array)   *p_array = &(d_array);                                      \
    dynarray_header_t *header  = (dynarray_header_t *)c_dynarray_header(d_array); \
    Expect(header, "D_array header is invalid for macro reserve()...\n");         \
    if(header->capacity < to_reserve) {                                           \
        u32 new_capacity = Max(header->capacity * 2, to_reserve);                 \
        _dynarray_grow_impl((void**)p_array, sizeof(*d_array), new_capacity);     \
        header   = c_dynarray_header(*p_array);                                   \
    }                                                                             \
                                                                                  \
    *p_array;                                                                     \
})

#define c_dynarray_add_element(d_array, element, index) ({                                                 \
    TypeOf(d_array) *p_first = &(d_array);                                                                 \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array);                            \
    if(header == null) {                                                                                   \
        *p_first = (TypeOf(d_array))_dynarray_create_impl(sizeof(*d_array));                               \
        header = (dynarray_header_t*)c_dynarray_header(*p_first);                                          \
    }                                                                                                      \
                                                                                                           \
    if(header->indices_used + 1 >= header->capacity) {                                                     \
        _dynarray_grow_impl((void**)p_first, sizeof(*d_array), header->capacity * DYNARRAY_GROWTH_FACTOR); \
        header = (dynarray_header_t*)c_dynarray_header(*p_first);                                          \
    }                                                                                                      \
    Expect(header, "Header for macro add_element is invalid...\n");                                        \
    Expect(index < header->capacity, "index is >= capacity in add\n");                                     \
    _dynarray_insert_impl((void**)p_first, (void*)&(element), sizeof(*d_array), index);                    \
    header->indices_used += 1;                                                                             \
                                                                                                           \
    p_first;                                                                                               \
})

#define c_dynarray_remove_element(d_array, index) ({                    \
    TypeOf(d_array) *p_first = &(d_array);                              \
    TypeOf(*(d_array)) value = c_dynarray_get_value(d_array, index);    \
    _dynarray_remove_impl((void**)p_first, sizeof(d_array[0]), index);  \
    value;                                                              \
})

#define c_dynarray_push(d_array, value) ({                                         \
    TypeOf(d_array) *p_first = &(d_array);                                         \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header((d_array));  \
    if(!header) {                                                                  \
        *p_first = (TypeOf(d_array))_dynarray_create_impl(sizeof(*d_array));       \
        header = (dynarray_header_t*)c_dynarray_header((d_array));                 \
    }                                                                              \
    u32 index = header->indices_used;                                              \
    p_first = c_dynarray_add_element(d_array, value, index);                       \
    p_first;                                                                       \
})

#define c_dynarray_pop(d_array) ({                                                                 \
        dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array);                \
        Expect(header, "DArray header is invalid...\n");                                           \
        TypeOf(*(d_array)) r_value = c_dynarray_remove_element(d_array, header->indices_used - 1); \
                                                                                                   \
        r_value;                                                                                   \
})

#define c_dynarray_get_value(d_array, index) ({                                 \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array); \
    TypeOf(*(d_array)) value = {};                                              \
    Expect(header, "Invalid d_array header...\n");                              \
    if(header && (index) < header->indices_used) {                              \
        value = d_array[(index)];                                               \
    }                                                                           \
    value;                                                                      \
})

#define c_dynarray_get_ptr(d_array, index) ({                                   \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array); \
    TypeOf(*(d_array)) *value = {};                                             \
    Expect(header, "Invalid d_array header...\n");                              \
    if(header && (index) < header->indices_used) {                              \
        value = d_array + (index);                                              \
    }                                                                           \
    value;                                                                      \
})

#define c_dynarray_count(d_array) ({ \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array); \
    Expect(header, "Invalid d_array header...\n");                              \
    u32 count = header->capacity;                                               \
                                                                                \
    count;                                                                      \
})

#define c_dynarray_clear(d_array) ({                                            \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(d_array); \
    Expect(header != null, "Invalid d_array header...\n");                      \
    ZeroMemory(d_array, header->capacity * sizeof(*d_array));                   \
    header->indices_used = 0;                                                   \
})

#define c_dynarray_copy(A, B) ({ \
    StaticAssert(TypesSame(*(A), *(B)), "arrays are not of equal type");                      \
    Expect(A, "First argument to c_dynarray_copy is invalid...\n")                            \
    dynarray_header_t *header = (dynarray_header_t*)c_dynarray_header(A);                     \
    if(!(B)) {                                                                                \
        (B) = c_dynarray_create(TypeOf(*A));                                                  \
        (B) = c_dynarray_reserve(B, header->capacity);                                        \
    }                                                                                         \
    Expect(B, "Second argument is still invalid...\n");                                       \
    Expect(header != null, "Invalid d_array header...\n");                                    \
    byte *A_data = (byte*)A - sizeof(dynarray_header_t);                                      \
    byte *B_data = (byte*)B - sizeof(dynarray_header_t);                                      \
    memcpy(B_data, A_data, (header->indices_used * sizeof(*A)) + sizeof(dynarray_header_t));  \
})

#define c_dynarray_for_impl(d_array, iterator_name, counter)                                          \
    dynarray_header_t *Glue(header, counter) = (dynarray_header_t *)c_dynarray_header(d_array);       \
    Expect(Glue(header, counter), "Header is invalid, cannot loop...\n");                             \
    for(u32 iterator_name = 0; iterator_name < Glue(header, counter)->indices_used; ++iterator_name)

#define c_dynarray_for(d_array, iterator_name) \
    c_dynarray_for_impl(d_array, iterator_name, __COUNTER__)

#ifdef DYNARRAY_IMPLEMENTATION

DYNARRAY_API void*
_dynarray_create_impl(u32 element_size)
{
    void *result = null;

    u32 allocation_size = Align16((element_size * DYNARRAY_INITIAL_SIZE) + (sizeof(dynarray_header_t)));
    result = malloc(allocation_size);
    ZeroMemory(result, allocation_size);

    dynarray_header_t *header = (dynarray_header_t*)result;
    result = (byte*)result + sizeof(dynarray_header_t);

    header->total_allocated_bytes = allocation_size;

    header->header_id    = DYNARRAY_HEADER_DEBUG_ID;
    header->capacity     = DYNARRAY_INITIAL_SIZE;
    header->element_size = element_size;

    return(result);
}

DYNARRAY_API void
_dynarray_grow_impl(void **array, u32 element_size, u32 new_capacity)
{
    void *base   = ((byte*)*array - sizeof(dynarray_header_t));
    void *result = base;

    Expect(array != null, "Array is invalid...\n");

    dynarray_header_t *header = c_dynarray_header(*array); 
    u64 old_size = header->capacity * element_size;
    u64 new_size = new_capacity     * element_size;

    result = realloc(base, (element_size * new_capacity) + sizeof(dynarray_header_t));
    ZeroMemory((byte*)result + sizeof(dynarray_header_t) + old_size, new_size - old_size);
    
    result = (byte*)result + sizeof(dynarray_header_t);
    *array = result;

    header = c_dynarray_header(result); 
    header->capacity = new_capacity;
    header->total_allocated_bytes = new_size;

    Expect(header->header_id == DYNARRAY_HEADER_DEBUG_ID, "Header ID is invalid...\n");
    Expect(new_capacity > DYNARRAY_INITIAL_SIZE, "new capacity is <= Initial\n");
}

DYNARRAY_API void
_dynarray_destroy_impl(void **array)
{
    Expect(array != null, "Array is invalid...\n");

    dynarray_header_t *header = c_dynarray_header(*array); 
    Expect(header->header_id == DYNARRAY_HEADER_DEBUG_ID, "Header ID is invalid...\n");

    void *array_data = (byte *)*array - sizeof(dynarray_header_t);
    free(array_data);

    *array = null;
}

DYNARRAY_API void
_dynarray_insert_impl(void **array, void *element, u32 element_size, u32 index)
{
    Expect(array != null, "Array is invalid...\n");

    dynarray_header_t *header = c_dynarray_header(*array); 
    Expect(header->header_id == DYNARRAY_HEADER_DEBUG_ID, "Header ID is invalid...\n");

    byte *index_data = (byte*)*array + (element_size * index);
    memcpy(index_data, element, element_size);
}

DYNARRAY_API void
_dynarray_remove_impl(void **array, u32 element_size, u32 index)
{
    Expect(array != null, "Array header is invalid...\n");

    dynarray_header_t *header = c_dynarray_header(*array); 
    Expect(header->header_id == DYNARRAY_HEADER_DEBUG_ID, "Header ID is invalid...\n");
    Expect(index <= header->indices_used, "Index is > to that of the header->size");

    byte *array_data = (byte*)*array;
    if(index < header->indices_used - 1) 
    {
        byte *to   = array_data + (element_size * index);
        byte *from = array_data + (element_size * (index + 1));

        usize bytes_to_write = (header->indices_used - index - 1) * element_size;
        memmove(to, from, bytes_to_write);
    }
    header->indices_used -= 1;
}

#endif // DYNARRAY_IMPLEMENTATION
#endif // C_DYNARRAY_H
