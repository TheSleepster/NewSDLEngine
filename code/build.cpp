/* ========================================================================
   $File: build.cpp $
   $Date: August 12 2026 02:24 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define MATH_IMPLEMENTATION
#define HASH_TABLE_IMPLEMENTATION
#define DYNARRAY_IMPLEMENTATION
#define ATHENA_IMPLMENTATION

#include <c_base.h>
#include <c_types.h>
#include <c_global_context.h>
#include <c_intrinsics.h>
#include <p_platform_data.h>
#include <c_log.h>
#include <c_math.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_hash_table.h>
#include <c_dynarray.h>
#include <c_string.h>
#include <c_program_flag_handler.h>
#include <c_synchronization.h>
#include <c_threadpool.h>
#include <c_duration_counter.h>
#include <c_tokenizer.h>
#include <s_RHI_core.h>
#include <s_RHI_image.h>
#include <r_immediate_rendering.h>
#include <s_asset_manager.h>
#include <s_input_manager.h>
#include <s_nt_networking.h>
#include <s_ui_core.h>
#include <s_entity.h>

//#include <meta/GENERATED_program_RTTI.h>
#include <code_generator/athena//athena.h>
#include <meta/ATHENA_GENERATED_RTTI.h>

// NOTE(Sleepster): Game 
#if defined(GAME_DLL_BUILD) || defined(RELEASE)
#include <s_ui_core.cpp>
#include <s_entity.cpp>
#include <main.cpp>
#endif

#if defined(ENGINE_BUILD)
// NOTE(Sleepster): Engine 
#include <c_global_context.cpp>
#include <c_memory_arena.cpp>
#include <c_string.cpp>
#include <c_tokenizer.cpp>
#include <c_zone_allocator.cpp>
#include <c_duration_counter.cpp>
#include <DEBUG_profiling.cpp>
#include <r_immediate_rendering.cpp>
#include <s_input_manager.cpp>
#include <s_RHI_core.cpp>

#include <p_platform_data.h>
#include <vk_backend_allocator.h>
#include <vk_backend_buffer.h>
#include <vk_backend_core.h>
#include <vk_backend_image.h>
#include <vk_backend_shader.h>

#include <c_threadpool.cpp>
#include <c_file_api.cpp>
#include <c_file_watcher.cpp>
#include <p_platform_data.cpp>
#include <s_nt_networking.cpp>
#include <s_RHI_image.cpp>
#include <vk_backend_allocator.cpp>
#include <vk_backend_buffer.cpp>
#include <vk_backend_core.cpp>
#include <vk_backend_image.cpp>
#include <vk_backend_shader.cpp>
#include <s_asset_manager.cpp>

#include <p_platform_data.cpp>
#include <entry.cpp>
#endif
