/* ========================================================================
   $File: vk_backend_core.cpp $
   $Date: February 12 2026 04:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>
#include <s_renderer.h>
#include <vk_backend_shader.h>
#include <c_string.h>
#include <c_file_api.h>
#include <s_renderer.h>

#include <s_asset_manager.h>

void vk_backend_create_depth_buffer(vulkan_context_t *vulkan_context);
void vk_backend_create_framebuffers(vulkan_context_t *vulkan_context);
void vk_backend_destroy_framebuffers(vulkan_context_t *vulkan_context);

/*
=============
Vk_backend_debug_log_callback
=============
*/

VKAPI_ATTR VkBool32 VKAPI_CALL
Vk_backend_debug_log_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                              VkDebugUtilsMessageTypeFlagsEXT             message_type,
                              const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                              void*                                       user_data)
{
    switch(message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            log_fatal(callback_data->pMessage);
            printf("\n");
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            log_warning(callback_data->pMessage);
            printf("\n");
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            log_info(callback_data->pMessage);
            printf("\n");
        }break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            log_trace(callback_data->pMessage);
            printf("\n");
        }break;
    }
    return VK_FALSE;
}

/*
=============
vk_backend_result_is_success
=============
*/

bool8 
vk_backend_result_is_success(VkResult result) 
{
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    switch (result) 
    {
        // Success Codes
        default:
        case VK_SUCCESS:
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
        case VK_SUBOPTIMAL_KHR:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return true;

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:

        // NOTE: Same as above
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        case VK_ERROR_UNKNOWN:
            return false; 
    }
}

/*
=============
vk_backend_vulkan_result_string
=============
*/

const char* 
vk_backend_vulkan_result_string(VkResult result, bool8 get_extended) 
{
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Success Codes
    switch (result) {
        default:
        case VK_SUCCESS:
            return !get_extended ? "VK_SUCCESS" : "VK_SUCCESS Command successfully completed";
        case VK_NOT_READY:
            return !get_extended ? "VK_NOT_READY" : "VK_NOT_READY A fence or query has not yet completed";
        case VK_TIMEOUT:
            return !get_extended ? "VK_TIMEOUT" : "VK_TIMEOUT A wait operation has not completed in the specified time";
        case VK_EVENT_SET:
            return !get_extended ? "VK_EVENT_SET" : "VK_EVENT_SET An event is signaled";
        case VK_EVENT_RESET:
            return !get_extended ? "VK_EVENT_RESET" : "VK_EVENT_RESET An event is unsignaled";
        case VK_INCOMPLETE:
            return !get_extended ? "VK_INCOMPLETE" : "VK_INCOMPLETE A return array was too small for the result";
        case VK_SUBOPTIMAL_KHR:
            return !get_extended ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
        case VK_THREAD_IDLE_KHR:
            return !get_extended ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
        case VK_THREAD_DONE_KHR:
            return !get_extended ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
        case VK_OPERATION_DEFERRED_KHR:
            return !get_extended ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return !get_extended ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return !get_extended ? "VK_PIPELINE_COMPILE_REQUIRED_EXT" : "VK_PIPELINE_COMPILE_REQUIRED_EXT A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return !get_extended ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return !get_extended ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return !get_extended ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
        case VK_ERROR_DEVICE_LOST:
            return !get_extended ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return!get_extended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return !get_extended ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return !get_extended ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return !get_extended ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return !get_extended ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return !get_extended ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return !get_extended ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
        case VK_ERROR_FRAGMENTED_POOL:
            return !get_extended ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return !get_extended ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return !get_extended ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return !get_extended ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return !get_extended ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_INVALID_SHADER_NV:
            return !get_extended ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return !get_extended ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return !get_extended ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
        case VK_ERROR_FRAGMENTATION:
            return !get_extended ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
            return !get_extended ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
        // NOTE: Same as above
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        //    return !get_extended ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" :"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return !get_extended ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
        case VK_ERROR_UNKNOWN:
            return !get_extended ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    }
}

/*
=============
vk_backend_allocate_descriptor_sets
=============
*/

void
vk_backend_allocate_descriptor_sets(vulkan_context_t *vulkan_context, material_archetype_t *archetype)
{
    vulkan_shader_t *shader = &archetype->shader_handle.shader->shader_data;

    VkDescriptorSetAllocateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool     = vulkan_context->first_descriptor_pool;
    info.descriptorSetCount = shader->descriptor_set_count;
    info.pSetLayouts        = shader->layouts;

    vkAssert(vkAllocateDescriptorSets(vulkan_context->device, &info, archetype->descriptors));
}

/*
=============
vk_backend_get_scratch_command_buffer
=============
*/

VkCommandBuffer
vk_backend_get_and_begin_scratch_command_buffer(vulkan_context_t *vulkan_context, bool8 is_primary)
{
    VkCommandBuffer result;
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vulkan_context->graphics_command_pool,
        .commandBufferCount = 1,
        .level              = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY
    };
    vkAssert(vkAllocateCommandBuffers(vulkan_context->device, &command_buffer_allocate_info, &result));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(result, &begin_info);

    return(result);
}

/*
=============
vk_backend_release_scratch_command_buffer
=============
*/

void
vk_backend_submit_and_release_scratch_command_buffer(vulkan_context_t *vulkan_context, VkCommandBuffer *command_buffer)
{
    vkEndCommandBuffer(*command_buffer);
    VkSubmitInfo submit_info = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = command_buffer
    };

    vkAssert(vkQueueSubmit(vulkan_context->graphics_queue, 1, &submit_info, 0));
    vkAssert(vkQueueWaitIdle(vulkan_context->graphics_queue));
    vkFreeCommandBuffers(vulkan_context->device, 
                         vulkan_context->graphics_command_pool, 
                         1, 
                         command_buffer);
}

/*
=============
vk_backend_find_memory_index
=============
*/

s32 
vk_backend_find_memory_index(vulkan_context_t *vulkan_context,
                             u32               type_filter, 
                             u32               property_flags)
{
    s32 result = -1;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vulkan_context->gpu.device, &memory_properties);

    for(u32 memory_index = 0;
        memory_index < memory_properties.memoryTypeCount;
        ++memory_index)
    
    {
        if((type_filter & (1 << memory_index)) && 
          ((memory_properties.memoryTypes[memory_index].propertyFlags & property_flags) == property_flags))
        {
            result = memory_index;
            break;
        }
    }

    if(result == -1)
    {
        log_error("Failure to find a valid memory type...\n");
    }

    return(result);
}

/*
=============
vk_backend_check_physical_device_support
=============
*/

internal_api bool8
vk_backend_check_physical_device_support(gpu_info_t *info)
{
    bool8 result = true;

    s32 required_exts  = g_device_extension_count;
    s32 exts_available = 0;
    for(s32 required_index = 0;
        required_index < required_exts;
        ++required_index)
    {
        for(u32 gpu_ext_index = 0;
            gpu_ext_index < c_dynarray_count(info->extension_properties);
            ++gpu_ext_index)
        {
            string_t required_extension = STR(g_device_extensions[required_index]);
            string_t gpu_extension      = STR((info->extension_properties + gpu_ext_index)->extensionName);
            if(c_string_compare(required_extension, gpu_extension))
            {
                ++exts_available;
                break;
            }
        }
    }

    result = (exts_available == required_exts);
    return(result);
}

/*
=============
vk_backend_create_instance
=============
*/

void
vk_backend_create_instance(vulkan_context_t *vulkan_context)
{
    vulkan_context->initialization_arena = c_arena_create(MB(10));
    vulkan_context->swapchain_arena      = c_arena_create(MB(10));
    vulkan_context->permanent_arena      = c_arena_create(MB(10));
    vulkan_context->frame_arena          = c_arena_create(MB(100));

	VkApplicationInfo app_info  = {};
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
    DynArray_t(char*) extensions = c_dynarray_create(char*);
    extensions = c_dynarray_reserve(extensions, platform_extension_count + 2);

    // NOTE(Sleepster): DEBUG LAYERS 
    extensions[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    memcpy((byte*)extensions + (sizeof(char *)), SDL_extensions, (platform_extension_count - 1) * sizeof(char *));

    const char *layer = "VK_LAYER_KHRONOS_validation\0";
    DynArray_t(char*) validation_layers = c_dynarray_create(char*);
    c_dynarray_push(validation_layers, layer);

    u32 total_validation_layers = 0;
    vkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, 0));

    DynArray_t(VkLayerProperties) found_validation_layers = c_dynarray_create(VkLayerProperties);
    found_validation_layers = c_dynarray_reserve(found_validation_layers, total_validation_layers);

    vkAssert(vkEnumerateInstanceLayerProperties(&total_validation_layers, found_validation_layers));
    c_dynarray_for(validation_layers, layer_index)
    {
        const char *layer_to_find = c_dynarray_get_value(validation_layers, layer_index);
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

    VkInstanceCreateInfo instance_info    = {};
    instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.ppEnabledExtensionNames = extensions;
    instance_info.enabledExtensionCount   = platform_extension_count;
    instance_info.ppEnabledLayerNames     = validation_layers;
    instance_info.enabledLayerCount       = 1;
    instance_info.pApplicationInfo        = &app_info;

    vkAssert(vkCreateInstance(&instance_info, vulkan_context->cpu_allocation_callbacks, &vulkan_context->instance));
    log_info("Vulkan Instance Created..\n");

    u32 debug_log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT  |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT   |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    u32 debug_message_types = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT vulkan_debug_info = {};
    vulkan_debug_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vulkan_debug_info.messageSeverity = debug_log_severity;
    vulkan_debug_info.messageType     = debug_message_types;
    vulkan_debug_info.pfnUserCallback = Vk_backend_debug_log_callback;


    PFN_vkCreateDebugUtilsMessengerEXT vk_debug_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_context->instance, "vkCreateDebugUtilsMessengerEXT");

    vkAssert(vk_debug_func(vulkan_context->instance, &vulkan_debug_info, vulkan_context->cpu_allocation_callbacks, &vulkan_context->debug_messenger));

    c_dynarray_destroy(extensions);
    c_dynarray_destroy(validation_layers);
    c_dynarray_destroy(found_validation_layers);
}

/*
=============
vk_backend_create_surface
=============
*/

void
vk_backend_create_surface(vulkan_context_t *vulkan_context, SDL_Window *window)
{
    vulkan_context->window = window;
    if(!SDL_Vulkan_CreateSurface(window, vulkan_context->instance, vulkan_context->cpu_allocation_callbacks, &vulkan_context->render_surface))
    {
        log_error("Failure to create the SDL Vulkan Surface... Error: '%s\n'", SDL_GetError());
    }

    log_info("Vulkan surface created...\n");
}


/*
=============
vk_backend_select_physical_device
=============
*/

void
vk_backend_select_physical_device(vulkan_context_t *vulkan_context)
{
    u32 physical_device_counter = 0;
    vkAssert(vkEnumeratePhysicalDevices(vulkan_context->instance, &physical_device_counter, 0));
    VkPhysicalDevice *devices = c_arena_push_array(&vulkan_context->initialization_arena, VkPhysicalDevice, physical_device_counter);
    vkAssert(vkEnumeratePhysicalDevices(vulkan_context->instance, &physical_device_counter, devices));

    DynArray_t(gpu_info_t) gpus;
    gpus = c_dynarray_create(gpu_info_t);
    gpus = c_dynarray_reserve(gpus, physical_device_counter);

    defer(c_dynarray_destroy(gpus));

    for(u32 device_index = 0;
        device_index < physical_device_counter;
        ++device_index)
    {
        gpu_info_t *gpu_info = gpus + device_index;
        gpu_info->device     = devices[device_index];

        gpu_info->queue_family_properties = c_dynarray_create(VkQueueFamilyProperties);
        gpu_info->extension_properties    = c_dynarray_create(VkExtensionProperties);
        gpu_info->valid_surface_formats   = c_dynarray_create(VkSurfaceFormatKHR);
        gpu_info->valid_present_modes     = c_dynarray_create(VkPresentModeKHR);
        {
            u32 num_queues;
            vkGetPhysicalDeviceQueueFamilyProperties(gpu_info->device, &num_queues, null);
            Expect(num_queues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned a num_queues of 0...\n");

            gpu_info->queue_family_properties = c_dynarray_reserve(gpu_info->queue_family_properties, num_queues);

            vkGetPhysicalDeviceQueueFamilyProperties(gpu_info->device, &num_queues, gpu_info->queue_family_properties);
            Expect(num_queues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned a num_queues of 0...\n");
        }

        {
            u32 num_extensions;
            vkEnumerateDeviceExtensionProperties(gpu_info->device, null, &num_extensions, null);
            Expect(num_extensions > 0, "vkEnumerateDeviceExtensionProperties returned a num_extensions of 0...\n");

            gpu_info->extension_properties = c_dynarray_reserve(gpu_info->extension_properties, num_extensions);

            vkEnumerateDeviceExtensionProperties(gpu_info->device, null, &num_extensions, gpu_info->extension_properties);
            Expect(num_extensions > 0, "vkEnumerateDeviceExtensionProperties returned a num_extensions of 0...\n");
        }

        vkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_info->device, vulkan_context->render_surface, &gpu_info->surface_capabilities));

        {
            u32 num_formats;
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_info->device, vulkan_context->render_surface, &num_formats, 0);
            Expect(num_formats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned a num_formats of 0...\n");

            gpu_info->valid_surface_formats = c_dynarray_reserve(gpu_info->valid_surface_formats, num_formats);

            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu_info->device, vulkan_context->render_surface, &num_formats, gpu_info->valid_surface_formats);
            Expect(num_formats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned a num_formats of 0...\n");
        }

        {
            u32 num_present_modes;
            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_info->device, vulkan_context->render_surface, &num_present_modes, null);
            Expect(num_present_modes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned a num_present_modes of 0...\n");

            gpu_info->valid_present_modes = c_dynarray_reserve(gpu_info->valid_present_modes, num_present_modes);

            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu_info->device, vulkan_context->render_surface, &num_present_modes, gpu_info->valid_present_modes);
            Expect(num_present_modes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned a num_present_modes of 0...\n");
        }

        vkGetPhysicalDeviceMemoryProperties(gpu_info->device, &gpu_info->memory_properties);
        vkGetPhysicalDeviceProperties(gpu_info->device, &gpu_info->properties);
        vkGetPhysicalDeviceFeatures(gpu_info->device, &gpu_info->features);

        gpu_info->queue_family_count = c_dynarray_count(gpu_info->queue_family_properties);
    }

    // NOTE(Sleepster): Select the best fit device 
    for(u32 gpu_index = 0;
        gpu_index < physical_device_counter;
        ++gpu_index)
    {
        gpu_info_t *gpu = gpus + gpu_index;

        s32 graphics_index = -1;
        s32 present_index  = -1;
        s32 compute_index  = -1;
        s32 transfer_index = -1;

        s32 graphics_score = -1;
        s32 present_score  = -1;
        s32 compute_score  = -1;
        s32 transfer_score = -1;

        if(vk_backend_check_physical_device_support(gpu))
        {
            if(c_dynarray_count(gpu->valid_present_modes)   == 0) continue;
            if(c_dynarray_count(gpu->valid_surface_formats) == 0) continue;

            u32 family_count = c_dynarray_count(gpu->queue_family_properties);
            for(u32 queue_family_index = 0; 
                queue_family_index < family_count; 
                ++queue_family_index)
            {
                VkQueueFamilyProperties *properties = gpu->queue_family_properties + queue_family_index;
                if(properties->queueCount == 0) continue;

                VkQueueFlags flags = properties->queueFlags;

                // NOTE(Sleepster): Count how many capabilities this family has.
                // Fewer capabilities = more dedicated = higher score.
                s32 capability_count = __builtin_popcount(flags &
                                                          (VK_QUEUE_GRAPHICS_BIT  |
                                                           VK_QUEUE_COMPUTE_BIT   |
                                                           VK_QUEUE_TRANSFER_BIT  |
                                                           VK_QUEUE_SPARSE_BINDING_BIT));

                // NOTE(Sleepster): A more dedicated queue scores higher (inverted penalty).
                s32 dedication_score = 4 - capability_count;

                // NOTE(Sleepster): Graphics queue family
                if(flags & VK_QUEUE_GRAPHICS_BIT)
                {
                    if(dedication_score > graphics_score)
                    {
                        graphics_score = dedication_score;
                        graphics_index = (s32)queue_family_index;
                    }
                }

                // NOTE(Sleepster): Present queue family
                VkBool32 supports_presenting = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu->device, queue_family_index, vulkan_context->render_surface, &supports_presenting);
                if(supports_presenting)
                {
                    if(dedication_score > present_score)
                    {
                        present_score = dedication_score;
                        present_index = (s32)queue_family_index;
                    }
                }

                // NOTE(Sleepster): Transfer queue family
                if(flags & VK_QUEUE_TRANSFER_BIT)
                {
                    if(dedication_score > transfer_score)
                    {
                        transfer_score = dedication_score;
                        transfer_index = (s32)queue_family_index;
                    }
                }

                // NOTE(Sleepster): Compute queue family
                if(flags & VK_QUEUE_COMPUTE_BIT)
                {
                    if(dedication_score > compute_score)
                    {
                        compute_score = dedication_score;
                        compute_index = (s32)queue_family_index;
                    }
                }
            }
        }

        if(graphics_index > -1 && present_index > -1 &&
           transfer_index > -1 && compute_index > -1)
        {
            vulkan_context->graphics_queue_family_idx = graphics_index;
            vulkan_context->present_queue_family_idx  = present_index;
            vulkan_context->transfer_queue_family_idx = transfer_index;
            vulkan_context->compute_queue_family_idx  = compute_index;
            vulkan_context->gpu = *gpu;

            VkPhysicalDeviceProperties device_properties = gpu->properties;
            log_info("Device: '%s' selected...\n", device_properties.deviceName);
            switch(device_properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:          log_info("Device Type is unknown...\n");      break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   log_info("Device Type is 'Discrete GPU'\n");  break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: log_info("Device Type is 'Integrated GPU'\n"); break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    log_info("Device Type is 'Virtual GPU'\n");   break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:            log_info("Device Type is 'CPU'\n");           break;
            }

            log_info("GPU DRIVER VERSION: %d.%d.%d\n",
                     VK_VERSION_MAJOR(device_properties.driverVersion),
                     VK_VERSION_MINOR(device_properties.driverVersion),
                     VK_VERSION_PATCH(device_properties.driverVersion));
            log_info("Vulkan API Version: %d.%d.%d\n",
                     VK_VERSION_MAJOR(device_properties.apiVersion),
                     VK_VERSION_MINOR(device_properties.apiVersion),
                     VK_VERSION_PATCH(device_properties.apiVersion));
            return;
        }
    }

    log_fatal("Failed to find a suitable physical device for our requirements...\n");
}

/*
=============
vk_backend_create_logical_device_and_queues
=============
*/

void
vk_backend_create_logical_device_and_queues(vulkan_context_t *vulkan_context)
{
    DynArray_t(VkDeviceQueueCreateInfo) queue_create_infos;
    queue_create_infos = c_dynarray_create(VkDeviceQueueCreateInfo);

    defer(c_dynarray_destroy(queue_create_infos));

    bool8 present_queue_shares_graphics_queue  = vulkan_context->graphics_queue_family_idx  == vulkan_context->present_queue_family_idx;
    bool8 transfer_queue_shares_graphics_queue = vulkan_context->graphics_queue_family_idx  == vulkan_context->transfer_queue_family_idx;
    bool8 compute_queue_shares_any             = (vulkan_context->graphics_queue_family_idx == vulkan_context->compute_queue_family_idx) ||
    (vulkan_context->present_queue_family_idx  == vulkan_context->compute_queue_family_idx)  ||
    (vulkan_context->transfer_queue_family_idx == vulkan_context->compute_queue_family_idx);

    u32 index_count = 1;
    if(!present_queue_shares_graphics_queue)  index_count++;
    if(!transfer_queue_shares_graphics_queue) index_count++;
    if(!compute_queue_shares_any)             index_count++;

    u32 indices[4] = {};
    u32 index = 0;

    indices[index++] = vulkan_context->graphics_queue_family_idx;
    if(!present_queue_shares_graphics_queue)
    {
        indices[index++] = vulkan_context->present_queue_family_idx;
    }
    if(!transfer_queue_shares_graphics_queue)
    {
        indices[index++] = vulkan_context->transfer_queue_family_idx;
    }
    if(!compute_queue_shares_any)
    {
        indices[index++] = vulkan_context->compute_queue_family_idx;
    }


    // TODO(Sleepster): Devices like the laptop really hate this.. Fix it later. 
    float32 priority = 1.0f;
    for(u32 queue_index = 0;
        queue_index < index_count;
        ++queue_index)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = indices[queue_index];
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;

        c_dynarray_push(queue_create_infos, queue_info);
    }


    VkPhysicalDeviceFeatures device_features = {};
    device_features.depthClamp        = VK_TRUE;
    device_features.depthBiasClamp    = VK_TRUE;
    device_features.depthBounds       = vulkan_context->gpu.features.depthBounds;
    device_features.fillModeNonSolid  = VK_TRUE;
    device_features.logicOp           = VK_TRUE;
    device_features.samplerAnisotropy = VK_TRUE;
    //device_features.sparseBinding     = VK_TRUE;

    VkPhysicalDeviceVulkan11Features device_11_features = {};
    device_11_features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    device_11_features.shaderDrawParameters = true;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType                   =  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext                   = &device_11_features;
    device_create_info.queueCreateInfoCount    =  index_count;
    device_create_info.pQueueCreateInfos       =  queue_create_infos;
    device_create_info.pEnabledFeatures        = &device_features;
    device_create_info.enabledExtensionCount   =  g_device_extension_count;
    device_create_info.ppEnabledExtensionNames =  g_device_extensions;
    vkAssert(vkCreateDevice(vulkan_context->gpu.device,
                            &device_create_info,
                             vulkan_context->cpu_allocation_callbacks,
                            &vulkan_context->device));

    vkGetDeviceQueue(vulkan_context->device, vulkan_context->graphics_queue_family_idx, 0, &vulkan_context->graphics_queue);
    vkGetDeviceQueue(vulkan_context->device, vulkan_context->present_queue_family_idx,  0, &vulkan_context->present_queue);
    vkGetDeviceQueue(vulkan_context->device, vulkan_context->transfer_queue_family_idx, 0, &vulkan_context->transfer_queue);
    vkGetDeviceQueue(vulkan_context->device, vulkan_context->compute_queue_family_idx,  0, &vulkan_context->compute_queue);


    log_info("Device meets queue requirements...\n");
    log_info("GRAPHICS | PRESENT | COMPUTE | TRANSFER | DEVICE NAME\n");
    log_info("   %d     |    %d    |    %d    |     %d    | %s\n",
             vulkan_context->graphics_queue_family_idx,
             vulkan_context->present_queue_family_idx,
             vulkan_context->transfer_queue_family_idx,
             vulkan_context->compute_queue_family_idx,
             vulkan_context->gpu.properties.deviceName);
}

/*
=============
vk_backend_create_sync_objects
=============
*/

void
vk_backend_create_sync_objects(vulkan_context_t *vulkan_context)
{
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vulkan_context->swapchain_image_acquired_semaphores = c_arena_push_array(&vulkan_context->permanent_arena, VkSemaphore, vulkan_context->swapchain.image_count);
    vulkan_context->render_complete_semaphores          = c_arena_push_array(&vulkan_context->permanent_arena, VkSemaphore, vulkan_context->swapchain.image_count);

    vulkan_context->image_render_idle_fences            = c_arena_push_array(&vulkan_context->permanent_arena, VkFence,     vulkan_context->swapchain.image_count);
    vulkan_context->image_in_flight_fences              = c_arena_push_array(&vulkan_context->permanent_arena, VkFence*,    vulkan_context->swapchain.image_count);

    for(u32 frame_index = 0;
        frame_index < vulkan_context->swapchain.image_count;
        ++frame_index)
    {
        vkAssert(vkCreateSemaphore(vulkan_context->device, &semaphore_create_info, null, vulkan_context->swapchain_image_acquired_semaphores + frame_index));
        vkAssert(vkCreateSemaphore(vulkan_context->device, &semaphore_create_info, null, vulkan_context->render_complete_semaphores + frame_index));

        vkAssert(vkCreateFence(vulkan_context->device, &fence_create_info, null, vulkan_context->image_render_idle_fences + frame_index));
    }
}

/*
=============
vk_backend_create_command_buffers
=============
*/

void
vk_backend_create_command_pools(vulkan_context_t *vulkan_context)
{
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = vulkan_context->graphics_queue_family_idx;

    vkAssert(vkCreateCommandPool(vulkan_context->device, &command_pool_create_info, null, &vulkan_context->graphics_command_pool));
}

/*
=============
vk_backend_create_command_buffers
=============
*/

void
vk_backend_create_command_buffers(vulkan_context_t *vulkan_context)
{
    u32 image_count = vulkan_context->swapchain.image_count;

    vulkan_context->frame_command_buffers         = c_arena_push_array(&vulkan_context->permanent_arena, VkCommandBuffer, image_count);
    vulkan_context->frame_command_buffer_fences   = c_arena_push_array(&vulkan_context->permanent_arena, VkFence,         image_count);
    vulkan_context->frame_command_buffer_recorded = c_arena_push_array(&vulkan_context->permanent_arena, bool32,          image_count);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool        = vulkan_context->graphics_command_pool;
    command_buffer_allocate_info.commandBufferCount = image_count;
    vkAssert(vkAllocateCommandBuffers(vulkan_context->device, &command_buffer_allocate_info, vulkan_context->frame_command_buffers));

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    for(u32 fence_index = 0;
        fence_index < vulkan_context->swapchain.image_count;
        ++fence_index) 
    {
        vkAssert(vkCreateFence(vulkan_context->device, &fence_create_info, null, vulkan_context->frame_command_buffer_fences));
    }
}

/*
=============
vk_backend_choose_surface_format
=============
*/

VkSurfaceFormatKHR
vk_backend_choose_surface_format(DynArray_t(VkSurfaceFormatKHR) formats)
{
    VkSurfaceFormatKHR result = {};

    if(c_dynarray_count(formats) == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        result.format     = VK_FORMAT_B8G8R8A8_UNORM;
        result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        for(u32 format_index =0; 
            format_index < c_dynarray_count(formats);
            ++format_index) 
        {
            VkSurfaceFormatKHR format = formats[format_index];
            if(format.format == VK_FORMAT_B8G8R8A8_UNORM && 
               format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
            {
                result = format;
                break;
            }
        }
    }

    return(result);
}

/*
=============
vk_backend_choose_present_mode
=============
*/

VkPresentModeKHR
vk_backend_choose_present_mode(DynArray_t(VkPresentModeKHR) present_modes)
{
    // NOTE(Sleepster): We prefer mailbox, but if it doesn't exist of this device,
    // just use immediate mode.
    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 present_index = 0;
        present_index < c_dynarray_count(present_modes);
        ++present_modes)
    {
        VkPresentModeKHR mode = present_modes[present_index];
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            result = mode;
            break;
        }
        if(mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            result = mode;
            break;
        }
    }

    return(result);
}

/*
=============
vk_backend_choose_swapchain_extent
=============
*/

VkExtent2D
vk_backend_choose_swapchain_extent(VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window)
{
    VkExtent2D result = {};
    if(capabilities->currentExtent.width == (u32)-1)
    {
        s32 width;
        s32 height;
        Assert(SDL_GetWindowSizeInPixels(window, &width, &height));

        result.width  = width;
        result.height = height;
    }
    else
    {
        result = capabilities->currentExtent;
    }

    return(result);
}

/*
=============
vk_backend_init_VMA_allocator
=============
*/

void
vk_backend_init_vulkan_allocator(vulkan_context_t *vulkan_context)
{
#if 0
    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice   = vulkan_context->gpu.device;
    allocator_create_info.device           = vulkan_context->device;
    allocator_create_info.instance         = vulkan_context->instance;

    vmaCreateAllocator(&allocator_create_info, &vulkan_context->vulkan_allocator);
#endif
    vulkan_context->vulkan_allocator = vk_allocator_create(vulkan_context, MB(32));
}

/*
=============
vk_backend_swapchain_create
=============
*/

void
vk_backend_swapchain_create(vulkan_context_t *vulkan_context)
{
    VkSurfaceFormatKHR surface_format   = vk_backend_choose_surface_format(vulkan_context->gpu.valid_surface_formats);
    VkPresentModeKHR   present_mode     = vk_backend_choose_present_mode(vulkan_context->gpu.valid_present_modes);
    VkExtent2D         swapchain_extent = vk_backend_choose_swapchain_extent(&vulkan_context->gpu.surface_capabilities, vulkan_context->window);

    vulkan_context->swapchain_format = surface_format;

    s32 width;
    s32 height;
    Assert(SDL_GetWindowSizeInPixels(vulkan_context->window, &width, &height));
    vulkan_context->last_window_width  = vulkan_context->current_window_width;
    vulkan_context->last_window_height = vulkan_context->current_window_height;

    vulkan_context->current_window_width  = width;
    vulkan_context->current_window_height = height;

    VkSwapchainCreateInfoKHR info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = vulkan_context->render_surface,
        .minImageCount    = MAX_FRAMES_IN_FLIGHT,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    };

    if(vulkan_context->graphics_queue_family_idx != vulkan_context->present_queue_family_idx)
    {
        s32 indices[] = {vulkan_context->graphics_queue_family_idx, vulkan_context->present_queue_family_idx};
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices   = (u32*)indices;
    }
    else
    {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode    = present_mode;
    info.clipped        = VK_TRUE;

    vkAssert(vkCreateSwapchainKHR(vulkan_context->device, &info, null, &vulkan_context->swapchain.handle));

    vulkan_context->swapchain.format       = surface_format;
    vulkan_context->swapchain.present_mode = present_mode;
    vulkan_context->swapchain.extent       = swapchain_extent;

    u32 num_images = 0;
    vkAssert(vkGetSwapchainImagesKHR(vulkan_context->device, vulkan_context->swapchain.handle, &num_images, null));
    Expect(num_images > 0, "vkGetSwapchainImagesKHR returned a value of zero...\n");

    vulkan_context->swapchain_images        = c_arena_push_array(&vulkan_context->swapchain_arena,        VkImage,       num_images);
    vulkan_context->swapchain_views         = c_arena_push_array(&vulkan_context->swapchain_arena,        VkImageView,   num_images);
    vulkan_context->swapchain_image_layouts = c_arena_push_array(&vulkan_context->swapchain_arena, VkImageLayout, num_images);
    
    vkAssert(vkGetSwapchainImagesKHR(vulkan_context->device, vulkan_context->swapchain.handle, &num_images, vulkan_context->swapchain_images));
    Expect(num_images > 0, "vkGetSwapchainImagesKHR returned a value of zero...\n");

    vulkan_context->swapchain.image_count = num_images;
    for(u32 image_index = 0;
        image_index < num_images;
        ++image_index)
    {
        // TODO(Sleepster): Is swizzling an issue? Is it needed? The driver may do this automatically
        VkImageViewCreateInfo view_info = {};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = vulkan_context->swapchain_images[image_index];
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = vulkan_context->swapchain.format.format;
        view_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
        view_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
        view_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
        view_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        vkAssert(vkCreateImageView(vulkan_context->device, &view_info, vulkan_context->cpu_allocation_callbacks, &vulkan_context->swapchain_views[image_index]));
        vulkan_context->swapchain_image_layouts[image_index] = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

/*
=============
vk_backend_destroy_swapchain
=============
*/

void
vk_backend_swapchain_destroy(vulkan_context_t *vulkan_context)
{
    for(u32 view_index = 0;
        view_index < MAX_FRAMES_IN_FLIGHT;
        ++view_index)
    {
        vkDestroyImageView(vulkan_context->device, vulkan_context->swapchain_views[view_index], vulkan_context->cpu_allocation_callbacks);
    }

    ZeroMemory(vulkan_context->swapchain_views, sizeof(VkImageView) * MAX_FRAMES_IN_FLIGHT);
    vkDestroySwapchainKHR(vulkan_context->device, vulkan_context->swapchain.handle, vulkan_context->cpu_allocation_callbacks);
}


/*
=============
vk_backend_swapchain_rebuild
=============
*/

void
vk_backend_swapchain_rebuild(vulkan_context_t *vulkan_context)
{
    if(vulkan_context->current_window_width > 0 && vulkan_context->current_window_height > 0)
    {            
        VkResult wait_result = vkDeviceWaitIdle(vulkan_context->device);
        if(!vk_backend_result_is_success(wait_result))
        {
            log_error("Begin frame failed on VkDeviceWaitIdle(): '%s'...\n", 
                      vk_backend_vulkan_result_string(wait_result, true));
        }

        vk_backend_swapchain_destroy(vulkan_context);
        vk_backend_image_destroy(vulkan_context, &vulkan_context->depth_buffer);
        vk_backend_destroy_framebuffers(vulkan_context);

        c_arena_reset(&vulkan_context->swapchain_arena);

        vk_backend_swapchain_create(vulkan_context);
        vk_backend_create_depth_buffer(vulkan_context);
        vk_backend_create_framebuffers(vulkan_context);

        vulkan_context->last_window_size_generation = vulkan_context->window_size_generation;
        log_debug("Window resized...\n");

        vulkan_context->rebuilding_swapchain = false;
    }
}

/*
=============
vk_backend_choose_supported_depth_format
=============
*/

VkFormat
vk_backend_choose_supported_depth_format(gpu_info_t *gpu, VkFormat *formats, u32 num_formats, VkImageTiling tiling, VkFormatFeatureFlagBits features)
{
    VkFormat result = VK_FORMAT_UNDEFINED;
    for(u32 format_index = 0;
        format_index < num_formats;
        ++format_index)
    {
        VkFormat format = formats[format_index];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(gpu->device, format, &properties);
        if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
        {
            result = format;
            break;
        }
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
        {
            result = format;
            break;
        }
    }

    if(result == VK_FORMAT_UNDEFINED)
    {
        Expect(false, "Failed to find a supported depth format...\n");
    }

    return(result);
}

/*
=============
vk_backend_create_depth_buffer
=============
*/

void
vk_backend_create_depth_buffer(vulkan_context_t *vulkan_context)
{
    VkFormat depth_formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT 
    };
    vulkan_context->depth_format = vk_backend_choose_supported_depth_format(&vulkan_context->gpu, 
                                                                            depth_formats,
                                                                            ArrayCount(depth_formats),
                                                                            VK_IMAGE_TILING_OPTIMAL,
                                                                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // NOTE(Sleepster): Allocate image 
    vulkan_image_t *image = &vulkan_context->depth_buffer;
    {
        // TODO(Sleepster): We need a proper backend image handler, after we have that we can replace this crap
        image->internal_format = vulkan_context->depth_format;
        image->width           = vulkan_context->current_window_width;
        image->height          = vulkan_context->current_window_height;

        // NOTE(Sleepster): The sharing mode matters very little here
        // https://developer.nvidia.com/blog/vulkan-dos-donts/
        //
        // "VkSharingMode is ignored by the driver, so VK_SHARING_MODE_CONCURRENT 
        // incurs no overhead relative to VK_SHARING_MODE_EXCLUSIVE."
        VkImageCreateInfo info = {};
        info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType     = VK_IMAGE_TYPE_2D;
        info.extent.width  = image->width;
        info.extent.height = image->height;
        info.extent.depth  = 1;
        info.mipLevels     = 1;
        info.arrayLayers   = 1;
        info.format        = image->internal_format;
        info.tiling        = VK_IMAGE_TILING_OPTIMAL;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.samples       = VK_SAMPLE_COUNT_1_BIT;
        info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        vkAssert(vkCreateImage(vulkan_context->device, &info, vulkan_context->cpu_allocation_callbacks, &image->handle));

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(vulkan_context->device, image->handle, &memory_requirements);
#if 1
        image->allocation = vk_allocator_allocate(&vulkan_context->vulkan_allocator, 
                                                                    &memory_requirements, 
                                                                     VULKAN_MEMORY_USAGE_GPU_ONLY);
        VkResult code = vkBindImageMemory(vulkan_context->device, image->handle, image->allocation.memory, image->allocation.offset);
        if(!vk_backend_result_is_success(code))
        {
            Expect(false, "Failed to bind the memory for the depth buffer...\n");
        }
#else
        s32 memory_index = vk_backend_find_memory_index(vulkan_context, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if(memory_index == -1)
        {
            Expect(false, "Failed to find a valid memory index for the depth buffer...\n");
        }

        VkMemoryAllocateInfo memory_allocation_info = {};
        memory_allocation_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocation_info.memoryTypeIndex = memory_index;
        memory_allocation_info.allocationSize  = memory_requirements.size;
        VkResult code = vkAllocateMemory(vulkan_context->device, &memory_allocation_info, vulkan_context->cpu_allocation_callbacks, &image->gpu_memory);
        if(!vk_backend_result_is_success(code))
        {
            Expect(false, "Failed to allocate memory for the depth buffer...\n");
        }

        code = vkBindImageMemory(vulkan_context->device, image->handle, image->gpu_memory, null);
        if(!vk_backend_result_is_success(code))
        {
            Expect(false, "Failed to bind the memory for the depth buffer...\n");
        }
#endif
    }

    // NOTE(Sleepster): Create image view. 
    {
        // TODO(Sleepster): We need a proper backend image handler, after we have that we can replace this crap
        VkImageViewCreateInfo view_info = {};
        view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image    = image->handle;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format   = image->internal_format;
        view_info.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        vkAssert(vkCreateImageView(vulkan_context->device, &view_info, vulkan_context->cpu_allocation_callbacks, &image->view));
    }
}


/*
=============
vk_backend_create_renderpasses
=============
*/

void
vk_backend_create_renderpasses(vulkan_context_t *vulkan_context)
{
    VkAttachmentDescription attachments[2];

    // NOTE(Sleepster): Color attachment 
    attachments[0] = {
        .format         = vulkan_context->swapchain.format.format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // NOTE(Sleepster): Depth attachments 
    attachments[1] = {
        .format         = vulkan_context->depth_format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depth_attachment_reference = {
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL 
    };

    VkSubpassDescription primary_subpass = {
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = 1,
        .pColorAttachments       = &color_attachment_reference,
        .pDepthStencilAttachment = &depth_attachment_reference,
        // NOTE(Sleepster): These supply imput to the shader
        .inputAttachmentCount    = 0,
        .pInputAttachments       = null,
        // NOTE(Sleepster): Multisampling resolution 
        .pResolveAttachments     = null,
        // NOTE(Sleepster): Items that should be preserved between subpasses and future renderpasses
        .preserveAttachmentCount = 0,
        .pPreserveAttachments    = null
    };

    // NOTE(Sleepster): We only have 1 subpass, so it's VK_SUBPASS_EXTERNAL here. 
    VkSubpassDependency subpass_dependencies = {
        .srcSubpass      = VK_SUBPASS_EXTERNAL,
        .dstSubpass      = null,
        .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask   = 0,
        .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = ArrayCount(attachments),
        .pAttachments    = attachments,
        .subpassCount    = 1,
        .pSubpasses      = &primary_subpass,
        .dependencyCount = 1,
        .pDependencies   = &subpass_dependencies,
        .pNext           = null,
        .flags           = 0
    };
    vkAssert(vkCreateRenderPass(vulkan_context->device, 
                               &renderpass_create_info, 
                                vulkan_context->cpu_allocation_callbacks, 
                               &vulkan_context->primary_renderpass));
}

void
vk_backend_begin_renderpass(vulkan_context_t *vulkan_context, 
                            VkRenderPass renderpass, 
                            VkFramebuffer framebuffer, 
                            u32           attachment_count,
                            VkClearValue *clear_values)
{
    VkRenderPassBeginInfo renderpass_info = {};
    renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_info.renderPass  = renderpass;
    renderpass_info.framebuffer = framebuffer;
    renderpass_info.renderArea = {
        .offset.x = 0,
        .offset.y = 0,
        .extent.width  = vulkan_context->current_window_width,
        .extent.height = vulkan_context->current_window_height
    };

    renderpass_info.clearValueCount = attachment_count;
    renderpass_info.pClearValues    = clear_values;

    vkCmdBeginRenderPass(*vulkan_context->render_command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
}

/*
=============
vk_backend_create_framebuffers
=============
*/

void
vk_backend_create_framebuffers(vulkan_context_t *vulkan_context)
{
    vulkan_context->framebuffers = c_arena_push_array(&vulkan_context->swapchain_arena, VkFramebuffer, vulkan_context->swapchain.image_count);
    for(u32 frame_index = 0;
        frame_index < vulkan_context->swapchain.image_count;
        ++frame_index)
    {
        VkImageView attachments[] = {
            vulkan_context->swapchain_views[frame_index],
            vulkan_context->depth_buffer.view
        };
        VkFramebufferCreateInfo framebuffer_create_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = vulkan_context->primary_renderpass,
            .attachmentCount = ArrayCount(attachments),
            .pAttachments    = attachments,
            .width           = vulkan_context->current_window_width,
            .height          = vulkan_context->current_window_height,
            .layers          = 1
        };
        vkAssert(vkCreateFramebuffer(vulkan_context->device, 
                                     &framebuffer_create_info, 
                                     vulkan_context->cpu_allocation_callbacks, 
                                     &vulkan_context->framebuffers[frame_index]));
    }
}

/*
=============
vk_backend_destroy_framebuffers
=============
*/

void
vk_backend_destroy_framebuffers(vulkan_context_t *vulkan_context)
{
    for(u32 frame_index = 0;
        frame_index < MAX_FRAMES_IN_FLIGHT;
        ++frame_index)
    {
        vkDestroyFramebuffer(vulkan_context->device, 
                             vulkan_context->framebuffers[frame_index],
                             vulkan_context->cpu_allocation_callbacks);
    }
}

/*
=============
vk_backend_create_render_buffers
=============
*/

void
vk_backend_create_render_buffers(vulkan_context_t *vulkan_context)
{
    VkBufferUsageFlagBits vertex_buffer_usage_bits = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
                                                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT  |
                                                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    VkBufferUsageFlagBits index_buffer_usage_bits = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
                                                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    vulkan_context->main_vertex_buffer   = vk_backend_buffer_create(vulkan_context, sizeof(vertex_t) * 4,                       vertex_buffer_usage_bits, VULKAN_MEMORY_USAGE_GPU_ONLY);
    vulkan_context->main_index_buffer    = vk_backend_buffer_create(vulkan_context, sizeof(u32) * MAX_VULKAN_INDEX_BUFFER_SIZE, index_buffer_usage_bits,  VULKAN_MEMORY_USAGE_GPU_ONLY);
    vulkan_context->staging_infos        = c_dynarray_create(vulkan_staging_info_t);

    // NOTE(Sleepster): Create the frame-based buffers 
    for(u32 index = 0;
        index < MAX_FRAMES_IN_FLIGHT;
        ++index)
    {
        vulkan_context->frame_render_buffer[index] = vk_backend_buffer_create(vulkan_context, 
                                                                              MB(128), 
                                                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                                                              VULKAN_MEMORY_USAGE_GPU_ONLY); 
        vulkan_context->staging_buffers[index] = vk_backend_staging_buffer_create(vulkan_context,
                                                                                  MB(128),
                                                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                                  VULKAN_MEMORY_USAGE_CPU_TO_GPU);
    }
    // NOTE(Sleepster): Fill vertex buffer
    vertex_t vertices[] = {
        [0] = {
            .vPosition = {0.5, -0.5, 0.0, 1.0},
            .vCorner   = {1.0, 1.0}
        },
        [1] = {
            .vPosition = {0.5, 0.5, 0.0, 1.0},
            .vCorner   = {1.0, 0.0}
        },
        [2] = {
            .vPosition = {-0.5, 0.5, 0.0, 1.0},
            .vCorner   = {0.0, 0.0}
        },
        [3] = {
            .vPosition = {-0.5, -0.5, 0.0, 1.0},
            .vCorner   = {0.0, 1.0}
        } 
    };
    //vk_backend_buffer_copy_data(&vulkan_context->staging_buffers[0], vertices, sizeof(vertex_t) * 4, 0);
    //vk_backend_buffer_copy_buffer(vulkan_context, &vulkan_context->staging_buffers[0], &vulkan_context->main_vertex_buffer, 0, vulkan_context->main_vertex_buffer.size, 0);

    // NOTE(Sleepster): Fill the index buffer 
    u32 *indices = c_arena_push_array(&vulkan_context->initialization_arena, u32, MAX_VULKAN_INDEX_BUFFER_SIZE);
    u32  index_offset = 0;
    for(u32 index = 0;
        index < MAX_VULKAN_INDEX_BUFFER_SIZE;
        index += 6)
    {
        indices[index + 0] = index_offset + 0;
        indices[index + 1] = index_offset + 1;
        indices[index + 2] = index_offset + 2;
        indices[index + 3] = index_offset + 2;
        indices[index + 4] = index_offset + 3;
        indices[index + 5] = index_offset + 0;

        index_offset += 4;
    }
    //vk_backend_buffer_copy_data(&vulkan_context->staging_buffers[0], indices, sizeof(u32) * MAX_VULKAN_INDEX_BUFFER_SIZE, 0);
    //vk_backend_buffer_copy_buffer(vulkan_context, &vulkan_context->staging_buffers[0], &vulkan_context->main_index_buffer, 0, vulkan_context->main_index_buffer.size, 0);
    
    vk_backend_buffer_stage_data(vulkan_context, (byte*)vertices, sizeof(vertices), &vulkan_context->main_vertex_buffer);
    vk_backend_buffer_stage_data(vulkan_context, (byte*)indices,  sizeof(indices),  &vulkan_context->main_index_buffer);

    VkCommandBuffer scratch_buffer;
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vulkan_context->graphics_command_pool,
        .commandBufferCount = 1,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };
    vkAssert(vkAllocateCommandBuffers(vulkan_context->device, &command_buffer_allocate_info, &scratch_buffer));

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(scratch_buffer, &begin_info);

    vk_backend_buffer_flush_staging_buffer(vulkan_context, scratch_buffer);

    VkMemoryBarrier barrier = {};
    barrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
    vkCmdPipelineBarrier(scratch_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                         0, 1, &barrier, 0, null, 0, null);
    vkEndCommandBuffer(scratch_buffer);

    VkSubmitInfo submit_info = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &scratch_buffer
    };

    vkAssert(vkQueueSubmit(vulkan_context->graphics_queue, 1, &submit_info, 0));
    vkAssert(vkQueueWaitIdle(vulkan_context->graphics_queue));
}

/*
=============
vk_backend_create_descriptor_pool
=============
*/

void
vk_backend_create_descriptor_pool(vulkan_context_t *vulkan_context)
{
    #define MAX_POOL_SET_TYPES (5)

    // NOTE(Sleepster): This is 10 for now...
    // https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkDescriptorType.html
    VkDescriptorPoolSize sizes[MAX_POOL_SET_TYPES];
    sizes[0] = {
        .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 4000,
    };
    sizes[1] = {
        .type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 2000,
    };
    sizes[2] = {
        .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 2000,
    };
    sizes[3] = {
        .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 200,
    };
    sizes[4] = {
        .type            = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        .descriptorCount = 200,
    };

    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = 100000,
        .poolSizeCount = MAX_POOL_SET_TYPES,
        .pPoolSizes    = sizes
    };   
    vkAssert(vkCreateDescriptorPool(vulkan_context->device, 
                                   &pool_create_info, 
                                    vulkan_context->cpu_allocation_callbacks, 
                                   &vulkan_context->first_descriptor_pool));
}

/*
=============
vk_backend_create_render_pipeline
=============
*/

// TODO(Sleepster): This will have to get moved out, we need MUCH more infrastructure to correctly support pipeline objects
// the idea originally was to rely on dynamic state to make it so that we can simply just change the blending modes and everything
// else at runtime. However, this isn't an option without requiring Vulkan 1.3 and VK_EXT_DYNAMIC_STATE3, which I'm not going
// to do. So, the idea is now that instead of the shader owning a single pipeline we will instead store 2 items:
//
// 1.) A hash table for each of the pipelines that use the shader. With each version of the pipeline state being the key into 
//     the table.
//
// 2.) An array of occupied indices in the hash table for easy iteration over loaded pipelines.
//
// This also only creates a GRAPHICS pipeline. Bad.
void
vk_backend_create_render_pipeline(vulkan_context_t *vulkan_context, vulkan_shader_t *shader, bool8 wireframe)
{
    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL,
        .lineWidth               = 1.0f,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling_state = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable   = false,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading      = 1.0f,
        .pSampleMask           = 0,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable      = false,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = true,
        .depthWriteEnable      = true,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = false,
        .stencilTestEnable     = false,
    };

    VkPipelineColorBlendAttachmentState blend_settings = {
        .blendEnable         = true,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = true,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &blend_settings
    };

    VkDynamicState dynamic_state[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_data = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates    = dynamic_state,
        .dynamicStateCount = ArrayCount(dynamic_state)
    };

    // NOTE(Sleepster): Vertex Attribute stuff
    VkVertexInputAttributeDescription attributes[] = {
        [0] = {
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
            .location = 0,
            .offset   = offsetof(vertex_t, vPosition)
        },
        [1] = {
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT, 
            .location = 1,
            .offset   = offsetof(vertex_t, vCorner)
        },
        [2] = {
            .binding = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT, 
            .location = 2,
            .offset   = offsetof(vertex_t, vPadding)
        },
    };

    VkVertexInputBindingDescription vertex_input_desc = {
        .binding   = 0,
        .stride    = sizeof(vertex_t),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    // NOTE(Sleepster): Vertex Attribute stuff
    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType                           =  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   =  1,
        .pVertexBindingDescriptions      = &vertex_input_desc,
        .vertexAttributeDescriptionCount =  ArrayCount(attributes),
        .pVertexAttributeDescriptions    =  attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo assembly_state = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = shader->descriptor_set_count,
        .pSetLayouts            = shader->layouts,
        .pushConstantRangeCount = shader->push_constant_count,
        .pPushConstantRanges    = shader->push_constants,
    };

    vkAssert(vkCreatePipelineLayout(vulkan_context->device,
                                   &pipeline_layout_info,
                                    vulkan_context->cpu_allocation_callbacks,
                                   &shader->pipeline_layout));

    VkPipelineShaderStageCreateInfo stage_create_infos[MAX_VULKAN_SHADER_STAGES]; 
    for(u32 stage_index = 0;
        stage_index < shader->stage_count;
        ++stage_index)
    {
        vulkan_shader_stage_t *stage = shader->stages + stage_index;
        stage_create_infos[stage_index]   = stage->pipeline_stage_create_info;
    }

    VkViewport viewport = {
        .x        =  (float32)0.0f,
        .y        =  (float32)vulkan_context->current_window_height,
        .width    =  (float32)vulkan_context->current_window_width,
        .height   = -(float32)vulkan_context->current_window_height,
        .minDepth =  0.0f,
        .maxDepth =  1.0f
    };

    VkRect2D scissor = {
        .extent = {
            .width  = vulkan_context->current_window_width,
            .height = vulkan_context->current_window_height
        },
    };

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .pScissors     = &scissor,
        .scissorCount  = 1
    };

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType               =  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages             =  stage_create_infos,
        .stageCount          =  shader->stage_count,
        .pVertexInputState   = &vertex_input_state,
        .pInputAssemblyState = &assembly_state,
        .pViewportState      = &viewport_info,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState   = &multisampling_state,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blend_state,
        .pDynamicState       = &dynamic_state_data,
        .layout              =  shader->pipeline_layout,
        .renderPass          =  vulkan_context->primary_renderpass,
        .subpass             =  0,
        .basePipelineHandle  =  VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VkResult success = vkCreateGraphicsPipelines(vulkan_context->device,
                                                 VK_NULL_HANDLE,
                                                 1,
                                                &pipeline_create_info,
                                                 vulkan_context->cpu_allocation_callbacks,
                                                &shader->pipeline);
    if(!vk_backend_result_is_success(success))
    {
        log_fatal("Failure to create the Vulkan Graphics Pipeline... Error: '%s'...\n", 
                  vk_backend_vulkan_result_string(success, true));
    }
    else
    {
        log_info("Vulkan Graphics Pipeline created...\n");
    }
}

/*
=============
vk_backend_create_renderpass
=============
*/

VkRenderPass
vk_backend_renderpass_create(vulkan_context_t    *vulkan_context, 
                             image_t             *attachments,
                             u32                  attachment_count,
                             VkImageLayout       *initial_layouts,
                             VkImageLayout       *final_layouts,
                             VkAttachmentLoadOp  *load_operations,
                             VkAttachmentStoreOp *store_operations,
                             VkImageLayout       *attachment_types)
{
    VkRenderPass result = null;

    u32 color_attachment_count    = 0;
    bool32 depth_attachment_found = false;

    VkAttachmentDescription attachment_descs[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkAttachmentReference   attachment_refs[MAX_RENDER_TARGET_ATTACHMENTS]  = {};

    VkAttachmentReference   color_attachments[MAX_RENDER_TARGET_ATTACHMENTS] = {};
    VkAttachmentReference   depth_attachment = {};
    for(u32 attachment_index = 0;
        attachment_index < attachment_count;
        ++attachment_index)
    {
        VkAttachmentDescription *attachment       =  attachment_descs + attachment_index;
        VkAttachmentReference   *attachment_ref   =  attachment_refs  + attachment_index;
        vulkan_image_t          *image_attachment = &(attachments     + attachment_index)->vulkan_image;

        VkImageLayout            initial_layout   = initial_layouts[attachment_index];
        VkImageLayout            final_layout     = final_layouts[attachment_index];
        VkImageLayout            attachment_type  = attachment_types[attachment_index];
        VkAttachmentLoadOp       load_operation   = load_operations[attachment_index];
        VkAttachmentStoreOp      store_operation  = store_operations[attachment_index];

        // NOTE(Sleepster): We don't support Multisampling.
        attachment->format         = image_attachment->internal_format;
        attachment->samples        = VK_SAMPLE_COUNT_1_BIT;
        attachment->loadOp         = load_operation;
        attachment->storeOp        = store_operation;
        attachment->stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout  = initial_layout;
        attachment->finalLayout    = final_layout;

        attachment_ref->attachment = attachment_index;
        attachment_ref->layout     = attachment_type;

        if(image_attachment->internal_format != VK_FORMAT_D24_UNORM_S8_UINT &&
           image_attachment->internal_format != VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            color_attachments[color_attachment_count++] = attachment_refs[attachment_index];
        }
        else if(image_attachment->internal_format == VK_FORMAT_D24_UNORM_S8_UINT || 
                image_attachment->internal_format == VK_FORMAT_D32_SFLOAT_S8_UINT)
        {
            Assert(!depth_attachment_found);

            depth_attachment = *attachment_ref;
            depth_attachment_found = true;
        }
    }

    // TODO(Sleepster): Maybe we need to allow some more of these too be modifiable? For now I don't see a reason.
    // For now, we know we'll never need multisampling (at least right now we won't). We MAY need preserved attachments
    // in the future, but I'm not gonna deal with that right now
    VkSubpassDescription subpass_desc    = {};
    subpass_desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount    = color_attachment_count;
    subpass_desc.pColorAttachments       = color_attachments; 
    subpass_desc.pDepthStencilAttachment = depth_attachment_found ? &depth_attachment : null;
    // NOTE(Sleepster): These supply imput to the shader
    subpass_desc.inputAttachmentCount    = 0;
    subpass_desc.pInputAttachments       = null;
    // NOTE(Sleepster): Multisampling resolution 
    subpass_desc.pResolveAttachments     = null;
    // NOTE(Sleepster): Items that should be preserved between subpasses and future renderpasses
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments    = null;

    // TODO(Sleepster): Does this need to exist? Do we need to allow this to be customizable?
    VkSubpassDependency primary_subpass_deps = {};
    primary_subpass_deps.srcSubpass      = VK_SUBPASS_EXTERNAL;
    primary_subpass_deps.dstSubpass      = null;
    primary_subpass_deps.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    primary_subpass_deps.srcAccessMask   = 0;
    primary_subpass_deps.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    primary_subpass_deps.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    primary_subpass_deps.dependencyFlags = 0;

    VkRenderPassCreateInfo renderpass_info = {};
    renderpass_info.sType           =  VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_info.attachmentCount =  attachment_count;
    renderpass_info.pAttachments    =  attachment_descs;
    renderpass_info.subpassCount    =  1;
    renderpass_info.pSubpasses      = &subpass_desc;
    renderpass_info.dependencyCount =  1;
    renderpass_info.pDependencies   = &primary_subpass_deps;
    renderpass_info.pNext           =  null;
    renderpass_info.flags           =  0;

    vkAssert(vkCreateRenderPass(vulkan_context->device, &renderpass_info, vulkan_context->cpu_allocation_callbacks, &result));

    return(result);
}

void
vk_backend_renderpass_destroy(vulkan_context_t *vulkan_context, VkRenderPass renderpass)
{
    vkDestroyRenderPass(vulkan_context->device, renderpass, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_ramebuffer_create
=============
*/

VkFramebuffer
vk_backend_framebuffer_create(vulkan_context_t  *vulkan_context, 
                              VkRenderPass       renderpass,
                              image_t           *attachments,
                              u32                attachment_count, 
                              u32                width,
                              u32                height)
{
    VkFramebuffer result = null;

    VkImageView views[10] = {};
    for(u32 attachment_index = 0;
        attachment_index < attachment_count;
        ++attachment_index)
    {
        vulkan_image_t *image   = &(attachments + attachment_index)->vulkan_image;
        views[attachment_index] = image->view;
    }

    VkFramebufferCreateInfo info = {};
    info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass      = renderpass;
    info.pAttachments    = views;
    info.attachmentCount = attachment_count;
    info.width           = width;
    info.height          = height;
    info.layers          = 1;

    vkAssert(vkCreateFramebuffer(vulkan_context->device, 
                                &info, 
                                 vulkan_context->cpu_allocation_callbacks, 
                                &result));

    return(result);
}

/*
=============
vk_backend_framebuffer_destroy
=============
*/

void
vk_backend_framebuffer_destroy(vulkan_context_t *vulkan_context, VkFramebuffer framebuffer)
{
    vkDestroyFramebuffer(vulkan_context->device, framebuffer, vulkan_context->cpu_allocation_callbacks);
}

/*
=============
vk_backend_init
=============
*/

// TODO(Sleepster): 
// - Image handler and manager
// - Shader handler and manager
void
vk_backend_init(vulkan_context_t *vulkan_context, SDL_Window *window) 
{
    log_info("----- vk_backend_init -----\n");
    vk_backend_create_instance(vulkan_context);

    // NOTE(Sleepster): This is for the render surface that we present too 
    vk_backend_create_surface(vulkan_context, window);

    // NOTE(Sleepster): Enumerate the available devices on this PC and get the best fit device
    vk_backend_select_physical_device(vulkan_context);

    // NOTE(Sleepster): Create the logical device interface for the context 
    vk_backend_create_logical_device_and_queues(vulkan_context);

    // NOTE(Sleepster): Either use VMA or our own 
    vk_backend_init_vulkan_allocator(vulkan_context);

    // NOTE(Sleepster): Generate the swapchain, it's images, and the views for those images. We do not create the depth buffer. 
    vk_backend_swapchain_create(vulkan_context);

    // NOTE(Sleepster): Create the command pools for our context's buffers
    vk_backend_create_command_pools(vulkan_context);

    // NOTE(Sleepster): Create the comamand buffers for the rendering in the engine 
    vk_backend_create_command_buffers(vulkan_context);

    // NOTE(Sleepster): Create the image acquisition and rendering completion semaphore objects, do the same for the fences
    vk_backend_create_sync_objects(vulkan_context);

    // NOTE(Sleepster): Generate the program's depth buffer 
    vk_backend_create_depth_buffer(vulkan_context);

    // NOTE(Sleepster): Generate the programs renderpasses 
    vk_backend_create_renderpasses(vulkan_context);

    // NOTE(Sleepster): Generate the programs framebuffers 
    vk_backend_create_framebuffers(vulkan_context);

    // NOTE(Sleepster): Generate the main vertex, index, and instanced_rendering buffers
    vk_backend_create_render_buffers(vulkan_context);

    // NOTE(Sleepster): Generate the descriptor pools 
    vk_backend_create_descriptor_pool(vulkan_context);

    c_arena_destroy(&vulkan_context->initialization_arena);
    log_info("----- Vulkan backend initialized -----\n");
}

/*
=============
vk_backend_render_frame
=============
*/

void
vk_backend_render_frame(vulkan_context_t *vulkan_context, renderer_state_t *renderer_state)
{
    bool32 window_resize = (vulkan_context->window_size_generation != vulkan_context->last_window_size_generation);
    if(window_resize || vulkan_context->rebuilding_swapchain)
    {
        vk_backend_swapchain_rebuild(vulkan_context);
        return;
    }

    vulkan_context->image_render_idle_fence   = vulkan_context->image_render_idle_fences            + vulkan_context->current_frame_index;
    vulkan_context->image_acquired_semaphore  = vulkan_context->swapchain_image_acquired_semaphores + vulkan_context->current_frame_index;

    vkAssert(vkWaitForFences(vulkan_context->device, 1, vulkan_context->image_render_idle_fence, true, U64_MAX));

    u32 image_index = 0;
    VkResult code = vkAcquireNextImageKHR(vulkan_context->device, vulkan_context->swapchain.handle, U64_MAX, *vulkan_context->image_acquired_semaphore, null, &image_index);
    if(code == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_context->rebuilding_swapchain = true;
        return;
    }
    else if(code != VK_SUCCESS && code != VK_SUBOPTIMAL_KHR)
    {
        Expect(false, "Could not acquire the next swapchain image!...\n");
    }

    vulkan_context->image_in_flight_fence     = vulkan_context->image_in_flight_fences     + image_index;
    vulkan_context->render_complete_semaphore = vulkan_context->render_complete_semaphores + image_index;
    vulkan_context->render_command_buffer     = vulkan_context->frame_command_buffers      + image_index;
    vulkan_context->render_framebuffer        = vulkan_context->framebuffers               + image_index;
    if(*vulkan_context->image_in_flight_fence != VK_NULL_HANDLE)
    {
        vkAssert(vkWaitForFences(vulkan_context->device, 1, *vulkan_context->image_in_flight_fence, true, U64_MAX));
    }
    *vulkan_context->image_in_flight_fence = vulkan_context->image_render_idle_fence;
    vulkan_context->current_image_index    = image_index;
    // TODO(Sleepster): Multithreading is important... This is not good for that... 
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkAssert(vkBeginCommandBuffer(*vulkan_context->render_command_buffer, &begin_info));

#if 0
    VkRenderPassBeginInfo renderpass_info = {};
    renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_info.renderPass  =  vulkan_context->primary_renderpass;
    renderpass_info.framebuffer = *vulkan_context->render_framebuffer;
    renderpass_info.renderArea = {
        .offset.x = 0,
        .offset.y = 0,
        .extent.width = vulkan_context->current_window_width,
        .extent.height = vulkan_context->current_window_height
    };

    VkClearValue clear_values[2] = {};
    clear_values[0].color.float32[0] = 0.4;
    clear_values[0].color.float32[1] = 0.3;
    clear_values[0].color.float32[2] = 0.8;
    clear_values[0].color.float32[3] = 1.0;

    clear_values[1].depthStencil.depth   = 0.0;
    clear_values[1].depthStencil.stencil = 0;

    renderpass_info.clearValueCount = 2;
    renderpass_info.pClearValues    = clear_values;
    
    vkCmdBeginRenderPass(*vulkan_context->render_command_buffer, &renderpass_info, VK_SUBPASS_CONTENTS_INLINE);
#endif
    if(renderer_state->current_window_size_generation != renderer_state->last_window_size_generation)
    {
        s_renderer_resize_render_targets(renderer_state, renderer_state->window_size);
    }

    // render commands here
    for(u32 command_list_index = 0;
        command_list_index < renderer_state->command_list_count;
        ++command_list_index)
    {
        render_command_list_t *command_list = renderer_state->command_lists + command_list_index;
        for(u32 command_index = 0;
            command_index < command_list->command_count;
            ++command_index)
        {
            render_command_t *command = command_list->commands + command_index;
            switch(command->header.command_type)
            {
                case RCT_BindRenderTarget:
                {
                    render_command_bind_render_target_t *rt_command = (render_command_bind_render_target_t*)command;
                    if(command_list->active_render_target)
                    {
                        vkCmdEndRenderPass(*vulkan_context->render_command_buffer);
                        command_list->active_render_target = null;
                    }
                    vk_backend_begin_renderpass(vulkan_context, 
                                                rt_command->render_target->renderpass, 
                                                rt_command->render_target->framebuffer,
                                                rt_command->render_target->attachment_count,
                                                rt_command->render_target->clear_values);
                    command_list->active_render_target = rt_command->render_target;
                }break;
                case RCT_BeginRenderGroup:
                {
                }break;
                case RCT_DrawRectangle:
                {
                }break;
                case RCT_EndRenderGroup:
                {
                }break;
                case RCT_PresentFrame:
                {
                    vkCmdEndRenderPass(*vulkan_context->render_command_buffer);

                    // blit
                    image_t *src_color_buffer      = command_list->active_render_target->primary_color_buffer;
                    VkImage  present_image         = vulkan_context->swapchain_images[vulkan_context->current_image_index];
                    VkImageLayout swapchain_layout = vulkan_context->swapchain_image_layouts[vulkan_context->current_image_index];

                    command_list->active_render_target = null;

                    // NOTE(Sleepster): We only ever care about the color buffer since the swapchain image is just a color buffer 
                    VkImageSubresourceRange src_range = {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseArrayLayer = 0,
                        .baseMipLevel   = 0,
                        .layerCount     = 1,
                        .levelCount     = 1,
                    };

                    VkImageSubresourceRange dst_range = {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    };

                    // NOTE(Sleepster): transition the color buffer to TRANSFER_SRC 
                    vk_backend_image_change_layout(vulkan_context, 
                                                  *vulkan_context->render_command_buffer,
                                                   src_color_buffer->vulkan_image.handle,
                                                   src_color_buffer->vulkan_image.layout,
                                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   0,
                                                   0,
                                                   src_range);
                    src_color_buffer->vulkan_image.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                    // NOTE(Sleepster): transition the swapchain image to TRANSFER_DST 
                    vk_backend_image_change_layout(vulkan_context, 
                                                  *vulkan_context->render_command_buffer,
                                                   present_image,
                                                   swapchain_layout,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   0,
                                                   0,
                                                   dst_range);

                    // NOTE(Sleepster): Do the blit 
                    VkImageBlit blit_region = {
                        .srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .srcSubresource.mipLevel       = 0,
                        .srcSubresource.baseArrayLayer = 0,
                        .srcSubresource.layerCount     = 1,
                        .srcOffsets[0] = (VkOffset3D){0, 0, 0},
                        .srcOffsets[1] = (VkOffset3D){(s32)src_color_buffer->vulkan_image.width, (s32)src_color_buffer->vulkan_image.height, 1},

                        .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .dstSubresource.mipLevel = 0, 
                        .dstSubresource.baseArrayLayer = 0,
                        .dstSubresource.layerCount = 1,

                        .dstOffsets[0] = (VkOffset3D){0, 0, 0},
                        .dstOffsets[1] = (VkOffset3D){(s32)vulkan_context->current_window_width, (s32)vulkan_context->current_window_height, 1},
                    };
                    vkCmdBlitImage(*vulkan_context->render_command_buffer,
                                   src_color_buffer->vulkan_image.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   present_image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,
                                   &blit_region,
                                   VK_FILTER_NEAREST);

                    // NOTE(Sleepster): Transfer the images back to what they were before the blit 
                    vk_backend_image_change_layout(vulkan_context, 
                                                  *vulkan_context->render_command_buffer,
                                                   src_color_buffer->vulkan_image.handle,
                                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                   0,
                                                   0,
                                                   src_range);

                    vk_backend_image_change_layout(vulkan_context, 
                                                   *vulkan_context->render_command_buffer,
                                                   present_image,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                   0,
                                                   0,
                                                   dst_range);
                    src_color_buffer->vulkan_image.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    vulkan_context->swapchain_image_layouts[vulkan_context->current_image_index] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                }break;
                default: {InvalidCodePath;}break;
            }
        }

        command_list->bind_material_command_count       = 0;
        command_list->bind_render_target_command_count  = 0;
        command_list->bind_shader_command_count         = 0;
        command_list->draw_instance_command_count       = 0;
        command_list->command_count                     = 0;

        c_arena_reset(&command_list->transient_arena);
    }
    renderer_state->command_list_count = 0;
    
#if 0
    vkCmdEndRenderPass(*vulkan_context->render_command_buffer);
#endif
    vkEndCommandBuffer(*vulkan_context->render_command_buffer);

    vkAssert(vkResetFences(vulkan_context->device, 1, vulkan_context->image_render_idle_fence));

    VkPipelineStageFlags stage_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info  = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        // NOTE(Sleepster): Command buffer()s that will be run 
        .commandBufferCount   = 1,
        .pCommandBuffers      = vulkan_context->render_command_buffer,

        // NOTE(Sleepster): The Semaphore(s) that signal when the queue is finished executing the commands
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = vulkan_context->render_complete_semaphore,

        // NOTE(Sleepster): The Semaphore(s) that ensures the operation cannot begin until the image is avaliable 
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = vulkan_context->image_acquired_semaphore,
        .pWaitDstStageMask    = stage_flags
    };
    vkAssert(vkQueueSubmit(vulkan_context->graphics_queue, 1, &submit_info, *vulkan_context->image_render_idle_fence));

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = vulkan_context->render_complete_semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &vulkan_context->swapchain.handle,
        .pImageIndices      = &image_index,
    };
    VkResult result = vkQueuePresentKHR(vulkan_context->graphics_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        vulkan_context->rebuilding_swapchain = true;
    }

    c_arena_reset(&vulkan_context->frame_arena);
    vulkan_context->current_frame_index = (vulkan_context->current_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

/*
=============
vk_backend_handle_window_resize
=============
*/

void
vk_backend_handle_window_resize(vulkan_context_t *vulkan_context, vec2_t window_size)
{
    vulkan_context->last_window_width  = vulkan_context->current_window_width;
    vulkan_context->last_window_height = vulkan_context->current_window_height;

    vulkan_context->current_window_width  = window_size.x;
    vulkan_context->current_window_height = window_size.y;

    ++vulkan_context->window_size_generation;
}
