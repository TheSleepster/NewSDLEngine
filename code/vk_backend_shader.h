#if !defined(VK_BACKEND_SHADER_H)
/* ========================================================================
   $File: vk_backend_shader.h $
   $Date: February 19 2026 07:35 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define VK_BACKEND_SHADER_H
#include <vk_backend_core.h>

struct uniform_constant_buffer_t;

#define MAX_DESCRIPTOR_SET_BINDINGS (5)

struct vulkan_shader_stage_t
{
    VkShaderModule                  handle;
    VkPipelineShaderStageCreateInfo pipeline_stage_create_info;
};

struct vulkan_shader_binding_t
{
    VkDescriptorType           type;
    uniform_constant_buffer_t *cpu_buffer;
    u64                        buffer_hash_index;
    u32                        descriptor_count;
    string_t                   name;

};

struct vulkan_shader_t 
{
    u32                     shader_id;
    string_t                source;
    memory_arena_t          shader_arena;

    // NOTE(Sleepster): Layouts for each of the descriptor sets in the shader 
    VkDescriptorSetLayout  *layouts;
    u32                     descriptor_set_count;

    VkPushConstantRange    *push_constants;
    u32                     push_constant_count;

    vulkan_shader_stage_t  *stages;
    u32                     stage_count;
    
    VkPipelineBindPoint     pipeline_type;
    VkPipelineLayout        pipeline_layout;
    VkPipeline              pipeline;

    vulkan_shader_binding_t bindings[MAX_DESCRIPTOR_SET_BINDINGS];
    u32                     binding_count;
};

vulkan_shader_t vk_backend_shader_create(vulkan_context_t *vulkan_context, string_t shader_source);

#endif // VK_BACKEND_SHADER_H

