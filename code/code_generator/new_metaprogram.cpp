/* ========================================================================
   $File: new_metaprogram.cpp $
   $Date: May 21 2026 07:46 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_synchronization.h>

#define HASH_TABLE_IMPLEMENTATION
#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION 

#include <c_hash_table.h>
#include <c_file_api.h>
#include <c_string.h>
#include <c_tokenizer.h>
#include <c_program_flag_handler.h>
#include <c_dynarray.h>

#include <p_platform_data.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_global_context.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_tokenizer.cpp>
// NOTE(Sleepster): 
//
// By default there is a nasty race condition where we need to SEE an item's declaration first before we can know anything about it...
// this is a drawback of single pass compilers. However, this is where the "multipass" idea comes in. We will:
//
// - Load all files into memory
// FIRST PASS
// - Scan them and harvest their type data, macro declarations, and function declarations
// SECOND PASS
// - Compute the RTTI from the previous pass...

struct state_t
{
    // NOTE(Sleepster): Maps macro declarations to their values... 
    ticket_mutex_t        macro_table_mutex;
    HashTable_t(string_t) macro_table;

    // NOTE(Sleepster): Maps type names to their actual type (accounts for typedef)... 
    ticket_mutex_t        type_table_mutex;
    HashTable_t(string_t) type_table;

    // NOTE(Sleepster): Map types to their definitions
    ticket_mutex_t        type_definition_table_mutex;
    HashTable_t(string_t) type_definition_table;

    // NOTE(Sleepster): Maps function names to 
    ticket_mutex_t        function_table_mutex;
    HashTable_t(string_t) function_table;
    
    // NOTE(Sleepster): Stores what files are already loaded... 
    ticket_mutex_t        loaded_file_table_mutex;
    HashTable_t(string)   loaded_file_table;
};

struct macro_data_t
{
    // NOTE(Sleepster): Whatever is #define BLAH 
    string_t  macro_name;
    // NOTE(Sleepster): Whatever is after the #define BLAH ... 
    string_t  macro_string;

    string_t *arguments;
    s32       argument_count;
    bool32    uses_va_args;
};

global_variable state_t *g_state;

// TODO(Sleepster): We will need to make these thread local since these are not threadsafe... 
global_variable memory_arena_t permanent_arena;
global_variable memory_arena_t transient_arena;

int
main(void)
{
    // TODO(Sleepster): Maybe replace malloc here... For now it's fine... 
    g_state = Alloc(state_t);
    ZeroStruct(*g_state);

    c_hash_table_init(&g_state->macro_table,           2048);
    c_hash_table_init(&g_state->type_table,            2048);
    c_hash_table_init(&g_state->type_definition_table, 2048);
    c_hash_table_init(&g_state->function_table,        2048);
    c_hash_table_init(&g_state->loaded_file_table,     2048);

    permanent_arena = c_arena_create(MB(200));
    transient_arena = c_arena_create(MB(50));

    string_t filename = STR("sandbox/GENERATED_test.h");
    string_t file_data = c_file_read_entirety(filename);

    tokenizer_t tokenizer = {file_data};
    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                token_data_t macro_type = c_tokenizer_get_next_token(&tokenizer);
                if(c_string_compare(macro_type.string, STR("define")))
                {
                    token_data_t macro_name = c_tokenizer_get_next_token(&tokenizer);
                    // NOTE(Sleepster): I cannot wait to get a custom string formatter... 
                    Expect(macro_name.type == TT_Identifier, "Invalid token on line: '%d' of file: '%.*s':\nToken: '%.*s' is invalid at this location, it must be an identifier.\n",
                           tokenizer.line_count, filename.count, C_STR(filename), token.string.count, C_STR(token.string));

                    macro_data_t macro = {};
                    macro.macro_name = c_string_make_copy(&permanent_arena, macro_name.string);

                    token = c_tokenizer_get_next_token(&tokenizer);
                    if(token.type == TT_OpeningParen)
                    {
                        // NOTE(Sleepster): Peek tokens ahead 
                        c_tokenizer_set_bookmark(&tokenizer);
                        while(token.type != TT_ClosingParen)
                        {
                            // TODO(Sleepster): We need to figure out what to do with __VA_ARGS__ (...)
                            token = c_tokenizer_get_next_token(&tokenizer);
                            if(token.type == TT_Identifier)
                            {
                                ++macro.argument_count;
                            }
                        }
                        c_tokenizer_restore_bookmark(&tokenizer);

                        // NOTE(Sleepster): allocate string_t * argument_count 
                        macro.arguments = c_arena_push_array(&permanent_arena, string_t, macro.argument_count);

                        // NOTE(Sleepster): Copy each of the argument strings
                        u32 argument_index = 0;
                        while(token.type != TT_ClosingParen)
                        {
                            token = c_tokenizer_get_next_token(&tokenizer);
                            if(token.type == TT_Identifier)
                            {
                                macro.arguments[argument_index++] = c_string_make_copy(&permanent_arena, token.string);
                            }
                        }

                        string_builder_t temp_builder;
                        c_string_builder_init(&temp_builder, MB(10));
                        defer(c_string_builder_deinit(&temp_builder));

                        Expect(token.type == TT_ClosingParen, "We have failed to parse the macro on line '%d' of file '%.*s' 'up a closing parenthesis... Current token is: '%.*s' which is not valid...\n",
                               tokenizer.line_count, filename.count, C_STR(filename), token.string.count, C_STR(token.string))
                    }
                    else
                    {
                    }
                }
                else if(c_string_compare(macro_type.string, STR("include")))
                {
                }
                else if(c_string_compare(macro_type.string, STR("if")))
                {
                }
            }break;
        }
    }
    printf("File is '%d' lines...\n", tokenizer.line_count);
    c_arena_reset(&transient_arena);

    return(0);
}
