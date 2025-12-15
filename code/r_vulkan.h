#if !defined(R_VULKAN_H)
/* ========================================================================
   $File: r_vulkan.h $
   $Date: December 14 2025 05:16 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_VULKAN_H
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

struct render_context_t
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_callback;
};

#endif // R_VULKAN_H

