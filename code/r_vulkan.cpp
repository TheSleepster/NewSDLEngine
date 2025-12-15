/* ========================================================================
   $File: r_vulkan.cpp $
   $Date: December 14 2025 04:50 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_types.h>
#include <c_base.h>
#include <c_math.h>
#include <c_string.h>
#include <c_dynarray.h>
#include <c_threadpool.h>
#include <c_log.h>
#include <c_globals.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <stdlib.h>

#define VkAssert(result) Statement(Assert(result == VK_SUCCESS))

struct render_context_t
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_callback;
};

VKAPI_ATTR VkBool32 VKAPI_CALL
Vk_debug_log_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                      VkDebugUtilsMessageTypeFlagsEXT             message_type,
                      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                      void*                                       user_data)
{
    switch(message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            log_fatal(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            log_warning(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            log_info(callback_data->pMessage);
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            log_trace(callback_data->pMessage);
        }break;
    }
    return VK_FALSE;
}

void
r_renderer_init(render_context_t *render_context)
{
	VkApplicationInfo app_info = {};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName   = null;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName        = null;
	app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion         = VK_API_VERSION_1_3;

    u32 platform_extension_count = 0;
    const char * const *SDL_extensions = SDL_Vulkan_GetInstanceExtensions(&platform_extension_count);
    if(SDL_extensions == null)
    {
        log_error("We have failed to get the SDL_Vulkan instance extensions... Error: '%s'\n", SDL_GetError());
        SDL_Quit();
    }
    platform_extension_count += 1;
    DynArray(char*) extensions = c_dynarray_create(char*);
    extensions = c_dynarray_reserve(extensions, platform_extension_count);

    // NOTE(Sleepster): DEBUG LAYERS 
    extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    memcpy((byte*)extensions + sizeof(char *), SDL_extensions, (platform_extension_count - 1) * sizeof(char *));

    const char *layer = "VK_LAYER_KHRONOS_validation\0";
    DynArray(char*) validation_layers = c_dynarray_create(char*);
    c_dynarray_push(validation_layers, layer);

    u32 total_validation_layers = 0;
    VkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, 0));

    DynArray(VkLayerProperties) found_validation_layers = c_dynarray_create(VkLayerProperties);
    found_validation_layers = c_dynarray_reserve(found_validation_layers, total_validation_layers);

    VkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, found_validation_layers));
    c_dynarray_for(validation_layers, layer_index)
    {
        const char *layer_to_find = c_dynarray_get_at_index(validation_layers, layer_index);
        log_info("Searching for Vulkan validation layer: '%s'\n", layer_to_find);

        bool8 found = false;
        for(u32 property_index = 0;
            property_index < total_validation_layers;
            ++property_index)
        {
            char *layer_name = (found_validation_layers + property_index)->layerName;
            if(strcmp(layer_to_find, layer_name) == 0)
            {
                found = true;
                log_info("Layer found...\n");

                break;
            }
        }

        if(!found)
        {
            log_error("Failure to find Vulkan validation layer: '%s'\n", layer_to_find);
        }
    }

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount   = platform_extension_count;
    instance_info.ppEnabledLayerNames     = validation_layers;
    instance_info.enabledLayerCount       = 1;

    VkAssert(vkCreateInstance(&instance_info, null, &render_context->instance));
    log_info("Vulkan Instance Created..\n");

    // NOTE(Sleepster): DEBUG LAYERS 
    {
        u32 debug_log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

        u32 debug_message_types = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT vulkan_debug_info = {};
        vulkan_debug_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        vulkan_debug_info.messageSeverity = debug_log_severity;
        vulkan_debug_info.messageType     = debug_message_types;
        vulkan_debug_info.pfnUserCallback = Vk_debug_log_callback;

        PFN_vkCreateDebugUtilsMessengerEXT vk_debug_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(render_context->instance, 
                                                                                                                     "vkCreateDebugUtilsMessengerEXT");

        VkAssert(vk_debug_func(render_context->instance, &vulkan_debug_info, null, &render_context->debug_callback));
    }
}
