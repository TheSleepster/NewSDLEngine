/* ========================================================================
   $File: metaprogram_test_complex_macro.cpp $
   $Date: May 21 2026 11:16 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define REPLACEMENT_NUMBER 10
#define REPLACEMENT_NUMBER_WITH_PAREN (10)

#define COMPLEX_MACRO(argument0, argument1, argument2) argument0##argument1##argument2
#define SIMPLE_MACRO2(argument) argument

// NOTE(Sleepster): A macro such as this just simply will not be turned into an AST. It's pointless.
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
