#if !defined(APOLLO_RUNTIME_H)
/* ========================================================================
   $File: apollo_runtime.h $
   $Date: October 24 2026 $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define APOLLO_RUNTIME_H
#include <string.h>
#include <stdio.h>

#define APOLLO_TAG(tag_type) [[clang::annotate((tag_type))]]

#define APOLLO_ASSERT(cond) \
do { \
    if(!(cond)) \
    { \
        fprintf(stderr, "[%s], [%d]: Apollo Assert Condition failed...", \
                __FILE__, __LINE__); \
        (*(volatile char *)0 = 0); \
    } \
}while(0)

namespace Apollo {

// TODO(Sleepster):  
// The current functions like type_info() are very slow because it's just an O(N) linear search
// which is stupid. In the future we should asign IDs to the generated structures so that binary
// searching can be done which will be MUCH quicker. It doesn't really matter right now with like
// ~10 type infos, but with hundreds it definitely will.

enum metatype_t
{
    APOLLO_METATYPE_PRIMITIVE,
    APOLLO_METATYPE_STRUCTURE,
    APOLLO_METATYPE_ENUM,
    APOLLO_METATYPE_PROCEDURE,
};

struct type_info_t
{
    const char    *name;
    const unsigned long size;
    const unsigned long metatype;
};

// NOTE(Sleepster): Strip the reference and cv-qualifiers from a deduced type.
template <typename T> struct remove_value_reference_t                       { using internal_type = T; };
template <typename T> struct remove_value_reference_t<T&>                   { using internal_type = typename remove_value_reference_t<T>::internal_type; };
template <typename T> struct remove_value_reference_t<T&&>                  { using internal_type = typename remove_value_reference_t<T>::internal_type; };
template <typename T> struct remove_value_reference_t<const T>              { using internal_type = typename remove_value_reference_t<T>::internal_type; };
template <typename T> struct remove_value_reference_t<volatile T>           { using internal_type = typename remove_value_reference_t<T>::internal_type; };
template <typename T> struct remove_value_reference_t<const volatile T>     { using internal_type = typename remove_value_reference_t<T>::internal_type; };

template <typename T> constexpr const type_info_t *type_info_value = nullptr;

template <typename T>
constexpr const type_info_t *
type_info()
{
    return(type_info_value<typename remove_value_reference_t<T>::internal_type>);
}

// NOTE(Sleepster): Fundamental types have no declaration cursor, so register them here.
#define APOLLO_FUNDAMENTALS(X)                              \
    X(void,               void,               0)            \
    X(bool,               bool,               sizeof(bool)) \
    X(char,               char,               sizeof(char)) \
    X(schar,              signed char,        sizeof(signed char)) \
    X(uchar,              unsigned char,      sizeof(unsigned char)) \
    X(short,              short,              sizeof(short)) \
    X(ushort,             unsigned short,     sizeof(unsigned short)) \
    X(int,                int,                sizeof(int)) \
    X(uint,               unsigned int,       sizeof(unsigned int)) \
    X(long,               long,               sizeof(long)) \
    X(ulong,              unsigned long,      sizeof(unsigned long)) \
    X(longlong,           long long,          sizeof(long long)) \
    X(ulonglong,          unsigned long long, sizeof(unsigned long long)) \
    X(float,              float,              sizeof(float)) \
    X(double,             double,             sizeof(double)) \
    X(longdouble,         long double,        sizeof(long double)) \
    X(wchar,              wchar_t,            sizeof(wchar_t)) \
    X(char16,             char16_t,           sizeof(char16_t)) \
    X(char32,             char32_t,           sizeof(char32_t))

#define APOLLO_FUNDAMENTAL(id, cpp_type, size_value)                                                    \
struct type_info_##id { const type_info_t info; };                                                      \
constexpr type_info_##id DEFAULT_##id = { .info = {#cpp_type, size_value, APOLLO_METATYPE_PRIMITIVE} }; \
template <> constexpr const type_info_t *type_info_value<cpp_type> = &DEFAULT_##id.info;

APOLLO_FUNDAMENTALS(APOLLO_FUNDAMENTAL)

#undef APOLLO_FUNDAMENTAL

constexpr const type_info_t *const APOLLO_primitive_type_table[] = {
#define APOLLO_PRIMITIVE_ENTRY(id, cpp_type, size_value) &DEFAULT_##id.info,
    APOLLO_FUNDAMENTALS(APOLLO_PRIMITIVE_ENTRY)
#undef APOLLO_PRIMITIVE_ENTRY
#undef APOLLO_FUNDAMENTALS
    nullptr,
};

struct type_info_member_t
{
    const type_info_t  info;
    const type_info_t *type_info;
    const unsigned long offset;
    const unsigned long value_type;
    union {
        int                int32_value;
        long long          int64_value;
        unsigned long      uint32_value;
        unsigned long long uint64_value;
        float              float32_value;
        double             float64_value;
        const char        *string_value;
    }value;
};

struct type_info_struct_t
{
    const type_info_t info;
    const unsigned long member_count;
    const type_info_member_t *const *member_pointer;
};

extern const type_info_t *const APOLLO_type_table[];
extern const type_info_t *const APOLLO_member_table[];

inline const type_info_t *
type_info(unsigned long index)
{
    return(APOLLO_type_table[index]);
}

inline const type_info_t *
type_info(const char *name)
{
    const type_info_t *result = nullptr;

    for(const type_info_t *const *type = APOLLO_primitive_type_table;
        *type;
        ++type)
    {
        if(strcmp((*type)->name, name) == 0)
        {
            result = *type;
        }
    }

    for(const type_info_t *const *type = APOLLO_type_table;
        *type;
        ++type)
    {
        if(strcmp((*type)->name, name) == 0)
        {
            result = *type;
        }
    }

    return(result);
}

inline const type_info_member_t *
member(unsigned long index)
{
    return((const type_info_member_t *)APOLLO_member_table[index]);
}

inline const type_info_struct_t *
as_structure(const type_info_t *info)
{
    APOLLO_ASSERT(info);
    APOLLO_ASSERT(info->metatype == APOLLO_METATYPE_STRUCTURE || info->metatype == APOLLO_METATYPE_ENUM);

    return((const type_info_struct_t *)info);
}

inline const type_info_struct_t *
as_structure(const type_info_member_t *member)
{
    APOLLO_ASSERT(member);
    APOLLO_ASSERT(member->info.metatype == APOLLO_METATYPE_STRUCTURE || member->info.metatype == APOLLO_METATYPE_ENUM);

    return((const type_info_struct_t *)member->type_info);
}

inline const type_info_member_t *
get_member(const type_info_struct_t *structure, unsigned long index)
{
    APOLLO_ASSERT(structure);
    APOLLO_ASSERT(structure->info.metatype == APOLLO_METATYPE_STRUCTURE || structure->info.metatype == APOLLO_METATYPE_ENUM);

    return((const type_info_member_t *)structure->member_pointer[index]);
}

inline const type_info_member_t *
get_member(const type_info_struct_t *structure, const char *name)
{
    APOLLO_ASSERT(structure);
    APOLLO_ASSERT(structure->info.metatype == APOLLO_METATYPE_STRUCTURE || structure->info.metatype == APOLLO_METATYPE_ENUM);

    for(unsigned long index = 0;
        index < structure->member_count;
        ++index)
    {
        const type_info_member_t *member = get_member(structure, index);
        if(strcmp(member->info.name, name) == 0)
        {
            return(member);
        }
    }

    return(nullptr);
}

inline const type_info_member_t *
get_member(const type_info_t *info, unsigned long index)
{
    return(get_member(as_structure(info), index));
}

inline const type_info_member_t *
get_member(const type_info_t *info, const char *name)
{
    return(get_member(as_structure(info), name));
}

template<typename T>
inline const type_info_member_t *
get_member(const type_info_struct_t *structure, T index)
{
    return(get_member(structure, (unsigned long)index));
}

template<typename T>
inline const type_info_member_t *
get_member(const type_info_t *info, T index)
{
    return(get_member(info, (unsigned long)index));
}

#if defined(C_STRING_H)
inline const type_info_t *
type_info(string_t name)
{
    for(const type_info_t *const *type = APOLLO_primitive_type_table;
        *type;
        ++type)
    {
        if(c_string_compare(STR((*type)->name), name))
        {
            return(*type);
        }
    }

    for(const type_info_t *const *type = APOLLO_type_table;
        *type;
        ++type)
    {
        if(c_string_compare(STR((*type)->name), name))
        {
            return(*type);
        }
    }

    return(nullptr);
}

inline const type_info_member_t *
get_member(const type_info_struct_t *structure, string_t name)
{
    for(unsigned long index = 0;
        index < structure->member_count;
        ++index)
    {
        const type_info_member_t *member = get_member(structure, index);
        if(c_string_compare(STR(member->info.name), name))
        {
            return(member);
        }
    }

    return(nullptr);
}

inline const type_info_member_t *
get_member(const type_info_t *info, string_t name)
{
    return(get_member(as_structure(info), name));
}
#endif

} // namespace Apollo

#endif
