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

constexpr u32 MAX_DESCRIPTOR_SET_BINDINGS  = 5;
constexpr u32 MAX_SHADER_PIPELINE_COUNT    = 1021;

struct vulkan_shader_stage_t
{
    VkShaderModule                  handle;
    VkPipelineShaderStageCreateInfo pipeline_stage_create_info;
};

struct vulkan_shader_binding_t
{
    VkDescriptorType           type;
    u64                        buffer_hash_index;
    u32                        descriptor_count;
    string_t                   name;

};

struct vulkan_shader_t 
{
    u32                                  shader_id;
    string_t                             source;
    memory_arena_t                       shader_arena;

    // NOTE(Sleepster): Layouts for each of the descriptor sets in the shader 
    VkDescriptorSetLayout               *layouts;
    u32                                  descriptor_set_count;

    VkPushConstantRange                 *push_constants;
    u32                                  push_constant_count;

    vulkan_shader_stage_t               *stages;
    u32                                  stage_count;
    
    VkPipelineBindPoint                  pipeline_type;
    VkPipelineLayout                     pipeline_layout;
    VkVertexInputBindingDescription      vertex_buffer_binding_desc[4];
    VkVertexInputAttributeDescription    buffer_attributes[12];
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state;
    VkPipeline                           default_pipeline;

    VkPipeline                           pipeline;

    HashTable_t(VkPipeline)              pipeline_hash;
    u32                                  used_pipeline_indices[MAX_SHADER_PIPELINE_COUNT];
    u32                                  used_pipeline_count;

    vulkan_shader_binding_t              bindings[MAX_DESCRIPTOR_SET_BINDINGS];
    u32                                  binding_count;
};

vulkan_shader_t vk_backend_shader_create(vulkan_context_t *vulkan_context, string_t shader_source);

#endif // VK_BACKEND_SHADER_H

