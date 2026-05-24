/* ========================================================================
   $File: metaprogram_test_complex_macro.cpp $
   $Date: May 21 2026 11:16 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define COMPLEX_MACRO_MULTILINE(argument0, argument1, argument2) \
argument0##argument1##argument2

#define COMPLEX_MACRO(argument) argument

#define Alloc(type) ({                    \
    void *_result = malloc(sizeof(type)); \
    ZeroMemory(_result, sizeof(type));    \
                                          \
    (type*)_result;                       \
})

#define AllocArray(type, count) ({                \
    void *_result = malloc(sizeof(type) * count); \
    ZeroMemory(_result, sizeof(type) * count);    \
                                                  \
    (type*)_result;                               \
})

#define AllocSize(size) ({        \
    void *_result = malloc(size); \
    ZeroMemory(_result, size);    \
                                  \
    _result;                      \
})
