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
#include <c_file_api.h>
#include <c_memory_arena.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <r_vulkan.h>

struct vertex_t
{
    vec4_t vPosition;
    vec4_t vColor;
};

/////////////////////////
// VULKAN UTILITIES
/////////////////////////

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

const char* 
r_vulkan_result_string(VkResult result, bool8 get_extended) 
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
            return !get_extended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
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

bool8 
r_vulkan_result_is_success(VkResult result) 
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

////////////////////////////
// VULKAN BUFFERS 
////////////////////////////

void
r_vulkan_buffer_bind(vulkan_render_context_t *render_context,
                     vulkan_buffer_data_t    *buffer,
                     u64                      offset)
{
    Assert(buffer->is_valid);
    VkAssert(vkBindBufferMemory(render_context->rendering_device.logical_device, 
                                buffer->handle,
                                buffer->device_memory, 
                                offset));
}

vulkan_buffer_data_t
r_vulkan_buffer_create(vulkan_render_context_t *render_context,
                       u64                      buffer_size,
                       VkBufferUsageFlagBits    usage_flags,
                       u32                      memory_flags,
                       bool8                    bind_on_create)
{
    vulkan_buffer_data_t result  = {};
    result.buffer_size           = buffer_size;
    result.usage_flags           = usage_flags;
    result.memory_property_flags = memory_flags;
    VkBufferCreateInfo buffer_create_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = buffer_size,
        .usage       = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkAssert(vkCreateBuffer(render_context->rendering_device.logical_device,
                           &buffer_create_info,
                            render_context->allocators,
                           &result.handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(render_context->rendering_device.logical_device, 
                                  result.handle, 
                                  &memory_requirements);

    result.memory_index = r_vulkan_find_memory_index(render_context, 
                                                     memory_requirements.memoryTypeBits, 
                                                     result.memory_property_flags);
    Assert(result.memory_index != -1);
    VkMemoryAllocateInfo allocate_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = (u32)result.memory_index,
    };
    VkResult success = vkAllocateMemory(render_context->rendering_device.logical_device,
                                       &allocate_info,
                                        render_context->allocators,
                                       &result.device_memory);
    if(success != VK_SUCCESS)
    {
        log_fatal("We have failed to create a buffer of size: '%d'... Error is: '%s'...\n", 
                  buffer_size, r_vulkan_result_string(success, true));
    }
    else
    {
        result.is_valid = true;
    }

    if(bind_on_create)
    {
        Assert(result.is_valid == true);
        r_vulkan_buffer_bind(render_context, &result, 0);
    }

    return(result);
}

void
r_vulkan_buffer_destroy(vulkan_render_context_t *render_context,
                        vulkan_buffer_data_t    *buffer)
{
    Assert(buffer);
    Assert(buffer->is_valid);

    if(buffer->device_memory)
    {
        vkFreeMemory(render_context->rendering_device.logical_device,
                     buffer->device_memory,
                     render_context->allocators);
    }
    if(buffer->handle)
    {
        vkDestroyBuffer(render_context->rendering_device.logical_device,
                        buffer->handle,
                        render_context->allocators);
    }

    ZeroStruct(*buffer);
}

void*
r_vulkan_buffer_map_memory(vulkan_render_context_t *render_context,
                           vulkan_buffer_data_t    *buffer,
                           u64                      offset,
                           u64                      map_size,
                           u32                      flags)
{
    Assert(buffer->is_valid);
    Assert(buffer->device_memory);
    Assert(buffer->buffer_size != 0);

    void *result = null;
    VkAssert(vkMapMemory(render_context->rendering_device.logical_device,
                         buffer->device_memory,
                         offset,
                         map_size,
                         flags,
                        &result));
    buffer->is_mapped = true;

    return(result);
}

void
r_vulkan_buffer_unmap_memory(vulkan_render_context_t *render_context,
                             vulkan_buffer_data_t    *buffer,
                             void                    *data)
{
    Assert(buffer->is_mapped);
    vkUnmapMemory(render_context->rendering_device.logical_device,
                  buffer->device_memory);
    buffer->is_mapped = false;
}

void
r_vulkan_buffer_copy_buffer(vulkan_render_context_t *render_context,
                            vulkan_buffer_data_t    *src_buffer,
                            u64                      src_offset,
                            vulkan_buffer_data_t    *dst_buffer,
                            u64                      copy_size,
                            u64                      dst_offset,
                            VkFence                  fence,
                            VkQueue                  queue,
                            VkCommandPool            command_pool)
{
    vkQueueWaitIdle(queue);
    vulkan_command_buffer_data_t temp_buffer = r_vulkan_command_buffer_acquire_scratch_buffer(render_context, 
                                                                                              command_pool);
    VkBufferCopy copy_region = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size      = copy_size
    };
    vkCmdCopyBuffer(temp_buffer.handle, src_buffer->handle, dst_buffer->handle, 1, &copy_region);
    r_vulkan_command_buffer_dispatch_scratch_buffer(render_context, &temp_buffer, command_pool, queue);
}

void
r_vulkan_buffer_copy_data(vulkan_render_context_t *render_context,
                          vulkan_buffer_data_t    *buffer,
                          void                    *data,
                          u64                      data_size,
                          u64                      offset,
                          u32                      flags)
{
    void *data_ptr = null;
    VkAssert(vkMapMemory(render_context->rendering_device.logical_device, 
                         buffer->device_memory, 
                         offset, 
                         data_size, 
                         flags, 
                        &data_ptr));

    memcpy(data_ptr, data, data_size);

    vkUnmapMemory(render_context->rendering_device.logical_device,
                  buffer->device_memory);
}

void
r_vulkan_buffer_resize(vulkan_render_context_t *render_context,
                       vulkan_buffer_data_t    *buffer,
                       u64                      new_buffer_size,
                       VkQueue                  queue,
                       VkCommandPool            pool)
{
    vulkan_buffer_data_t new_buffer = r_vulkan_buffer_create(render_context,
                                                             new_buffer_size,
                                                             buffer->usage_flags,
                                                             buffer->memory_property_flags,
                                                             true);
    r_vulkan_buffer_copy_buffer(render_context, 
                                &new_buffer, 
                                0, 
                                buffer, 
                                buffer->buffer_size, 
                                0, 
                                null, 
                                queue,
                                pool);

    vkDeviceWaitIdle(render_context->rendering_device.logical_device);
    r_vulkan_buffer_destroy(render_context, buffer);

    *buffer = new_buffer;
}

// TODO(Sleepster): TEMPORARY
void
r_vulkan_buffer_upload(vulkan_render_context_t *render_context, 
                       vulkan_buffer_data_t    *buffer,
                       void                    *data,
                       u64                      upload_size,
                       u64                      offset,
                       VkFence                  fence,
                       VkCommandPool            command_pool,
                       VkQueue                  queue)
{
    VkBufferUsageFlagBits staging_buffer_flags = (VkBufferUsageFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vulkan_buffer_data_t staging_buffer = r_vulkan_buffer_create(render_context,
                                                                 upload_size,
                                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                 staging_buffer_flags,
                                                                 true);
    Assert(staging_buffer.is_valid);
    Assert(staging_buffer.handle != null);

    r_vulkan_buffer_copy_data(render_context, &staging_buffer, data, upload_size, offset, 0);
    r_vulkan_buffer_copy_buffer(render_context, 
                               &staging_buffer, 
                                0, 
                                buffer, 
                                upload_size, 
                                offset, 
                                fence, 
                                queue, 
                                command_pool);
    r_vulkan_buffer_destroy(render_context, &staging_buffer);
}

////////////////////////////
// VULKAN PIPELINE 
////////////////////////////

// TODO(Sleepster): Dynamic state
vulkan_pipeline_data_t
r_vulkan_pipeline_create(vulkan_render_context_t           *render_context, 
                         vulkan_renderpass_data_t          *renderpass,
                         VkVertexInputAttributeDescription *vertex_attributes,
                         u32                                attribute_count,
                         VkDescriptorSetLayout             *descriptor_sets,
                         u32                                descriptor_set_count,
                         VkPipelineShaderStageCreateInfo   *shader_stages,
                         u32                                stage_count,
                         VkViewport                         viewport,
                         VkRect2D                           scissor,
                         bool8                              wireframe)
{
    vulkan_pipeline_data_t result;

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pViewports    = &viewport,
        .viewportCount = 1,
        .pScissors     = &scissor,
        .scissorCount  = 1
    };

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
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_data = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates    = dynamic_state,
        .dynamicStateCount = ArrayCount(dynamic_state)
    };

    VkVertexInputBindingDescription vertex_input_desc = {
        .binding   = 0,
        .stride    = sizeof(vertex_t),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   =  1,
        .pVertexBindingDescriptions      = &vertex_input_desc,
        .vertexAttributeDescriptionCount =  attribute_count,
        .pVertexAttributeDescriptions    =  vertex_attributes,
    };

    VkPipelineInputAssemblyStateCreateInfo assembly_state = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = descriptor_set_count,
        .pSetLayouts    = descriptor_sets
    };

    VkAssert(vkCreatePipelineLayout(render_context->rendering_device.logical_device,
                                   &pipeline_layout_info,
                                    render_context->allocators,
                                   &result.layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType               =  VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages             =  shader_stages,
        .stageCount          =  stage_count,
        .pVertexInputState   = &vertex_input_state,
        .pInputAssemblyState = &assembly_state,
        .pViewportState      = &viewport_info,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState   = &multisampling_state,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blend_state,
        .pDynamicState       = &dynamic_state_data,
        .layout              =  result.layout,
        .renderPass          =  renderpass->handle,
        .subpass             =  0,
        .basePipelineHandle  =  VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    VkResult success = vkCreateGraphicsPipelines(render_context->rendering_device.logical_device,
                                                 VK_NULL_HANDLE,
                                                 1,
                                                &pipeline_create_info,
                                                 render_context->allocators,
                                                &result.handle);
    if(!r_vulkan_result_is_success(success))
    {
        log_fatal("Failure to create the Vulkan Graphics Pipeline... Error: '%s'...\n", r_vulkan_result_string(success, true));
    }
    else
    {
        log_info("Vulkan Graphics Pipeline created...\n");
    }

    return(result);
}

void
r_vulkan_pipeline_destroy(vulkan_render_context_t *render_context,
                          vulkan_pipeline_data_t  *pipeline)
{
    Assert(pipeline);

    if(pipeline->handle)
    {
        vkDestroyPipeline(render_context->rendering_device.logical_device, 
                          pipeline->handle,
                          render_context->allocators);
    }

    if(pipeline->layout)
    {
        vkDestroyPipelineLayout(render_context->rendering_device.logical_device, 
                          pipeline->layout,
                          render_context->allocators);
    }

    pipeline->handle = null;
    pipeline->layout = null;
}

void
r_vulkan_pipeline_bind(vulkan_command_buffer_data_t *command_buffer,
                       VkPipelineBindPoint           bind_point,
                       vulkan_pipeline_data_t       *pipeline)
{
    Assert(command_buffer->handle);
    Assert(command_buffer->state == VKCBS_RECORDING || 
           command_buffer->state == VKCBS_WITHIN_RENDERPASS);
    Assert(pipeline->handle);

    vkCmdBindPipeline(command_buffer->handle, bind_point, pipeline->handle);
}

////////////////////////////
// VULKAN SHADER MODULES 
////////////////////////////
VkDescriptorType 
r_vulkan_convert_spv_reflect_descriptor_type(SpvReflectDescriptorType spv_type)
{
    switch(spv_type)
    {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkShaderStageFlags
r_vulkan_convert_spv_shader_stage(SpvReflectShaderStageFlagBits spv_stage)
{
    VkShaderStageFlags result = 0;
    
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
        result |= VK_SHADER_STAGE_VERTEX_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
        result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
        result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT)
        result |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT)
        result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT)
        result |= VK_SHADER_STAGE_COMPUTE_BIT;
    
    return(result);
}

void
r_vulkan_shader_stage_create(vulkan_render_context_t    *render_context, 
                             string_t                    shader_source, 
                             SpvReflectEntryPoint       *entry_point, 
                             vulkan_shader_stage_info_t *stage)
{
    stage->type        = (VkShaderStageFlagBits)r_vulkan_convert_spv_shader_stage(entry_point->shader_stage);
    stage->entry_point = entry_point->name;
    stage->module_create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_source.count,
        .pCode    = (u32*)shader_source.data,
    };

    VkAssert(vkCreateShaderModule(render_context->rendering_device.logical_device,
                                 &stage->module_create_info,
                                  render_context->allocators,
                                 &stage->handle));

    stage->shader_stage_create_info = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = stage->type,
        .module = stage->handle,
        .pName  = entry_point->name,
    };
}
    
struct spv_vulkan_type_map 
{
    SpvReflectDescriptorType spv_type;
    VkDescriptorType         vk_type;
};

global_variable spv_vulkan_type_map type_map[] = {
    {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER,                VK_DESCRIPTOR_TYPE_SAMPLER               },
    {SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         },
    {SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        },
    {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER,         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        },
    {SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE,          VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         },
    {SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
};

vulkan_shader_data_t
r_vulkan_shader_create(vulkan_render_context_t *render_context, string_t filepath)
{
    vulkan_shader_data_t result = {};
    result.arena = c_arena_create(MB(10));
    string_t shader_source = c_file_read_entirety(filepath, &result.arena);

    // TODO(Sleepster): TEMPORARY
    
    float32 framebuffer_width  = (float32)render_context->framebuffer_width;
    float32 framebuffer_height = (float32)render_context->framebuffer_height;

    mat4_t view_matrix       = mat4_identity();
    mat4_t projection_matrix = mat4_RHGL_ortho(framebuffer_width  * -0.5f,
                                               framebuffer_width  *  0.5f,
                                               framebuffer_height * -0.5f,
                                               framebuffer_height *  0.5f,
                                              -1.0f,
                                               1.0f);

    result.camera_matrices = {
        .view_matrix       = view_matrix,
        .projection_matrix = projection_matrix,
    };

    // TODO(Sleepster): TEMPORARY

    SpvReflectShaderModule *module = &result.spv_reflect_module;
    SpvReflectResult error = spvReflectCreateShaderModule(shader_source.count, shader_source.data, module);
    if(error != SPV_REFLECT_RESULT_SUCCESS)
    {
        printf("Failure to create a spvReflectCreateShaderModule for shader file: '%s'...\n", C_STR(filepath));
    }

    log_info("Shader: '%s' has '%u' entry points...\n", C_STR(filepath), module->entry_point_count);

    VkDescriptorPoolSize descriptor_pool_type_info[16] = {};
    u32                  used_pool_indices     = 0;
    u32                  total_descriptor_sets = 0;

    for(u32 entry_point_index = 0;
        entry_point_index < module->entry_point_count;
        ++entry_point_index)
    {
        SpvReflectEntryPoint       *entry_point = module->entry_points + entry_point_index;
        vulkan_shader_stage_info_t *stage       = result.stages + entry_point_index;

        VkShaderStageFlags current_stage = r_vulkan_convert_spv_shader_stage(entry_point->shader_stage);

        const char *name = entry_point->name;
        log_trace("Entry Point %d: '%s'...\n", entry_point_index, name);

        for(u32 set_index = 0;
            set_index < entry_point->descriptor_set_count;
            ++set_index)
        {
            SpvReflectDescriptorSet             *current_set    = entry_point->descriptor_sets + set_index;
            vulkan_shader_descriptor_set_info_t *set_info       = result.set_info + set_index;
            VkDescriptorSetLayout               *current_layout = result.layouts  + set_index;
            u64 set_buffer_size = 0;

            ZeroStruct(*set_info);

            total_descriptor_sets += 1;

            log_trace("Shader Descriptor Set: '%u' has '%u' bindings...\n",
                     current_set->set, current_set->binding_count);

            set_info->bindings = c_arena_push_array(&result.arena, VkDescriptorSetLayoutBinding, current_set->binding_count);
            set_info->binding_count = current_set->binding_count;

            VkDescriptorSetLayoutBinding *set_bindings = set_info->bindings;
            for(u32 binding_index = 0;
                binding_index < current_set->binding_count;
                ++binding_index)
            {
                SpvReflectDescriptorBinding *binding = current_set->bindings[binding_index];
                log_trace("Binding of index: '%u' with name: '%s' of type: '%d'...\n", 
                         binding->binding, binding->name, binding->descriptor_type);

                set_buffer_size += binding->block.padded_size;

                result.type_counts[binding->descriptor_type] += 1;
                set_bindings[binding_index] = {
                    .binding            = binding->binding,
                    .descriptorType     = r_vulkan_convert_spv_reflect_descriptor_type(binding->descriptor_type),
                    .descriptorCount    = binding->count,
                    .stageFlags         = current_stage,
                    .pImmutableSamplers = null,
                };
            }

            VkDescriptorSetLayoutCreateInfo layout_create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = current_set->binding_count,
                .pBindings    = set_bindings,
            };
            VkAssert(vkCreateDescriptorSetLayout(render_context->rendering_device.logical_device,
                                                &layout_create_info,
                                                 null,
                                                 current_layout));
            for(u32 type_index = 0;
                type_index < ArrayCount(type_map);
                ++type_index)
            {
                u32 set_type_count = result.type_counts[type_map[type_index].spv_type];
                if(set_type_count > 0)
                {
                    // TODO(Sleepster): this is also hardcoded for triple buffering... 
                    // Maybe bad idea...
                    descriptor_pool_type_info[used_pool_indices++] = {
                        .type            = type_map[type_index].vk_type,
                        .descriptorCount = set_type_count * 3 
                    };
                }
            }

            // NOTE(Sleepster): Aligning with a value of 256 because nvidia cards sometimes 
            // require uniform buffers to be at least 256 bytes...
            set_info->buffer = r_vulkan_buffer_create(render_context, 
                                                Align(set_buffer_size, 256), 
                               (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
                                                  (u32)VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                      true);
            Assert(set_info->buffer.handle);
        }

        r_vulkan_shader_stage_create(render_context, shader_source, entry_point, stage);
    }
    
    if(used_pool_indices == 0)
    {
        log_warning("No desciptor pool sizes set...\n");
        descriptor_pool_type_info[0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        };
        used_pool_indices = 1;
    }
    result.stage_count = module->entry_point_count;

    // NOTE(Sleepster): 3 frames in flight 
    u32 allocated_descriptor_sets = total_descriptor_sets * render_context->swapchain.image_count;
    VkDescriptorPoolCreateInfo pool_create_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,

        // TODO(Sleepster): Do we care???          
        //https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorPoolCreateFlagBits.html
        //.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        
        .maxSets       = allocated_descriptor_sets,
        .poolSizeCount = used_pool_indices,
        .pPoolSizes    = descriptor_pool_type_info
    };   
    VkAssert(vkCreateDescriptorPool(render_context->rendering_device.logical_device, 
                                   &pool_create_info, 
                                    render_context->allocators, 
                                   &result.primary_pool));

    for(u32 set_index = 0;
        set_index < total_descriptor_sets;
        ++set_index)
    {
        vulkan_shader_descriptor_set_info_t *info   = result.set_info + set_index;
        VkDescriptorSetLayout                layout = result.layouts[set_index];

        VkDescriptorSetLayout layout_info[3] = {
            layout,
            layout,
            layout
        };

        // TODO(Sleepster): maybe not hard code the count...
        VkDescriptorSetAllocateInfo allocation_info = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = result.primary_pool,
            .descriptorSetCount = 3,
            .pSetLayouts        = layout_info
        };
        VkAssert(vkAllocateDescriptorSets(render_context->rendering_device.logical_device,
                                         &allocation_info,
                                          info->sets));
    }


    const u32 attribute_count = 2;
    VkVertexInputAttributeDescription attributes[attribute_count] = {};

    VkFormat attribute_formats[attribute_count] = {
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT
    };

    u64 attribute_sizes[attribute_count] = {
        sizeof(vec4_t),
        sizeof(vec4_t)
    };

    u32 offset = 0;
    for(u32 index = 0;
        index < attribute_count;
        ++index)
    {
        VkVertexInputAttributeDescription *attrib = attributes + index;
        attrib->binding  = 0;
        attrib->format   = attribute_formats[index];
        attrib->location = index;
        attrib->offset   = offset;

        offset += attribute_sizes[index];
    }

    // TODO(Sleepster): We will explode if we ever try to support more than 2 shaders in a file... Oh well!
    VkPipelineShaderStageCreateInfo stage_create_infos[2] = {};
    for(u32 index = 0;
        index < ArrayCount(stage_create_infos);
        ++index)
    {
        stage_create_infos[index] = result.stages[index].shader_stage_create_info;
    }

    VkViewport viewport = {
        .x        =  (float32)0.0f,
        .y        =  (float32)render_context->framebuffer_height,
        .width    =  (float32)render_context->framebuffer_width,
        .height   = -(float32)render_context->framebuffer_height,
        .minDepth =  0.0f,
        .maxDepth =  1.0f
    };

    VkRect2D scissor = {
        .extent = {
            .width  = render_context->framebuffer_width,
            .height = render_context->framebuffer_height
        },
    };
    result.used_descriptor_set_count = total_descriptor_sets;
    result.pipeline = r_vulkan_pipeline_create(render_context,
                                              &render_context->main_renderpass,
                                               attributes,
                                               attribute_count,
                                               result.layouts,
                                               total_descriptor_sets,
                                               stage_create_infos,
                                               2,
                                               viewport,
                                               scissor,
                                               false);
    if(!result.pipeline.handle)
    {
        log_fatal("We have failed to create our pipeline...\n");
    }

    return(result);
}

void
r_vulkan_shader_destroy(vulkan_render_context_t *render_context, vulkan_shader_data_t *shader)
{
    spvReflectDestroyShaderModule(&shader->spv_reflect_module);
    for(u32 module_index = 0;
        module_index < shader->stage_count;
        ++module_index)
    {
        vulkan_shader_stage_info_t *stage = shader->stages + module_index;
        vkDestroyShaderModule(render_context->rendering_device.logical_device, 
                              stage->handle, 
                              render_context->allocators);
    }
    
    // TODO(Sleepster): hardcoded for triple buffering again... 
    for(u32 set_index = 0;
        set_index < 3;
        ++set_index)
    {
        vulkan_shader_descriptor_set_info_t *info = shader->set_info + set_index;
        r_vulkan_buffer_destroy(render_context, &info->buffer);

        // TODO(Sleepster): Probably not a good idea, but we just happen to ALSO hard 
        // code this to support triple buffering 
        vkDestroyDescriptorSetLayout(render_context->rendering_device.logical_device,
                                     shader->layouts[set_index],
                                     render_context->allocators);
    }
    vkDestroyDescriptorPool(render_context->rendering_device.logical_device,
                            shader->primary_pool,
                            render_context->allocators);

    r_vulkan_pipeline_destroy(render_context, &shader->pipeline);
    c_arena_destroy(&shader->arena);
}

void
r_vulkan_shader_bind(vulkan_render_context_t *render_context, vulkan_shader_data_t *shader)
{
    // TODO(Sleepster): Graphics is hard coded... 
    r_vulkan_pipeline_bind(&render_context->graphics_command_buffers[render_context->current_image_index],
                           VK_PIPELINE_BIND_POINT_GRAPHICS,
                           &shader->pipeline);
}

// TODO(Sleepster): Ability to control what kind of descriptor sets are updated:
// r_vulkan_shader_update_static_sets
// r_vulkan_shader_update_perdraw_sets
// r_vulkan_shader_update_instance_sets
// r_vulkan_shader_update_descriptor_sets
void
r_vulkan_shader_update_descriptor_sets(vulkan_render_context_t *render_context,
                                       vulkan_shader_data_t    *shader)
{
    Assert(shader->used_descriptor_set_count <=  SDS_Count);

    u32 current_image_index = render_context->current_image_index;
    VkCommandBuffer command_buffer = render_context->graphics_command_buffers[current_image_index].handle;
    
    for(u32 descriptor_index = SDS_Static;
        descriptor_index < shader->used_descriptor_set_count;
        ++descriptor_index)
    {
        vulkan_shader_descriptor_set_info_t *info = shader->set_info + descriptor_index;
        VkDescriptorSet current_set = info->sets[current_image_index];  

        // TODO(Sleepster): Graphics is hard coded... 
        vkCmdBindDescriptorSets(command_buffer, 
                                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                shader->pipeline.layout, 
                                0, 
                                1,
                               &current_set,
                                0,
                                null);
        // TODO(Sleepster): Transfer queue?
        r_vulkan_buffer_upload(render_context, 
                              &info->buffer, 
                              &shader->camera_matrices, 
                               info->buffer.buffer_size, 
                               0, 
                               null, 
                               render_context->rendering_device.graphics_command_pool, 
                               render_context->rendering_device.graphics_queue);

        VkDescriptorBufferInfo buffer_info = {
            .buffer = info->buffer.handle,
            .offset = 0,
            .range  = info->buffer.buffer_size
        };

        VkWriteDescriptorSet *writes = c_arena_push_array(&render_context->frame_arena, VkWriteDescriptorSet, info->binding_count);
        for(u32 binding_index = 0;
            binding_index < info->binding_count;
            ++binding_index)
        {
            VkDescriptorSetLayoutBinding *binding = info->bindings + binding_index;
            writes[binding_index] = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = current_set,
                .dstBinding      = binding->binding,
                .dstArrayElement = 0,
                .descriptorType  = binding->descriptorType,
                .descriptorCount = binding->descriptorCount,
                .pBufferInfo     = &buffer_info
            };
        }

        vkUpdateDescriptorSets(render_context->rendering_device.logical_device,
                               info->binding_count,
                               writes,
                               0,
                               null);
    }
}

////////////////////////////
// VULKAN FRAMEBUFFERS 
////////////////////////////

vulkan_framebuffer_data_t
r_vulkan_framebuffer_create(vulkan_render_context_t  *render_context,
                            vulkan_swapchain_data_t  *swapchain,
                            vulkan_renderpass_data_t *renderpass,
                            u32                       width,
                            u32                       height,
                            u32                       attachment_count,
                            VkImageView              *attachments)
{
    log_trace("Framebuffer size is: '%d'... %d...\n", width, height);

    // NOTE(Sleepster): We're making a copy of the attachment data so that if it ceases to exist, we don't explode. 
    vulkan_framebuffer_data_t result = {};
    result.renderpass       = renderpass;
    result.attachment_count = attachment_count;
    result.attachments      = c_arena_push_array(&swapchain->arena, VkImageView, attachment_count);
    memcpy(result.attachments, attachments, sizeof(VkImageView) * attachment_count);

    VkFramebufferCreateInfo framebuffer_create_info = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderpass->handle,
        .attachmentCount = attachment_count,
        .pAttachments    = result.attachments,
        .width           = width,
        .height          = height,
        .layers          = 1
    };
    VkAssert(vkCreateFramebuffer(render_context->rendering_device.logical_device, 
                                &framebuffer_create_info, 
                                 render_context->allocators, 
                                &result.handle));

    return(result);
}

void
r_vulkan_framebuffer_destroy(vulkan_render_context_t   *render_context,
                             vulkan_framebuffer_data_t *framebuffer)
{
    vkDestroyFramebuffer(render_context->rendering_device.logical_device,
                         framebuffer->handle,
                         render_context->allocators);

    framebuffer->handle           = null;
    framebuffer->renderpass       = null;
    framebuffer->attachments      = null;

    framebuffer->attachment_count = 0;
}

////////////////////////////
// VULKAN FENCES 
////////////////////////////

vulkan_fence_t 
r_vulkan_fence_create(vulkan_render_context_t *render_context,
                      bool8                    start_signaled)
{
    vulkan_fence_t result = {};
    result.signaled = start_signaled;

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = start_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (u32)0 
    };

    VkAssert(vkCreateFence(render_context->rendering_device.logical_device, 
                 &fence_create_info, 
                  render_context->allocators,
                 &result.handle));

    return(result);
}

void
r_vulkan_fence_destroy(vulkan_render_context_t *render_context,
                       vulkan_fence_t          *fence)
{
    Assert(fence->handle);

    vkDestroyFence(render_context->rendering_device.logical_device,
                   fence->handle,
                   render_context->allocators);
    fence->handle   = null;
    fence->signaled = false;
}

bool8
r_vulkan_fence_wait(vulkan_render_context_t  *render_context,
                    vulkan_fence_t           *fence,
                    u64                       timeout_ns)
{
    bool8 result = true;
    if(!fence->signaled)
    {
        VkResult wait_result = vkWaitForFences(render_context->rendering_device.logical_device,
                                               1,
                                              &fence->handle,
                                               true,
                                               timeout_ns);
        switch(wait_result)
        {
            case VK_SUCCESS:
            {
                fence->signaled = true;

                return(result);
            }break;
            case VK_TIMEOUT:
            {
                log_warning("Timed out waiting for fence with wait time of: '%llu'...\n", timeout_ns);
            }break;
            case VK_ERROR_DEVICE_LOST:
            {
                log_fatal("We have lost the device... VK_ERROR_DEVICE_LOST\n");
            }break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
            {
                log_fatal("We have run out of host memory...\n");
            }break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            {
                log_fatal("We have run out of device memory...\n");
            }
            default:
            {
                log_fatal("An unknown error has occurred...\n");
            }break;
        }
    }

    return(result);
}

void 
r_vulkan_fence_reset(vulkan_render_context_t *render_context,
                     vulkan_fence            *fence)
{
    if(fence->signaled)
    {
        VkAssert(vkResetFences(render_context->rendering_device.logical_device, 1, &fence->handle));
        fence->signaled = false;
    }
    else
    {
        log_warning("Resetting a fence that is unsignaled...\n");
    }
}

////////////////////////////
// VULKAN IMAGE 
////////////////////////////

VkImageView
r_vulkan_image_view_create(vulkan_render_context_t *render_context,
                           VkFormat                 format, 
                           vulkan_image_data_t     *image_data,
                           u32                      view_aspect_flags)
{
    VkImageView result = {};
    VkImageViewCreateInfo view_create_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = image_data->handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format   = format,
        .subresourceRange = {
            .aspectMask     = view_aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        },
    };
    VkAssert(vkCreateImageView(render_context->rendering_device.logical_device, &view_create_info, render_context->allocators, &result));

    return(result);
}

void
r_vulkan_image_destroy(vulkan_render_context_t *render_context,
                       vulkan_image_data_t     *image_data)
{
    if(image_data->view)
    {
        vkDestroyImageView(render_context->rendering_device.logical_device, 
                           image_data->view, 
                           render_context->allocators);
        image_data->view = 0;
    }
    if(image_data->memory)
    {
        vkFreeMemory(render_context->rendering_device.logical_device, 
                     image_data->memory, 
                     render_context->allocators);
        image_data->memory = 0;


        log_info("Freeing GPU memory for image...\n");
    }
    if(image_data->handle)
    {
        vkDestroyImage(render_context->rendering_device.logical_device, 
                       image_data->handle, 
                       render_context->allocators);
        image_data->handle = 0;
    }
}

vulkan_image_data_t
r_vulkan_image_create(vulkan_render_context_t *render_context,
                      VkImageType              image_type,
                      u32                      width,
                      u32                      height,
                      VkFormat                 format,
                      VkImageTiling            tiling,
                      VkImageUsageFlags        usage_flags,
                      VkMemoryPropertyFlags    memory_flags,
                      VkImageAspectFlags       view_aspect_flags,
                      u32                      mip_levels,
                      bool32                   create_view)
{
    Assert(width < 4096);
    log_info("Creating image: %ux%u, format=%d\n", width, height, format);
 
    vulkan_image_data_t result = {};
    result.width  = width;
    result.height = height;

    VkImageCreateInfo image_create_info = {
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = {
            .width  = width,
            .height = height,
            .depth  = 1
        },
        .mipLevels     = mip_levels,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage_flags,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE
    };

    VkAssert(vkCreateImage(render_context->rendering_device.logical_device, 
                          &image_create_info, 
                           render_context->allocators, 
                          &result.handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(render_context->rendering_device.logical_device, result.handle, &memory_requirements);
    s32 memory_index = r_vulkan_find_memory_index(render_context, memory_requirements.memoryTypeBits, memory_flags);
    if(memory_index == -1)
    {
        log_error("Requested memory type could not be found... Image is invalid...\n");
    }

    VkMemoryAllocateInfo memory_allocation_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memory_requirements.size,
        .memoryTypeIndex = (u32)memory_index
    };
    VkResult code = vkAllocateMemory(render_context->rendering_device.logical_device, &memory_allocation_info, render_context->allocators, &result.memory);
    if(code != VK_SUCCESS)
    {
        log_fatal("Failure to allocate image memory: '%s'...\n", r_vulkan_result_string(code, true));
        Assert(false);
    }
    code = vkBindImageMemory(render_context->rendering_device.logical_device, result.handle, result.memory, 0); // final parameter is an offset
    if(code != VK_SUCCESS)
    {
        log_fatal("Failure to allocate image memory: '%s'...\n", r_vulkan_result_string(code, true));
        Assert(false);
    }

    if(create_view)       
    {
        result.view = r_vulkan_image_view_create(render_context, format, &result, view_aspect_flags);
    }

    return(result);
}

////////////////////////////
// VULKAN COMMAND BUFFER 
////////////////////////////

vulkan_command_buffer_data_t
r_vulkan_command_buffer_acquire(vulkan_render_context_t *render_context,
                                VkCommandPool            command_pool,
                                bool8                    is_primary_buffer)
{
    vulkan_command_buffer_data_t result = {};
    result.is_primary_buffer = is_primary_buffer;

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = command_pool,
        .commandBufferCount = 1,
        .level              = is_primary_buffer ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY
    };
    // NOTE(Sleepster): This RETURNS the command buffer, so I don't think this is needed... 
    VkAssert(vkAllocateCommandBuffers(render_context->rendering_device.logical_device, 
                                     &command_buffer_allocate_info, 
                                     &result.handle));
    result.state = VKCBS_READY;

    return(result);
}

void
r_vulkan_command_buffer_release(vulkan_render_context_t      *render_context,
                                vulkan_command_buffer_data_t *command_buffer,
                                VkCommandPool                 command_pool)
{
    Assert(command_buffer->handle);
    Assert(command_buffer->state != VKCBS_INVALID);

    vkFreeCommandBuffers(render_context->rendering_device.logical_device, 
                         command_pool, 
                         1, 
                        &command_buffer->handle);
    command_buffer->handle = null;
    command_buffer->state  = VKCBS_INVALID;
}

void
r_vulkan_command_buffer_begin(vulkan_command_buffer_data_t *command_buffer, 
                              bool8                         single_use, 
                              bool8                         renderpass_continue, 
                              bool8                         simultaneous_usage)
{
    Assert(command_buffer->state == VKCBS_READY);

    // NOTE(Sleepster): 
    // https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkCommandBufferUsageFlagBits.html 
    u32 begin_flags = 0;
    if(single_use)
    {
        begin_flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    if(renderpass_continue)
    {
        begin_flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    if(simultaneous_usage)
    {
        begin_flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = begin_flags
    };

    VkAssert(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
    command_buffer->state = VKCBS_RECORDING;
}

void 
r_vulkan_command_buffer_end(vulkan_command_buffer_data_t *command_buffer)
{
    Assert(command_buffer->state == VKCBS_RECORDING || 
           command_buffer->state == VKCBS_WITHIN_RENDERPASS);

    vkEndCommandBuffer(command_buffer->handle);
    command_buffer->state = VKCBS_RECORDING_ENDED;
}

void
r_vulkan_command_buffer_submit(vulkan_command_buffer_data_t *command_buffer)
{
    Assert(command_buffer->state == VKCBS_RECORDING_ENDED);
    command_buffer->state = VKCBS_SUBMITTED;
}

void
r_vulkan_command_buffer_reset(vulkan_command_buffer_data_t *command_buffer)
{
    command_buffer->state = VKCBS_READY;
}

vulkan_command_buffer_data_t
r_vulkan_command_buffer_acquire_scratch_buffer(vulkan_render_context_t *render_context, 
                                               VkCommandPool            command_pool)
{
    vulkan_command_buffer_data_t result;
    result = r_vulkan_command_buffer_acquire(render_context, command_pool, true);
    r_vulkan_command_buffer_begin(&result, true, false, false);

    return(result);
}

void
r_vulkan_command_buffer_dispatch_scratch_buffer(vulkan_render_context_t      *render_context,
                                                vulkan_command_buffer_data_t *command_buffer,
                                                VkCommandPool                 command_pool,
                                                VkQueue                       queue)
{
    r_vulkan_command_buffer_end(command_buffer);
    VkSubmitInfo submit_info = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &command_buffer->handle
    };

    // TODO(Sleepster): No fence???
    VkAssert(vkQueueSubmit(queue, 1, &submit_info, 0));

    // NOTE(Sleepster): Hack since we aren't using a fence... 
    VkAssert(vkQueueWaitIdle(queue));
    r_vulkan_command_buffer_release(render_context, command_buffer, command_pool);
}

void
r_vulkan_initialize_graphics_command_buffers(vulkan_render_context_t *render_context)
{
    Assert(render_context->graphics_command_buffers);
    for(u32 frame_index = 0;
        frame_index < render_context->swapchain.image_count;
        ++frame_index)
    {
        vulkan_command_buffer_data_t *command_buffer = render_context->graphics_command_buffers + frame_index; 
        
        *command_buffer = r_vulkan_command_buffer_acquire(render_context, 
                                                          render_context->rendering_device.graphics_command_pool, 
                                                          true);
    }
}

////////////////////////////
// VULKAN RENDERPASS 
////////////////////////////

vulkan_renderpass_data_t
r_vulkan_renderpass_create(vulkan_render_context_t *render_context, 
                           vec2_t                   offset, 
                           vec2_t                   size, 
                           vec4_t                   clear_color, 
                           float32                  clear_depth, 
                           u32                      stencil_value)
{
    log_trace("renderpass size is: '%.02f'... '%.02f'...\n", size.x, size.y);

    vulkan_renderpass_data_t result = {};
    result.clear_color   = clear_color;
    result.depth_clear   = clear_depth;
    result.stencil_clear = stencil_value;
    result.size          = size;
    result.offset        = offset;

    VkSubpassDescription primary_subpass = {};
    primary_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // NOTE(Sleepster): Why is this a fucking clang extension????? 
    u32 attachment_description_count = render_context->swapchain.has_depth_attachment ? 2 : 1;
    VkAttachmentDescription attachment_descriptions[attachment_description_count];

    // NOTE(Sleepster): index 0, color attachment
    attachment_descriptions[0] = {
        .format         = render_context->swapchain.image_format.format,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_reference = {
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    primary_subpass.colorAttachmentCount = 1;
    primary_subpass.pColorAttachments    = &color_attachment_reference;

    // NOTE(Sleepster): index 1, depth attachment
    VkAttachmentReference depth_attachment_reference;
    if(render_context->swapchain.has_depth_attachment)
    {
        attachment_descriptions[1] = {
            .format = render_context->rendering_device.physical_device.device_depth_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        depth_attachment_reference = {
            .attachment = 1,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL 
        };

        primary_subpass.pDepthStencilAttachment = &depth_attachment_reference;
    }

    // NOTE(Sleepster): Input from a shader... 
    primary_subpass.inputAttachmentCount = 0;
    primary_subpass.pInputAttachments    = null;

    // NOTE(Sleepster): Used for multisampling...
    primary_subpass.pResolveAttachments = 0;

    // NOTE(Sleepster): Not needed this pass, but should be preserved for a future pass...
    primary_subpass.preserveAttachmentCount = 0;
    primary_subpass.pPreserveAttachments    = null;

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
        .attachmentCount = attachment_description_count,
        .pAttachments    = attachment_descriptions,
        .subpassCount    = 1,
        .pSubpasses      = &primary_subpass,
        .dependencyCount = 1,
        .pDependencies   = &subpass_dependencies,
        .pNext           = null,
        .flags           = 0
    };
    VkAssert(vkCreateRenderPass(render_context->rendering_device.logical_device, 
                               &renderpass_create_info, 
                                render_context->allocators, 
                               &result.handle));
    log_info("Vulkan renderpass created successfully...\n");

    return(result);
}

void
r_vulkan_renderpass_destroy(vulkan_render_context_t *render_context, vulkan_renderpass_data_t *renderpass)
{
    if(renderpass && renderpass->handle)
    {
        vkDestroyRenderPass(render_context->rendering_device.logical_device, renderpass->handle, render_context->allocators);
        renderpass->handle = 0;
    }
    else
    {
        log_warning("Tried to destroy a renderpass, but it's data isn't valid...\n");
    }
}

void
r_vulkan_renderpass_begin(vulkan_render_context_t      *render_context,
                          vulkan_renderpass_data_t     *renderpass,
                          vulkan_command_buffer_data_t *command_buffer,
                          VkFramebuffer                 framebuffer)
{
    VkRenderPassBeginInfo begin_info = {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass  = renderpass->handle,
        .framebuffer = framebuffer,
        .renderArea = {
            .offset.x      = (s32)renderpass->offset.x,
            .offset.y      = (s32)renderpass->offset.y,
            .extent.width  = (u32)renderpass->size.x,
            .extent.height = (u32)renderpass->size.y
        }
    };

    VkClearValue clear_values[2] = {};
    clear_values[0].color.float32[0] = renderpass->clear_color.x;
    clear_values[0].color.float32[1] = renderpass->clear_color.y;
    clear_values[0].color.float32[2] = renderpass->clear_color.z;
    clear_values[0].color.float32[3] = renderpass->clear_color.w;

    clear_values[1].depthStencil.depth   = renderpass->depth_clear;
    clear_values[1].depthStencil.stencil = renderpass->stencil_clear;

    begin_info.clearValueCount = 2;
    begin_info.pClearValues    = clear_values;
    
    vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = VKCBS_WITHIN_RENDERPASS;
}

void
r_vulkan_renderpass_end(vulkan_render_context_t      *render_context,
                        vulkan_command_buffer_data_t *command_buffer,
                        vulkan_renderpass_data_t     *renderpass)
{
    vkCmdEndRenderPass(command_buffer->handle);
    command_buffer->state = VKCBS_RECORDING;
}


////////////////////////////
// VULKAN SWAPCHAIN 
////////////////////////////

vulkan_swapchain_data_t
r_vulkan_swapchain_create(vulkan_render_context_t *render_context, 
                          u32                      image_width, 
                          u32                      image_height,
                          bool8                    wants_depth_buffer,
                          vulkan_swapchain_data_t *old_swapchain)
{
    Assert(image_width < 4096);
    log_trace("renderpass size is: '%d'... %d...\n", image_width, image_height);

    vulkan_swapchain_data_t result = {};
    result.arena = c_arena_create(MB(10));

    VkExtent2D swapchain_extent = {image_width, image_height};
        
    // NOTE(Sleepster): Triple buffering. 
    result.max_frames_in_flight = 2;

    vulkan_rendering_device_t *device_data = &render_context->rendering_device;
    
    bool8 default_surface_format_found = false;
    for(u32 format_index = 0;
        format_index < device_data->physical_device.swapchain_support_info.valid_surface_format_count;
        ++format_index)
    {
        VkSurfaceFormatKHR format = device_data->physical_device.swapchain_support_info.valid_surface_formats[format_index];
        if(format.format     == VK_FORMAT_R8G8B8A8_UNORM &&
           format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {    
            result.image_format = format;
            default_surface_format_found = true;

            break;
        }
    }

    if(!default_surface_format_found)
    {
        log_error("You really don't have this default image format?...\n");
        result.image_format = device_data->physical_device.swapchain_support_info.valid_surface_formats[0];
    }

    // NOTE(Sleepster): Vulkan spec says by default this one must exist:
    // https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkPresentModeKHR.html
    VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for(u32 present_mode_index = 0;
        present_mode_index < device_data->physical_device.swapchain_support_info.valid_present_mode_count;
        ++present_mode_index)
    {
        VkPresentModeKHR mode = device_data->physical_device.swapchain_support_info.valid_present_modes[present_mode_index];
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain_present_mode = mode;
            break;
        }
    }
    result.present_mode = swapchain_present_mode; 

    r_vulkan_physical_device_get_swapchain_support_info(render_context, 
                                                        device_data->physical_device.handle,
                                                       &device_data->physical_device.swapchain_support_info);
    if(device_data->physical_device.swapchain_support_info.surface_capabilities.currentExtent.width != U32_MAX)
    {
        swapchain_extent = device_data->physical_device.swapchain_support_info.surface_capabilities.currentExtent;
    }
    //VkExtent2D max_extent = device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageExtent;

    u32 swapchain_image_count = device_data->physical_device.swapchain_support_info.surface_capabilities.minImageCount + 1;
    if(device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount > 0 && 
       swapchain_image_count > device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount)
    {
        swapchain_image_count = device_data->physical_device.swapchain_support_info.surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = render_context->render_surface,
        .minImageCount    = swapchain_image_count,
        .imageFormat      = result.image_format.format,
        .imageColorSpace  = result.image_format.colorSpace,
        .imageExtent      = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    if(device_data->graphics_queue_family_index != device_data->present_queue_family_index)
    {
        u32 queue_family_indices[] = {
            device_data->graphics_queue_family_index,
            device_data->present_queue_family_index
        };

        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices   = 0;
    }

    // NOTE(Sleepster): COMPOSITE_ALPHA here is for the windowing system. NOT FOR THE IMAGES. 
    swapchain_create_info.preTransform   = device_data->physical_device.swapchain_support_info.surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode    = swapchain_present_mode;
    swapchain_create_info.clipped        = true;
    swapchain_create_info.oldSwapchain   = old_swapchain != null ? old_swapchain->handle : null;
    VkAssert(vkCreateSwapchainKHR(device_data->logical_device, &swapchain_create_info, render_context->allocators, &result.handle));

    render_context->current_frame_index = 0;
    render_context->current_image_index = 0;

    result.image_count = 0;
    VkAssert(vkGetSwapchainImagesKHR(device_data->logical_device, result.handle, &result.image_count, null));
    result.images       = c_arena_push_array(&result.arena, VkImage,     result.image_count);
    result.views        = c_arena_push_array(&result.arena, VkImageView, result.image_count);
    result.framebuffers = c_arena_push_array(&result.arena, vulkan_framebuffer_data_t, result.image_count);
    VkAssert(vkGetSwapchainImagesKHR(device_data->logical_device, result.handle, &result.image_count, result.images));

    for(u32 view_index = 0;
        view_index < result.image_count;
        ++view_index)
    {
        VkImageViewCreateInfo image_view_create_info = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = result.images[view_index],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = result.image_format.format,
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            },
        };

        VkAssert(vkCreateImageView(device_data->logical_device, &image_view_create_info, render_context->allocators, &result.views[view_index]));
    }

    if(wants_depth_buffer)
    {
        bool8 could_get = r_vulkan_physical_device_detect_depth_format(&device_data->physical_device);
        if(!could_get)
        {
            device_data->physical_device.device_depth_format = VK_FORMAT_UNDEFINED;
            log_fatal("Failure to find a supported depth format...\n");
        }

        result.depth_attachment = r_vulkan_image_create(render_context,
                                                        VK_IMAGE_TYPE_2D, 
                                                        swapchain_extent.width,
                                                        swapchain_extent.height,
                                                        device_data->physical_device.device_depth_format,
                                                        VK_IMAGE_TILING_OPTIMAL,
                                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                        VK_IMAGE_ASPECT_DEPTH_BIT,
                                                        1,
                                                        true);
        result.has_depth_attachment = true;
    }
    result.is_valid = true;

    log_info("Swapchain Created...\n");
    return(result);
}

void
r_vulkan_swapchain_destroy(vulkan_render_context_t *render_context,
                           vulkan_swapchain_data_t *swapchain)
{
    if(swapchain->depth_attachment.handle)
    {
        r_vulkan_image_destroy(render_context, &swapchain->depth_attachment);
    }

    for(u32 view_index = 0;
        view_index < swapchain->image_count;
        ++view_index)
    {
        vkDestroyImageView(render_context->rendering_device.logical_device, 
                           swapchain->views[view_index], 
                           render_context->allocators);
    }
    c_arena_destroy(&swapchain->arena);

    vkDestroySwapchainKHR(render_context->rendering_device.logical_device, swapchain->handle, render_context->allocators);
    swapchain->is_valid     = false;
    swapchain->handle       = null;
    swapchain->framebuffers = null;
    swapchain->views        = null;
    swapchain->images       = null;

    log_info("Swapchain destroyed...\n");
}

vulkan_swapchain_data_t
r_vulkan_swapchain_recreate(vulkan_render_context_t *render_context, 
                            u32                      image_width, 
                            u32                      image_height)
{
    log_trace("swapchain recreate size is: '%d'... '%d'...\n", image_width, image_height);
    vulkan_swapchain_data_t result;

    // TODO(Sleepster): Let this reuse the swapchain if possible
    r_vulkan_swapchain_destroy(render_context, &render_context->swapchain);

    result = r_vulkan_swapchain_create(render_context, 
                                       image_width, 
                                       image_height, 
                                       true, 
                                       null);
    log_info("Swapchain Recreated...\n");
    return(result);
}

u32 
r_vulkan_swapchain_get_next_image_index(vulkan_render_context_t *render_context,
                                        vulkan_swapchain_data_t *swapchain,
                                        u64                      timeout_ns,
                                        VkSemaphore              image_semaphore,
                                        VkFence                  fence)
{
    u32 result = 0;
    VkResult code = vkAcquireNextImageKHR(render_context->rendering_device.logical_device, 
                                          render_context->swapchain.handle,
                                          timeout_ns, 
                                          image_semaphore,
                                          fence,
                                         &result);
    if(code == VK_ERROR_OUT_OF_DATE_KHR)
    {
        render_context->swapchain = r_vulkan_swapchain_recreate(render_context, 
                                                                render_context->framebuffer_width, 
                                                                render_context->framebuffer_height);
    }
    else if(code != VK_SUCCESS && code != VK_SUBOPTIMAL_KHR)
    {
        log_fatal("Failure to get the swapchain image...\n");
        Assert(false);

        result = INVALID_SWAPCHAIN_IMAGE_INDEX;
    }

    return(result);
}

void
r_vulkan_swapchain_present(vulkan_render_context_t *render_context,
                           vulkan_swapchain_data_t *swapchain,
                           VkQueue                  graphics_queue,
                           VkQueue                  present_queue,
                           VkSemaphore              render_semaphore,
                           u32                      image_index)
{
    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &render_semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain->handle,
        .pImageIndices      = &image_index,
    };
    
    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        render_context->swapchain = r_vulkan_swapchain_recreate(render_context, 
                                                                render_context->framebuffer_width, 
                                                                render_context->framebuffer_height);
    }
    else if(result != VK_SUCCESS)
    {
        log_fatal("Failure to present the swapchain image...\n");
        Assert(false);
    }

    render_context->current_frame_index = (render_context->current_frame_index + 1) % swapchain->max_frames_in_flight;
}

////////////////////////////
// VULKAN PHYSICAL DEVICE 
////////////////////////////

bool8
r_vulkan_physical_device_detect_depth_format(vulkan_physical_device_t *device)
{
    bool8 result = false;

    VkFormat valid_canidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    u32 attachment_flag = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for(u32 canidate_index = 0;
        canidate_index < ArrayCount(valid_canidates);
        ++canidate_index)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->handle, valid_canidates[canidate_index], &properties);
        if((properties.linearTilingFeatures & attachment_flag) == attachment_flag)
        {
            device->device_depth_format = valid_canidates[canidate_index];
            result = true;

            break;
        }
        else if((properties.optimalTilingFeatures & attachment_flag) == attachment_flag)
        {
            device->device_depth_format = valid_canidates[canidate_index];
            result = true;

            break;
        }
    }

    return(result);
}

void 
r_vulkan_physical_device_get_swapchain_support_info(vulkan_render_context_t                         *context, 
                                                    VkPhysicalDevice                                 device,
                                                    vulkan_physical_device_swapchain_support_info_t *swapchain_info)
{
    VkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, 
                                                       context->render_surface, 
                                                       &swapchain_info->surface_capabilities));
    VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->render_surface, &swapchain_info->valid_surface_format_count, 0));
    if(swapchain_info->valid_surface_format_count > 0)
    {
        VkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(device, 
                                                      context->render_surface, 
                                                      &swapchain_info->valid_surface_format_count, 
                                                      swapchain_info->valid_surface_formats));
    }

    VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->render_surface, &swapchain_info->valid_present_mode_count, 0));
    if(swapchain_info->valid_present_mode_count > 0)
    {
        VkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->render_surface, 
                                                           &swapchain_info->valid_present_mode_count, 
                                                           swapchain_info->valid_present_modes));
    }
}

bool32 
r_vulkan_physical_device_is_supported(vulkan_render_context_t                         *context_data, 
                                      VkPhysicalDevice                                 device,
                                      VkPhysicalDeviceProperties                      *device_properties,
                                      VkPhysicalDeviceFeatures                        *device_features,
                                      vulkan_physical_device_swapchain_support_info_t *device_swapchain_support_info,
                                      vulkan_physical_device_queue_info_t             *device_queue_info,
                                      vulkan_physical_device_requirements_t           *requirements)
{
    device_queue_info->graphics_queue_family_index = (u32)-1;
    device_queue_info->present_queue_family_index  = (u32)-1;
    device_queue_info->transfer_queue_family_index = (u32)-1;
    device_queue_info->compute_queue_family_index  = (u32)-1;

    u32 queue_family_counter;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_counter, 0);
    VkQueueFamilyProperties *queue_family_data = c_arena_push_array(&context_data->initialization_arena, VkQueueFamilyProperties, queue_family_counter);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_counter, queue_family_data);

    u8 minimum_transfer_score = 255;
    for(u32 queue_family_index = 0;
        queue_family_index < queue_family_counter;
        ++queue_family_index)
    {
        u8 current_transfer_score = 0;

        VkQueueFamilyProperties properties = queue_family_data[queue_family_index];
        if(properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            device_queue_info->graphics_queue_family_index = queue_family_index;
            ++current_transfer_score;
        }

        if(properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            device_queue_info->compute_queue_family_index = queue_family_index;
            ++current_transfer_score;
        }

        if(properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            if(current_transfer_score <= minimum_transfer_score)
            {
                minimum_transfer_score = current_transfer_score;
                device_queue_info->transfer_queue_family_index = queue_family_index;
            }
        }

        VkBool32 supports_presenting = VK_FALSE;
        VkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_family_index, context_data->render_surface, &supports_presenting));
        if(supports_presenting)
        {
            device_queue_info->present_queue_family_index = queue_family_index;
        }
    }

    log_info("GRAPHICS | PRESENT | COMPUTE | TRANSFER | DEVICE NAME\n");
    log_info("   %d     |    %d    |    %d    |     %d    | %s\n",
             device_queue_info->graphics_queue_family_index, device_queue_info->present_queue_family_index,
             device_queue_info->compute_queue_family_index,  device_queue_info->transfer_queue_family_index,
             device_properties->deviceName);
    if((!requirements->has_graphics_queue || (requirements->has_graphics_queue && device_queue_info->graphics_queue_family_index != (u32)-1)) &&
       (!requirements->has_present_queue  || (requirements->has_present_queue  && device_queue_info->present_queue_family_index  != (u32)-1)) &&
       (!requirements->has_compute_queue  || (requirements->has_compute_queue  && device_queue_info->compute_queue_family_index  != (u32)-1)) &&
       (!requirements->has_transfer_queue || (requirements->has_transfer_queue && device_queue_info->transfer_queue_family_index != (u32)-1)))
    {
        log_info("Device meets queue requirements...\n");
        log_trace("Graphics Queue Family Index: %d\n", device_queue_info->graphics_queue_family_index);
        log_trace("Present  Queue Family Index: %d\n", device_queue_info->present_queue_family_index);
        log_trace("Transfer Queue Family Index: %d\n", device_queue_info->transfer_queue_family_index);
        log_trace("Compute  Queue Family Index: %d\n", device_queue_info->compute_queue_family_index);

        r_vulkan_physical_device_get_swapchain_support_info(context_data, device, device_swapchain_support_info);
        

        if(device_swapchain_support_info->valid_surface_format_count < 1 || device_swapchain_support_info->valid_present_mode_count < 1)
        {
            log_info("Swapchain requirements not met, skipping this device...\n");
            return(false);
        }

        if(requirements->required_extensions)
        {
            u32 device_avaliable_extension_counter = 0;
            VkExtensionProperties *device_avaliable_extensions = null;

            VkAssert(vkEnumerateDeviceExtensionProperties(device, 0, &device_avaliable_extension_counter, 0));
            if(device_avaliable_extension_counter != 0)
            {
                device_avaliable_extensions = c_arena_push_array(&context_data->initialization_arena, VkExtensionProperties, device_avaliable_extension_counter);
            }
            VkAssert(vkEnumerateDeviceExtensionProperties(device, 0, &device_avaliable_extension_counter, device_avaliable_extensions));

            for(u32 extension_index = 0;
                extension_index < ArrayCount(requirements->required_extensions);
                ++extension_index)
            {
                bool8 found = false;
                const char *extension = requirements->required_extensions[extension_index];

                for(u32 device_extension_index = 0;
                    device_extension_index < device_avaliable_extension_counter;
                    ++device_extension_index)
                {
                    const char *device_extension = device_avaliable_extensions[device_extension_index].extensionName;
                    if(strcmp(extension, device_extension) == 0)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    log_info("Vulkan Extension '%s' was no found... skipping device...\n", extension);
                    return(false);
                }
            }
        }

        return(true);
    }

    return(false);
}

////////////////////////////
// VULKAN RENDERER CORE 
////////////////////////////

void
r_vulkan_on_resize(vulkan_render_context_t *render_context, vec2_t new_window_size)
{
    render_context->cached_framebuffer_width  = new_window_size.x;
    render_context->cached_framebuffer_height = new_window_size.y;

    render_context->current_framebuffer_size_generation++;
}

bool8
r_vulkan_begin_frame(vulkan_render_context_t *render_context, float32 delta_time)
{
    bool8 result = false;
    vulkan_rendering_device_t *device = &render_context->rendering_device;

    if(render_context->recreating_swapchain)
    {
        VkResult wait_result = vkDeviceWaitIdle(device->logical_device);
        if(!r_vulkan_result_is_success(wait_result))
        {
            log_error("Begin frame failed on VkDeviceWaitIdle(): '%s'...\n", 
                      r_vulkan_result_string(wait_result, true));
        }
        log_info("Recreating swapchain... Can't begin...\n");
        result = false;

        goto begin_frame_return;
    }
    else
    {
        result = true;
        if(render_context->current_framebuffer_size_generation != 
           render_context->last_framebuffer_size_generation)
        {
            VkResult wait_result = vkDeviceWaitIdle(device->logical_device);
            if(!r_vulkan_result_is_success(wait_result))
            {
                log_error("Begin frame failed on VkDeviceWaitIdle(): '%s'...\n", 
                          r_vulkan_result_string(wait_result, true));
                result = false;

                goto begin_frame_return;
            }

            Assert(render_context->swapchain.is_valid);
            r_vulkan_rebuild_swapchain(render_context);
            if(!render_context->swapchain.is_valid)
            {
                result = false;
                log_error("Swapchain recreation is invalid...\n");

                goto begin_frame_return;
            }
        }

        bool8 waited = r_vulkan_fence_wait(render_context, 
                                           render_context->image_fences + render_context->current_frame_index, 
                                           U64_MAX);
        if(!waited)
        {
            log_warning("Frame fence wait has failed...\n");
            result = false;

            goto begin_frame_return;
        }

        render_context->current_image_index = r_vulkan_swapchain_get_next_image_index(render_context,
                                                                                     &render_context->swapchain,
                                                                                      U64_MAX,
                                                                                      render_context->image_avaliable_semaphores[render_context->current_frame_index],
                                                                                      null);
        if(render_context->current_frame_index == INVALID_SWAPCHAIN_IMAGE_INDEX)
        {
            result = false;
            log_error("We have failed to gather a valid swapchain image...\n");

            goto begin_frame_return;
        }

        vulkan_command_buffer_data_t *command_buffer = render_context->graphics_command_buffers + render_context->current_image_index; 
        Assert(command_buffer);

        r_vulkan_command_buffer_reset(command_buffer);
        r_vulkan_command_buffer_begin(command_buffer, false, false, false);

        // NOTE(Sleepster): This might be a problem in the future... 
        // We do this to keep this in line with how OpenGL does things.
        VkViewport viewport_data = {
            .x        = 0.0f,
            .y        =  (float32)render_context->framebuffer_height,
            .width    =  (float32)render_context->framebuffer_width,
            .height   = -(float32)render_context->framebuffer_height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        VkRect2D scissor_data = {
            .offset = {
                .x = 0,
                .y = 0,
            },
            .extent = {
                .width  = render_context->framebuffer_width,
                .height = render_context->framebuffer_height,
            }
        };

        vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport_data);
        vkCmdSetScissor(command_buffer->handle,  0, 1, &scissor_data);

        render_context->main_renderpass.size.x = (float32)render_context->framebuffer_width;
        render_context->main_renderpass.size.y = (float32)render_context->framebuffer_height;

        r_vulkan_renderpass_begin(render_context, 
                                 &render_context->main_renderpass, 
                                  command_buffer, 
                                  render_context->swapchain.framebuffers[render_context->current_image_index].handle);


    // TODO(Sleepster): TRIANGLE CODE
    r_vulkan_shader_bind(render_context, &render_context->default_shader);
    r_vulkan_shader_update_descriptor_sets(render_context,
                                           &render_context->default_shader);

    VkDeviceSize offsets[1] = {};
    vkCmdBindVertexBuffers(command_buffer->handle, 0, 1, &render_context->vertex_buffer.handle, (VkDeviceSize*)&offsets);
    vkCmdBindIndexBuffer(command_buffer->handle, render_context->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(command_buffer->handle, 3, 1, 0, 0, 0);

    // TODO(Sleepster): TRIANGLE CODE
    }

begin_frame_return:
    return(result);
}

bool8
r_vulkan_end_frame(vulkan_render_context_t *render_context, float32 delta_time)
{
    bool8 result = true;

    vulkan_command_buffer_data_t *command_buffer = render_context->graphics_command_buffers + render_context->current_image_index; 
    r_vulkan_renderpass_end(render_context, command_buffer, 
                           &render_context->main_renderpass);
    r_vulkan_command_buffer_end(command_buffer);

    // NOTE(Sleepster): Blocking... just syncing to make sure we aren't writing too fast 
    if(render_context->frame_fences[render_context->current_image_index] != VK_NULL_HANDLE)
    {
        r_vulkan_fence_wait(render_context, 
                            render_context->image_fences + render_context->current_image_index,
                            U64_MAX);
    }

    render_context->frame_fences[render_context->current_image_index] = &render_context->image_fences[render_context->current_frame_index];
    r_vulkan_fence_reset(render_context, render_context->image_fences + render_context->current_frame_index);

    VkPipelineStageFlags stage_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        // NOTE(Sleepster): Command buffer()s that will be run 
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffer->handle,

        // NOTE(Sleepster): The Semaphore(s) that signal when the queue is finished executing the commands
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = render_context->queue_finished_semaphores + render_context->current_image_index,

        // NOTE(Sleepster): The Semaphore(s) that ensures the operation cannot begin until the image is avaliable 
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = render_context->image_avaliable_semaphores + render_context->current_frame_index,

        .pWaitDstStageMask    = stage_flags
    };

    VkResult submit_result = vkQueueSubmit(render_context->rendering_device.graphics_queue, 
                                           1, 
                                          &submit_info, 
                                           render_context->image_fences[render_context->current_frame_index].handle);
    if(submit_result != VK_SUCCESS)
    {
        log_error("vkQueueSubmit has failed with result: '%s'...\n", r_vulkan_result_string(submit_result, true));
        result = false;
    }
    else
    {
        r_vulkan_command_buffer_submit(command_buffer);
        r_vulkan_swapchain_present(render_context, 
                                  &render_context->swapchain,
                                   render_context->rendering_device.graphics_queue,
                                   render_context->rendering_device.present_queue,
                                   render_context->queue_finished_semaphores[render_context->current_image_index],
                                   render_context->current_image_index);
    }

    c_arena_reset(&render_context->frame_arena);
    return(result);
}

void
r_vulkan_regenerate_framebuffers(vulkan_render_context_t  *render_context,
                                 vulkan_swapchain_data_t  *swapchain,
                                 vulkan_renderpass_data_t *renderpass)
{
    for(u32 image_index = 0;
        image_index < swapchain->image_count;
        ++image_index)
    {
        // TODO(Sleepster): Make this configurable 
        u32 attachment_count = 2;
        VkImageView attachments[] = {
            swapchain->views[image_index],
            swapchain->depth_attachment.view
        };

        swapchain->framebuffers[image_index] = r_vulkan_framebuffer_create(render_context,
                                                                           swapchain,
                                                                           renderpass,
                                                                           render_context->framebuffer_width,
                                                                           render_context->framebuffer_height,
                                                                           attachment_count,
                                                                           attachments);
    }
}

bool8
r_vulkan_rebuild_swapchain(vulkan_render_context_t *render_context)
{
    bool8 result = true;
    if(!render_context->recreating_swapchain)
    {
        if((render_context->framebuffer_width != 0) && (render_context->framebuffer_height != 0))
        {
            render_context->recreating_swapchain = true;
            vkDeviceWaitIdle(render_context->rendering_device.logical_device);

            for(u32 frame_index = 0;
                frame_index < render_context->swapchain.image_count;
                ++frame_index)
            {
                render_context->frame_fences[frame_index] = null;
            }

            r_vulkan_physical_device_get_swapchain_support_info(render_context, 
                                                                render_context->rendering_device.physical_device.handle, 
                                                               &render_context->rendering_device.physical_device.swapchain_support_info);
            r_vulkan_physical_device_detect_depth_format(&render_context->rendering_device.physical_device);

            render_context->swapchain = r_vulkan_swapchain_recreate(render_context, 
                                                                    render_context->cached_framebuffer_width,
                                                                    render_context->cached_framebuffer_height);

            render_context->framebuffer_width         = render_context->cached_framebuffer_width;
            render_context->framebuffer_height        = render_context->cached_framebuffer_height;
            render_context->main_renderpass.size.x    = render_context->framebuffer_width;
            render_context->main_renderpass.size.y    = render_context->framebuffer_height;
            render_context->cached_framebuffer_width  = 0;
            render_context->cached_framebuffer_height = 0;

            // NOTE(Sleepster): We are creating a framebuffer and a command buffer for EACH OF THE SWAPCHAIN INDICES
            // regardless of if they are a part of our "frames in flight"
            render_context->last_framebuffer_size_generation = render_context->current_framebuffer_size_generation;
            for(u32 command_buffer_index = 0;
                command_buffer_index < render_context->swapchain.image_count;
                ++command_buffer_index)
            {
                vulkan_command_buffer_data_t *graphics_command_buffer = render_context->graphics_command_buffers + command_buffer_index;
                r_vulkan_command_buffer_release(render_context, graphics_command_buffer, render_context->rendering_device.graphics_command_pool);
            }

            for(u32 framebuffer_index = 0;
                framebuffer_index < render_context->swapchain.image_count;
                ++framebuffer_index)
            {
                vulkan_framebuffer_data_t *framebuffer = render_context->swapchain.framebuffers + framebuffer_index;
                r_vulkan_framebuffer_destroy(render_context, framebuffer);
            }

            render_context->main_renderpass.offset.x = 0;
            render_context->main_renderpass.offset.y = 0;
            render_context->main_renderpass.size.x   = render_context->framebuffer_width;
            render_context->main_renderpass.size.y   = render_context->framebuffer_height;

            r_vulkan_regenerate_framebuffers(render_context, 
                                            &render_context->swapchain, 
                                            &render_context->main_renderpass);
            r_vulkan_initialize_graphics_command_buffers(render_context);

            render_context->recreating_swapchain = false;
        }
        else
        {
            log_warning("Not recreating the swapchain when the window is < 1 in size... X: '%d, Y: '%d'\n", 
                        render_context->framebuffer_width, render_context->framebuffer_height);
            result = false;
        }
    }
    else
    {
        log_warning("Not trying to rebuild the swapchain whilst it is already being created...\n");
        result = false;
    }
    return(result);
}

s32 
r_vulkan_find_memory_index(vulkan_render_context_t *render_context,
                           u32                      type_filter, 
                           u32                      property_flags)
{
    s32 result = -1;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(render_context->rendering_device.physical_device.handle, &memory_properties);

    for(u32 memory_index = 0;
        memory_index < memory_properties.memoryTypeCount;
        ++memory_index)
    
    {
        if((type_filter & (1 << memory_index)) && ((memory_properties.memoryTypes[memory_index].propertyFlags & property_flags) == property_flags))
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

void
r_renderer_init(vulkan_render_context_t *render_context, vec2_t window_size)
{
    render_context->framebuffer_width  = window_size.x;
    render_context->framebuffer_height = window_size.y;
    render_context->window_width  = 0;
    render_context->window_height = 0;

    render_context->initialization_arena = c_arena_create(MB(10));
    render_context->frame_arena          = c_arena_create(MB(10));
    render_context->permanent_arena      = c_arena_create(MB(100));

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
    instance_info.pApplicationInfo        = &app_info;

    VkAssert(vkCreateInstance(&instance_info, render_context->allocators, &render_context->instance));
    log_info("Vulkan Instance Created..\n");

    // NOTE(Sleepster): DEBUG LAYERS 
    {
        u32 debug_log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

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

    c_dynarray_destroy(extensions);
    c_dynarray_destroy(validation_layers);
    c_dynarray_destroy(found_validation_layers);

    if(!SDL_Vulkan_CreateSurface(render_context->window, render_context->instance, render_context->allocators, &render_context->render_surface))
    {
        log_error("Failure to create the SDL Vulkan Surface... Error: '%s\n'", SDL_GetError());
    }

    log_info("Vulkan surface created...\n");

    // NOTE(Sleepster): GET VULKAN PHYSICAL DEVICE
    {
        u32 physical_device_counter = 0;
        VkAssert(vkEnumeratePhysicalDevices(render_context->instance, &physical_device_counter, 0));
        VkPhysicalDevice *physical_devices = c_arena_push_array(&render_context->initialization_arena, VkPhysicalDevice, physical_device_counter);
        VkAssert(vkEnumeratePhysicalDevices(render_context->instance, &physical_device_counter, physical_devices));

        log_info("Physical Device(s) found!\n");
        for(u32 device_index = 0;
            device_index < physical_device_counter;
            ++device_index)
        {
            VkPhysicalDevice current_device = physical_devices[device_index];

            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(current_device, &device_properties);

            VkPhysicalDeviceFeatures device_features;
            vkGetPhysicalDeviceFeatures(current_device, &device_features);

            VkPhysicalDeviceMemoryProperties device_memory_properties;
            vkGetPhysicalDeviceMemoryProperties(current_device, &device_memory_properties);

            vulkan_physical_device_requirements_t requirements;
            requirements.has_graphics_queue = true;
            requirements.has_present_queue  = true;
            requirements.has_transfer_queue = true;
            requirements.has_compute_queue  = true;

            requirements.required_extensions = c_arena_push_array(&render_context->initialization_arena, const char *, 10);
            requirements.required_extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

            vulkan_physical_device_swapchain_support_info_t device_swapchain_support_info = {};
            vulkan_physical_device_queue_info_t             device_queue_information = {};
            if(r_vulkan_physical_device_is_supported(render_context,
                                                     current_device,
                                                     &device_properties,
                                                     &device_features,
                                                     &device_swapchain_support_info,
                                                     &device_queue_information,
                                                     &requirements))
            {
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
                for(u32 heap_index = 0;
                    heap_index < device_memory_properties.memoryHeapCount;
                    ++heap_index)
                {
                    float32 memory_sizeGB = (float32)(device_memory_properties.memoryHeaps[heap_index].size) / 1024.0f / 1024.0f / 1024.0f;
                    if(device_memory_properties.memoryHeaps[heap_index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    {
                        log_info("Local GPU Memory: '%.2fGB'...\n", memory_sizeGB);
                    }
                    else
                    {
                        log_info("Shared GPU Memory: '%.2fGB'...\n", memory_sizeGB);
                    }
                }

                vulkan_rendering_device_t *device = &render_context->rendering_device;

                device->physical_device.handle = current_device;
                Assert(device->physical_device.handle);

                device->graphics_queue_family_index = device_queue_information.graphics_queue_family_index;
                device->present_queue_family_index  = device_queue_information.present_queue_family_index;
                device->transfer_queue_family_index = device_queue_information.transfer_queue_family_index;
                device->compute_queue_family_index  = device_queue_information.compute_queue_family_index;

                device->physical_device.properties             = device_properties;
                device->physical_device.features               = device_features;
                device->physical_device.memory_properties      = device_memory_properties;
                device->physical_device.swapchain_support_info = device_swapchain_support_info;

                break;
            }
        }

        if(!render_context->rendering_device.physical_device.handle)
        {
            log_fatal("No compatible physical GPU device was found...\n");
        }

        log_info("Physical device created successfully...\n");
    }

    // NOTE(Sleepster): LOGICAL DEVICE
    {
        bool8 present_queue_shares_graphics_queue  = render_context->rendering_device.graphics_queue_family_index  == render_context->rendering_device.present_queue_family_index;
        bool8 transfer_queue_shares_graphics_queue = render_context->rendering_device.graphics_queue_family_index  == render_context->rendering_device.transfer_queue_family_index;
        bool8 compute_queue_shares_any             = (render_context->rendering_device.graphics_queue_family_index == render_context->rendering_device.compute_queue_family_index) ||
                                                     (render_context->rendering_device.present_queue_family_index  == render_context->rendering_device.compute_queue_family_index)  ||
                                                     (render_context->rendering_device.transfer_queue_family_index == render_context->rendering_device.compute_queue_family_index);

        u32 index_count = 1;
        if(!present_queue_shares_graphics_queue)  index_count++;
        if(!transfer_queue_shares_graphics_queue) index_count++;
        if(!compute_queue_shares_any)             index_count++;

        u32 indices[4] = {};
        u32 index = 0;

        indices[index++] = render_context->rendering_device.graphics_queue_family_index;
        if(!present_queue_shares_graphics_queue)
        {
            indices[index++] = render_context->rendering_device.present_queue_family_index;
        }
        if(!transfer_queue_shares_graphics_queue)
        {
            indices[index++] = render_context->rendering_device.transfer_queue_family_index;
        }
        if(!compute_queue_shares_any)
        {
            indices[index++] = render_context->rendering_device.compute_queue_family_index;
        }

        VkDeviceQueueCreateInfo queue_create_infos[4] = {};
        for(u32 queue_index = 0;
            queue_index < index_count;
            ++queue_index)
        {
            VkDeviceQueueCreateInfo *info = queue_create_infos + queue_index;

            info->sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info->queueFamilyIndex = indices[queue_index];
            info->queueCount       = 1;
            if(indices[queue_index] == render_context->rendering_device.graphics_queue_family_index)
            {
                info->queueCount = 2;
            }

            info->flags = 0;
            info->pNext = 0;

            float32 queue_priority = 1.0f;
            info->pQueuePriorities = &queue_priority;
        }

        VkPhysicalDeviceFeatures device_features = {
            .logicOp           = true,
            .samplerAnisotropy = true,
        };

        VkDeviceCreateInfo device_create_info = {};
        device_create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.queueCreateInfoCount  = index_count;
        device_create_info.pQueueCreateInfos     = queue_create_infos;
        device_create_info.pEnabledFeatures      = &device_features;
        device_create_info.enabledExtensionCount = 1;

        const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        device_create_info.ppEnabledExtensionNames = &extension;

        VkAssert(vkCreateDevice(render_context->rendering_device.physical_device.handle,
                               &device_create_info,
                                render_context->allocators,
                               &render_context->rendering_device.logical_device));
        log_info("Logical device created...\n");

        // TODO(Sleepster): Shouldn't we be using the queue at index 2 here for one of these? 
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.graphics_queue_family_index, 0, &render_context->rendering_device.graphics_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.present_queue_family_index,  0, &render_context->rendering_device.present_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.transfer_queue_family_index, 0, &render_context->rendering_device.transfer_queue);
        vkGetDeviceQueue(render_context->rendering_device.logical_device, render_context->rendering_device.compute_queue_family_index,  0, &render_context->rendering_device.compute_queue);

        log_info("Device Queues gathered...\n");

        // NOTE(Sleepster): For the flags:
        // https://vkdoc.net/man/VkCommandPoolCreateFlagBits
        VkCommandPoolCreateInfo command_pool_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = render_context->rendering_device.graphics_queue_family_index,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
        };
        VkAssert(vkCreateCommandPool(render_context->rendering_device.logical_device, 
                                    &command_pool_info, 
                                     render_context->allocators, 
                                    &render_context->rendering_device.graphics_command_pool));
        log_info("Device Command Pools created...\n");
    }
    render_context->swapchain       = r_vulkan_swapchain_create(render_context, render_context->framebuffer_width, render_context->framebuffer_height, true, null);
    render_context->main_renderpass = r_vulkan_renderpass_create(render_context, 
                                                                 {0, 0}, 
                                                                 {(float32)render_context->framebuffer_width, (float32)render_context->framebuffer_height}, 
                                                                 {0.3, 0.2, 0.4, 1.0}, 
                                                                 1.0f, 
                                                                 0);
    r_vulkan_regenerate_framebuffers(render_context,
                                    &render_context->swapchain,
                                    &render_context->main_renderpass);
    log_info("Framebuffers created...\n");

    render_context->graphics_command_buffers = c_arena_push_array(&render_context->permanent_arena, vulkan_command_buffer_data_t, render_context->swapchain.image_count);
    r_vulkan_initialize_graphics_command_buffers(render_context);
    log_info("Command buffers initialized...\n");

    render_context->queue_finished_semaphores  = c_arena_push_array(&render_context->permanent_arena, VkSemaphore,     render_context->swapchain.image_count);
    render_context->image_avaliable_semaphores = c_arena_push_array(&render_context->permanent_arena, VkSemaphore,     render_context->swapchain.image_count);
    render_context->image_fences               = c_arena_push_array(&render_context->permanent_arena, vulkan_fence_t,  render_context->swapchain.image_count);
    render_context->frame_fences               = c_arena_push_array(&render_context->permanent_arena, vulkan_fence_t*, render_context->swapchain.image_count);
    for(u32 image_index = 0;
        image_index < render_context->swapchain.image_count;
        ++image_index)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        vkCreateSemaphore(render_context->rendering_device.logical_device, 
                         &semaphore_create_info, 
                          render_context->allocators, 
                         &render_context->queue_finished_semaphores[image_index]);

        vkCreateSemaphore(render_context->rendering_device.logical_device, 
                         &semaphore_create_info, 
                          render_context->allocators, 
                         &render_context->image_avaliable_semaphores[image_index]);

        render_context->image_fences[image_index] = r_vulkan_fence_create(render_context, true);
    }

    render_context->default_shader = r_vulkan_shader_create(render_context, STR("shader_binaries/test.spv"));

    // NOTE(Sleepster): buffer initialization 
    {
        VkMemoryPropertyFlagBits memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkBufferUsageFlagBits vertex_buffer_usage_bits = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT| 
                                                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        render_context->vertex_buffer = r_vulkan_buffer_create(render_context,
                                                               (sizeof(vertex_t) * 1024),
                                                               vertex_buffer_usage_bits,
                                                          (u32)memory_flags,
                                                               true);
        Assert(render_context->vertex_buffer.is_valid);
        VkBufferUsageFlagBits index_buffer_usage_bits = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | 
                                                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                                                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        render_context->index_buffer = r_vulkan_buffer_create(render_context,
                                                              (sizeof(u32) * 1024),
                                                              index_buffer_usage_bits,
                                                         (u32)memory_flags,
                                                              true);
        Assert(render_context->index_buffer.is_valid);

        log_info("Vertex and Index buffers created...\n");
    }

    // TODO(Sleepster): TRIANGLE CODE
    vertex_t vertices[] = {
        [0] = {
            .vPosition = {100, -100, 0.0},
            .vColor    = {1.0, 0.0, 0.0, 1.0}
        },
        [1] = {
            .vPosition = {0, 100, 0.0},
            .vColor    = {0.0, 1.0, 0.0, 1.0}
        },
        [2] = {
            .vPosition = {-100, -100, 0.0},
            .vColor    = {0.0, 0.0, 1.0, 1.0}
        },
    };

    u32 indices[] = {
        0, 1, 2
    };

    r_vulkan_buffer_upload(render_context, 
                          &render_context->vertex_buffer, 
                           vertices,
                           sizeof(vertex_t) * ArrayCount(vertices), 
                           0, 
                           null, 
                           render_context->rendering_device.graphics_command_pool, 
                           render_context->rendering_device.graphics_queue);

    r_vulkan_buffer_upload(render_context, 
                          &render_context->index_buffer, 
                           indices,
                           sizeof(u32) * ArrayCount(indices), 
                           0, 
                           null, 
                           render_context->rendering_device.graphics_command_pool, 
                           render_context->rendering_device.graphics_queue);
    // TODO(Sleepster): TRIANGLE CODE

    log_info("Vulkan context initialized...\n");
    c_arena_destroy(&render_context->initialization_arena);
}
