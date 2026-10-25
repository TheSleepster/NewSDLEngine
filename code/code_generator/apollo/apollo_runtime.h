#if !defined(APOLLO_RUNTIME_H)
/* ========================================================================
   $File: apollo_runtime.h $
   $Date: October 24 2026 $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define APOLLO_RUNTIME_H
#include <string.h>
#define APOLLO_TAG(tag_type) [[clang::annotate((tag_type))]]

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
    for(const type_info_t *const *type = APOLLO_type_table;
        *type;
        ++type)
    {
        if(strcmp((*type)->name, name) == 0)
        {
            return(*type);
        }
    }

    return(nullptr);
}

inline const type_info_member_t *
member(unsigned long index)
{
    return((const type_info_member_t *)APOLLO_member_table[index]);
}

inline const type_info_struct_t *
as_structure(const type_info_t *info)
{
    return((const type_info_struct_t *)info);
}

inline const type_info_struct_t *
as_structure(const type_info_member_t *member)
{
    return((const type_info_struct_t *)member->type_info);
}

inline const type_info_member_t *
get_member(const type_info_struct_t *structure, unsigned long index)
{
    return((const type_info_member_t *)structure->member_pointer[index]);
}

inline const type_info_member_t *
get_member(const type_info_struct_t *structure, const char *name)
{
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
