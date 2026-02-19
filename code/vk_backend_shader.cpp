/* ========================================================================
   $File: vk_backend_shader.cpp $
   $Date: February 18 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_core.h>
#include <vk_backend_buffer.h>
#include <vk_backend_image.h>

#include <spirv_reflect.h>

struct vulkan_shader_t
{
    u32                   shader_id;
    string_t              source;
    VkDescriptorSetLayout layout;

    VkPipeline            pipeline;
};

vulkan_shader_t 
vk_backend_shader_create(vulkan_context_t *vulkan_context, string_t shader_source)
{
    vulkan_shader_t result = {};

    SpvReflectShaderModule module;
    SpvReflectResult error = spvReflectCreateShaderModule(shader_source.count, shader_source.data, &module);
    if(error != SPV_REFLECT_RESULT_SUCCESS)
    {
        log_error("Could not initialize spirv_reflect reflect data for this module...\n");
    }

    return(result);
}
