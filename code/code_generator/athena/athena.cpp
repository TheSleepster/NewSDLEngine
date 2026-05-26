/* ========================================================================
   $File: athena.cpp $
   $Date: May 26 2026 10:56 am $
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

int
main(void)
{
    printf("Hello, World!\n");
}
