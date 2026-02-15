/* ========================================================================
   $File: vk_backend_core.cpp $
   $Date: February 12 2026 04:47 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>

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
        if(vk_backend_check_physical_device_support(gpu))
        {
            if(c_dynarray_count(gpu->valid_present_modes)   == 0) continue;
            if(c_dynarray_count(gpu->valid_surface_formats) == 0) continue;

            // NOTE(Sleepster): Graphics queue family 
            for(u32 queue_properties_index = 0;
                queue_properties_index < c_dynarray_count(gpu->queue_family_properties);
                ++queue_properties_index)
            {
                VkQueueFamilyProperties *properties = gpu->queue_family_properties + queue_properties_index;
                if(properties->queueCount == 0)
                {
                    continue;
                }

                if(properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_index = queue_properties_index;
                    break;
                }
            }

            // NOTE(Sleepster): Present queue family 
            for(u32 queue_properties_index = 0;
                queue_properties_index < c_dynarray_count(gpu->queue_family_properties);
                ++queue_properties_index)
            {
                VkQueueFamilyProperties *properties = gpu->queue_family_properties + queue_properties_index;
                if(properties->queueCount == 0)
                {
                    continue;
                }

                VkBool32 supports_presenting = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(gpu->device, queue_properties_index, vulkan_context->render_surface, &supports_presenting);
                if(supports_presenting)
                {
                    present_index = queue_properties_index;
                    break;
                }
            }

            // NOTE(Sleepster): Transfer queue family 
            for(u32 queue_properties_index = 0;
                queue_properties_index < c_dynarray_count(gpu->queue_family_properties);
                ++queue_properties_index)
            {
                VkQueueFamilyProperties *properties = gpu->queue_family_properties + queue_properties_index;
                if(properties->queueCount == 0)
                {
                    continue;
                }

                if(properties->queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    transfer_index = queue_properties_index;
                    break;
                }
            }

            // NOTE(Sleepster): Compute queue family 
            for(u32 queue_properties_index = 0;
                queue_properties_index < c_dynarray_count(gpu->queue_family_properties);
                ++queue_properties_index)
            {
                VkQueueFamilyProperties *properties = gpu->queue_family_properties + queue_properties_index;
                if(properties->queueCount == 0)
                {
                    continue;
                }

                if(properties->queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    compute_index = queue_properties_index;
                    break;
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

            log_info("Device meets queue requirements...\n");
            log_info("GRAPHICS | PRESENT | COMPUTE | TRANSFER | DEVICE NAME\n");
            log_info("   %d     |    %d    |    %d    |     %d    | %s\n",
                     graphics_index, present_index,
                     compute_index,  compute_index,
                     gpu->properties.deviceName);

            VkPhysicalDeviceProperties device_properties = gpu->properties;
            log_info("Device: '%s' selected...\n", device_properties.deviceName);
            switch(device_properties.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                {
                    log_info("Device Type is unknown...\n");
                }break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                {
                    log_info("Device Type is 'Discrete GPU'\n");
                }break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                {
                    log_info("Device Type is 'Integrated GPU'\n");
                }break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                {
                    log_info("Device Type is 'Virtual GPU'\n");
                }break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                {
                    log_info("Device Type is 'CPU'\n");
                }break;
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

    s32 queue_indices[] = {
        vulkan_context->graphics_queue_family_idx,
        vulkan_context->present_queue_family_idx,
        vulkan_context->transfer_queue_family_idx,
        vulkan_context->compute_queue_family_idx
    };

    float32 priority = 1.0f;
    for(u32 queue_index = 0;
        queue_index < ArrayCount(queue_indices);
        ++queue_index)
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue_indices[queue_index];
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
    device_features.sparseBinding     = VK_TRUE;

    VkPhysicalDeviceVulkan11Features device_11_features = {};
    device_11_features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    device_11_features.shaderDrawParameters = true;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType                   =  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext                   = &device_11_features;
    device_create_info.queueCreateInfoCount    =  c_dynarray_count(queue_indices);
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
}

/*
=============
vk_backend_create_semaphores
=============
*/

void
vk_backend_create_semaphores(vulkan_context_t *vulkan_context)
{
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for(u32 frame_index = 0;
        frame_index < MAX_FRAMES_IN_FLIGHT;
        ++frame_index)
    {
        vkAssert(vkCreateSemaphore(vulkan_context->device, &semaphore_create_info, null, vulkan_context->swapchain_image_acquired_semaphores));
        vkAssert(vkCreateSemaphore(vulkan_context->device, &semaphore_create_info, null, vulkan_context->render_complete_semaphores));
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
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool        = vulkan_context->graphics_command_pool;
    command_buffer_allocate_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    vkAssert(vkAllocateCommandBuffers(vulkan_context->device, &command_buffer_allocate_info, vulkan_context->frame_command_buffer));

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    for(u32 fence_index = 0;
        fence_index < MAX_FRAMES_IN_FLIGHT;
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
vk_backend_init_VMA_allocator(vulkan_context_t *vulkan_context)
{
    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags            = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice   = vulkan_context->gpu.device;
    allocator_create_info.device           = vulkan_context->device;
    allocator_create_info.instance         = vulkan_context->instance;

    vmaCreateAllocator(&allocator_create_info, &vulkan_context->vulkan_allocator);
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

    s32 width;
    s32 height;
    Assert(SDL_GetWindowSizeInPixels(vulkan_context->window, &width, &height));
    vulkan_context->window_width  = width;
    vulkan_context->window_height = height;

    VkSwapchainCreateInfoKHR info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = vulkan_context->render_surface,
        .minImageCount    = MAX_FRAMES_IN_FLIGHT,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
        .imageExtent      = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
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
    
    vkAssert(vkGetSwapchainImagesKHR(vulkan_context->device, vulkan_context->swapchain.handle, &num_images, vulkan_context->swapchain_images));
    Expect(num_images > 0, "vkGetSwapchainImagesKHR returned a value of zero...\n");

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
        image->width           = vulkan_context->window_width;
        image->height          = vulkan_context->window_height;

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

        VmaAllocationCreateInfo alloc_create_info = {};
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        alloc_create_info.priority = 1.0f;

        vmaCreateImage(vulkan_context->vulkan_allocator, &info, &alloc_create_info, &image->handle, &image->gpu_memory, null);
#if 0
        // TODO(Sleepster): The GPU memory allocator
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(vulkan_context->device, image->handle, &memory_requirements);
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
vk_backend_init
=============
*/

// TODO(Sleepster): 
// - Vulkan GPU memory allocator
// - Image handler and manager
// - Staging buffer manager
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

    // NOTE(Sleepster): Create the image acquisition and rendering completion semaphore objects 
    vk_backend_create_semaphores(vulkan_context);

    // NOTE(Sleepster): Create the command pools for our context's buffers
    vk_backend_create_command_pools(vulkan_context);

    // NOTE(Sleepster): Create the comamnd buffers for the rendering in the engine 
    vk_backend_create_command_buffers(vulkan_context);

    // TODO(Sleepster): Define the Vulkan allocator we will use here, create a macro to toggle between ours and VMA
    vk_backend_init_VMA_allocator(vulkan_context);

    // NOTE(Sleepster): Generate the swapchain, it's images, and the views for those images. We do not create the depth buffer. 
    vk_backend_swapchain_create(vulkan_context);

    // NOTE(Sleepster): Generate the program's depth buffer 
    vk_backend_create_depth_buffer(vulkan_context);
}
