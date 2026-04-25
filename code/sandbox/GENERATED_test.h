#if !defined(GENERATED_TEST_H)
/* ========================================================================
   $File: GENERATED_test.h $
   $Date: January 25 2026 10:44 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define GENERATED_TEST_H
#include <c_types.h>
#include <c_base.h>

CODE_GEN_IGNORE_FILE()

#define SIZE_MACRO_TEST (20)
#define MULTILINE()          \
struct {                     \
    u32 macro_element;       \
    u32 other_marco_element; \
    u32 thingy_mabob;        \
}

CODE_GEN_IGNORE_STRUCTURE()
struct we_should_not_find_this 
{
    u32 not_found0;
    u32 not_found1;
    u32 not_found2;
    u32 not_found3;
};

struct should_crash_this;

struct c_plus_plus_struct_test_t
{
    u32 thingy_inside_cpp_struct0;
    u32 thingy_inside_cpp_struct1;
    u32 thingy_inside_cpp_struct2;
    
    // TODO(Sleepster): The problem here is that we find this open brace, 
    // and marks it as an anonymous struct, this is bad.
    //
    // we essentially need to make a copy of the token stream from 
    // token.data -> index which we get from c_string_find_first_char_from_left()
    // looking for a '}'. Then perform c_tokenizer_get_next_token() on the copy until we 
    // find that curling brace and check if it's an identifier or a ;. If it's an Identifier,
    // it's not actually anonymous and should return false. If it's a ;, it is actually anonymous.
    struct {
        u32 test_anon_ending_element0;
        u32 test_anon_ending_element1;
        u32 test_anon_ending_element2;
        u32 test_anon_ending_element3;
    }test_anon_ending_name;
};

typedef struct test_thing
{
    const u32             element0[64];
    DynArray_t(string_t)  element1;
    HashTable_t(s32)      element2;
    const volatile u32    element3;

    // NOTE(Sleepster): We need to make it so that test_structure_t is a member of 
    // test_thing_t
    struct test_structure {
        u32 test_element;
        u32 other_test_element;

        // NOTE(Sleepster): Same thing here, make it so that other_thing_t is a mmember
        // of test_structure_t
        struct other_thing {
            s32 element_inside_other_struct0;
            s32 element_inside_other_struct1;
        }other_thing;
    }test_structure;
}test_thing_t;

#endif // GENERATED_TEST_H

