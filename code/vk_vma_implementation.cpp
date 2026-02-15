/* ========================================================================
   $File: vk_vma_implementation.cpp $
   $Date: February 13 2026 02:21 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define VMA_DEBUG_MARGIN 16

// NOTE(Sleepster): Margin validation (corruption detection) works only for memory types that are HOST_VISIBLE and HOST_COHERENT.

#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DEBUG_DETECT_CORRUPTION 1

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
