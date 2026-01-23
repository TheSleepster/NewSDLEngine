/* ========================================================================
   $File: arg_parser.cpp $
   $Date: January 22 2026 01:27 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdlib.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>
#include <c_file_api.h>
#include <c_string.h>

#define PROGRAM_FLAG_HANDLER_IMPLEMENTATION
#include <c_program_flag_handler.h>

#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_string.cpp>
#include <c_dynarray_impl.cpp>
#include <c_globals.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>

int
main(int argc, char **argv)
{
    // NOTE(Sleepster): THESE WILL NEVER ALLOCATE ANY MEMORY
    bool32  *help        = c_program_flag_add_bool32("test_bool", false, "Displays this help message\n");
    float32 *sensitivity = c_program_flag_add_float32("test_float", 1.0f, "Sets the program's sensitivity\n");
    u64     *size        = c_program_flag_add_size("test_u64", 10, "Tells us the size of the file\n");
    char   **data        = c_program_flag_add_string("test_string", null, "data information\n");

    log_info("Argument 'help' is: '%s'...\n", *help == true ? "true" : "false");
    log_info("Argument 'sensitivity' is: '%f'...\n", *sensitivity);
    log_info("Argument 'size' is: '%llu'...\n", *size);
    log_info("Argument 'data' is: '%s'...\n", *data);

    log_info("\n");
    log_info("Parsing...\n");
    log_info("\n");
    c_program_flag_parse_args(argc, argv);

    log_info("Argument 'help' is: '%s'...\n", *help == true ? "true" : "false");
    log_info("Argument 'sensitivity' is: '%f'...\n", *sensitivity);
    log_info("Argument 'size' is: '%llu'...\n", *size);
    log_info("Argument 'data' is: '%s'...\n", *data);
    if(*help)
    {
        c_program_flag_print_flag_list();
    }

    return(0);
}
