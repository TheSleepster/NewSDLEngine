/* ========================================================================
   $File: apollo_test.cpp $
   $Date: October 12 2026 05:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#define APOLLO_IMPLEMENTATION 1
#include "../code_generator/apollo/apollo.h"

int
main(void)
{
    dynarray_t<string_t> options = {};

    string_t include_path  = STR("-I../code/");
    string_t engine_define = STR("-DENGINE_BUILD");
    string_t game_define   = STR("-DGAME_BUILD");

    c_dynarray_add(&options, &include_path);
    c_dynarray_add(&options, &engine_define);
    c_dynarray_add(&options, &game_define);
    apollo_code_decl_list_t *declarations = Apollo::parse_singular_file(STR("build.cpp"), options);
    if(declarations)
    {
        for(code_decl_t *declaration: declarations->all)
        {
            if(declaration->metatype == CODE_DECL_METATYPE_STRUCTURE)
            {
                printf("STRUCTURE: '%.*s'...\n", fprint_string(declaration->identifier));
            }
            else if(declaration->metatype == CODE_DECL_METATYPE_LAMBDA)
            {
                printf("PROCEDURE: '%.*s'...\n", fprint_string(declaration->identifier));
            }
        }
    }

    return(0);
}
