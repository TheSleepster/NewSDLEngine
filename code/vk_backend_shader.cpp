/* ========================================================================
   $File: vk_backend_shader.cpp $
   $Date: February 18 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <vk_backend_shader.h>
#include <spirv_reflect.h>

#include <s_renderer.h>

VkDescriptorType 
vk_backend_spv_reflect_to_vulkan_descriptor(SpvReflectDescriptorType spv_type)
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
vk_backend_spv_shader_stage_to_vulkan_stage(SpvReflectShaderStageFlagBits spv_stage)
{
    VkShaderStageFlags result = 0;
    
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)                  result |= VK_SHADER_STAGE_VERTEX_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT)    result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT)                result |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT)                result |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if(spv_stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT)                 result |= VK_SHADER_STAGE_COMPUTE_BIT;
    
    return(result);
}

vulkan_shader_t
vk_backend_shader_create(vulkan_context_t *vulkan_context, string_t shader_source)
{
    vulkan_shader_t result = {};
    result.shader_arena = c_arena_create(MB(5));

    SpvReflectShaderModule module;
    SpvReflectResult error = spvReflectCreateShaderModule(shader_source.count, shader_source.data, &module);
    if(error != SPV_REFLECT_RESULT_SUCCESS)
    {
        log_error("Could not initialize spirv_reflect reflect data for this module...\n");
    }

    if(module.entry_point_count > 0)
    {
        result.stages = c_arena_push_array(&result.shader_arena, vulkan_shader_stage_t, module.entry_point_count);
        Assert(result.stages);
    }

    result.descriptor_set_count = module.descriptor_set_count;
    result.push_constant_count  = module.push_constant_block_count;
    result.stage_count          = module.entry_point_count;
    if(result.descriptor_set_count > 0)
    {
        result.layouts = c_arena_push_array(&result.shader_arena, VkDescriptorSetLayout, result.descriptor_set_count);
        Assert(result.layouts);
    }
    if(result.push_constant_count > 0)
    {
        result.push_constants = c_arena_push_array(&result.shader_arena, VkPushConstantRange, result.push_constant_count);
        Assert(result.push_constants);
    }

    bool8 is_compute_shader = false;

    // NOTE(Sleepster): Iterate the entry points 
    VkShaderStageFlags set_stage_flags[MAX_DESCRIPTOR_SET_BINDINGS] = {};
    for(u32 entry_index = 0;
        entry_index < module.entry_point_count;
        ++entry_index)
    {
        SpvReflectEntryPoint  *entry_point         = module.entry_points + entry_index;
        VkShaderStageFlags     current_stage_type = vk_backend_spv_shader_stage_to_vulkan_stage(entry_point->shader_stage);
        vulkan_shader_stage_t *stage_info         = result.stages + entry_index;

        if(current_stage_type == VK_SHADER_STAGE_COMPUTE_BIT) is_compute_shader = true;

        // NOTE(Sleepster): Set the push constant's stage flags and range data 
        for(u32 push_constant_index = 0;
            push_constant_index < result.push_constant_count;
            ++push_constant_index)
        {
            SpvReflectBlockVariable *constant_block = module.push_constant_blocks + push_constant_index;
            VkPushConstantRange     *push_constant  = result.push_constants       + push_constant_index;
            push_constant->offset      = constant_block->offset;
            push_constant->size        = constant_block->padded_size;
            push_constant->stageFlags |= current_stage_type;
        }

        for(u32 descriptor_set_index = 0;
            descriptor_set_index < result.descriptor_set_count;
            ++descriptor_set_index)
        {
            Assert(descriptor_set_index < MAX_DESCRIPTOR_SET_BINDINGS);
            set_stage_flags[descriptor_set_index] |= current_stage_type;
        }


        result.pipeline_type = is_compute_shader ? VK_PIPELINE_BIND_POINT_COMPUTE : 
                                                   VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.pCode    = (u32*)shader_source.data;
        create_info.codeSize = shader_source.count;

        vkAssert(vkCreateShaderModule(vulkan_context->device,
                                     &create_info,
                                      vulkan_context->cpu_allocation_callbacks,
                                     &stage_info->handle));
        stage_info->pipeline_stage_create_info = (VkPipelineShaderStageCreateInfo) {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = (VkShaderStageFlagBits)current_stage_type,
            .module = stage_info->handle,
            .pName  = entry_point->name
        };
    }

    // NOTE(Sleepster): This should be fine for getting the descriptor set data for now.
    // I can't really imagine why we'd need better.
    for(u32 descriptor_set_index = 0;
        descriptor_set_index < result.descriptor_set_count;
        ++descriptor_set_index)
    {
        Assert(descriptor_set_index < MAX_DESCRIPTOR_SET_BINDINGS);

        VkDescriptorSetLayout   *layout      = result.layouts + descriptor_set_index;
        SpvReflectDescriptorSet *set         = module.descriptor_sets  + descriptor_set_index;
        VkShaderStageFlags       stage_flags = set_stage_flags[descriptor_set_index];

        // NOTE(Sleepster): Set up each of the descriptor set bindings, this is capped at 5 right now,
        // but that should be okay since we just need to create these here for vkCreateDescriptorSetLayout
        // and then never use this information anywhere else ever again (I hope)
        u32 binding_count = set->binding_count;
        VkDescriptorSetLayoutBinding bindings[MAX_DESCRIPTOR_SET_BINDINGS] = {};
        for(u32 binding_index = 0;
            binding_index < binding_count;
            ++binding_index)
        {
            SpvReflectDescriptorBinding  *binding     = set->bindings[binding_index];
            VkDescriptorSetLayoutBinding *set_binding = bindings + binding->binding;

            set_binding->stageFlags         = stage_flags;
            set_binding->binding            = binding->binding;
            set_binding->descriptorType     = vk_backend_spv_reflect_to_vulkan_descriptor(binding->descriptor_type);
            set_binding->descriptorCount    = binding->count;
            set_binding->pImmutableSamplers = null;

            vulkan_shader_binding_t *shader_binding = result.bindings + binding_index;
            Assert(shader_binding);
            
            // TODO(Sleepster): For now we just assume that the size of the hahs table won't arbitrarily change
            // Bad.
            shader_binding->type               = set_binding->descriptorType;
            shader_binding->name               = STR(binding->name);
            shader_binding->cpu_buffer         = null;
            shader_binding->buffer_hash_index  = c_fnv_hash_value(shader_binding->name.data, shader_binding->name.count);
            shader_binding->buffer_hash_index %= MAX_CONSTANT_BUFFERS;
        }

        VkDescriptorSetLayoutCreateInfo layout_create_info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = binding_count,
            .pBindings    = bindings,
        };
        vkAssert(vkCreateDescriptorSetLayout(vulkan_context->device,
                                             &layout_create_info,
                                             null,
                                             layout));
    }

    vk_backend_create_render_pipeline(vulkan_context, &result, false);

    return(result);
}
