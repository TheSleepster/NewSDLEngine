/* ========================================================================
   $File: slang_reflector.cpp $
   $Date: April 21 2026 01:56 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <c_types.h>
#include <c_base.h>

#define DYNARRAY_IMPLEMENTATION 
#define HASH_TABLE_IMPLEMENTATION
#include <c_hash_table.h>
#include <c_dynarray.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_global_context.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <c_tokenizer.cpp>

#define PRIMITIVE_TYPE_LIST(X)           \
    X(SLANG_TYPE_FLOAT,     "float")     \
    X(SLANG_TYPE_FLOAT2,    "float2")    \
    X(SLANG_TYPE_FLOAT3,    "float3")    \
    X(SLANG_TYPE_FLOAT4,    "float4")    \
    X(SLANG_TYPE_FLOAT2x2,  "float2x2")  \
    X(SLANG_TYPE_FLOAT3x3,  "float3x3")  \
    X(SLANG_TYPE_FLOAT4x4,  "float4x4")  \
    X(SLANG_TYPE_INT,       "int")       \
    X(SLANG_TYPE_INT2,      "int2")      \
    X(SLANG_TYPE_INT3,      "int3")      \
    X(SLANG_TYPE_INT4,      "int4")      \
    X(SLANG_TYPE_INT2x2,    "int2x2")    \
    X(SLANG_TYPE_INT3x3,    "int3x3")    \
    X(SLANG_TYPE_INT4x4,    "int4x4")    \
    X(SLANG_TYPE_TEXTURE2D, "Texture2D") \
    X(SLANG_TYPE_SAMPLER2D, "Sampler2D")

enum slang_primitive_types_t
{
#define X(enum, string) enum,
    PRIMITIVE_TYPE_LIST(X)
#undef X
};

struct slang_reflector_module_t
{
    tokenizer_t tokenizer; 
};

int
main(void)
{
    string_t file_data = c_file_read_entirety(STR("shaders/basic_triangle.slang"));
    slang_reflector_module_t module = {
        .tokenizer = tokenizer_t{file_data}
    };

    tokenizer_t *tokenizer = &module.tokenizer;
    while(tokenizer->data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(tokenizer);
        switch(token.type)
        {
            case TT_Identifier:
            {
                if(c_string_compare(token.string, STR("struct")))
                {
                    // NOTE(Sleepster): Right now, this MUST be a user declared type                     
                    token_data_t next_token = c_tokenizer_peek_token(tokenizer, 1);
                    if(next_token.type == TT_Identifier)
                    {
                        Expect(false, "You cannot currently declare a 'struct' like this... Expected Identifier, instead found: '%.*s'...\n", 
                               next_token.string.count, next_token.string.data);
                    }
                    else
                    {
                        // NOTE(Sleepster): This is invalid 
                        Expect(false, "You cannot currently declare a 'struct' like this... Expected Identifier, instead found: '%.*s'...\n", 
                               next_token.string.count, next_token.string.data);
                    }
                }
            }break;
            case TT_OpeningBrace:
            {
                // NOTE(Sleepster): Start of constant definition 
            }break;
            case TT_ClosingBrace:
            {
                // NOTE(Sleepster): End of constant defintion 
            }break;
            case TT_OpenBracket:
            {
                // NOTE(Sleepster): Some weird slang thing 
            }break;
            case TT_ClosingBracket:
            {
                // NOTE(Sleepster): End of Some weird slang thing
            }break;
            case TT_OpeningParen:
            {
                // NOTE(Sleepster): Start Important item 
            }break;
            case TT_ClosingParen:
            {    
                // NOTE(Sleepster): End of Important Item 
            }break;
            case TT_Colon:
            {
                // NOTE(Sleepster): Some stupid register thing we don't care about 
            }break;
            case TT_HashTag:
            {
                // NOTE(Sleepster): Some preprocessor item. 
            }break;
        }
    }

    return(0);
}
