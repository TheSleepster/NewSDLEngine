/* ========================================================================
   $File: vk_backend_shader.cpp $
   $Date: February 18 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <spirv_reflect.h>

#include <vk_backend_shader.h>
#include <s_renderer.h>

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(shader_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

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

internal_api u32 
vk_backend_get_vk_format_size(VkFormat format)
{
    switch(format)
    {
        // 8-bit
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        {
            return(1);
        }break;

        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        {
            return(2);
        }break;

        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
        {
            return(4);
        }break;

        // 16-bit
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
        {
            return(2);
        }break;

        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        {
            return(4);
        }break;

        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        {
            return(8);
        }break;

        // 32-bit
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        {
            return(4);
        }break;

        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        {
            return(8);
        }break;

        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        {
            return(12);
        }break;

        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        {
            return(16);
        }break;

        // 64-bit
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
        {
            return(8);
        }break;

        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
        {
            return(16);
        }break;

        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        {
            return(24);
        }break;

        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        {
            return(32);
        }break;

        default:
        {
            Expect(false, "Unhandled VkFormat in vk_backend_get_vk_format_size");
            return(0);
        }break;
    }
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
    //
    // The goal here is to set the stage flags for each of the shader modules, extract the push constants on per-stage basis,
    // and create the shader module for this specific shader stage.
    VkShaderStageFlags set_stage_flags[MAX_DESCRIPTOR_SET_BINDINGS] = {};
    for(u32 entry_index = 0;
        entry_index < module.entry_point_count;
        ++entry_index)
    {
        SpvReflectEntryPoint  *entry_point        = module.entry_points + entry_index;
        VkShaderStageFlags     current_stage_type = vk_backend_spv_shader_stage_to_vulkan_stage(entry_point->shader_stage);
        vulkan_shader_stage_t *stage_info         = result.stages + entry_index;

        // NOTE(Sleepster): 
        // If it's a compute shader, mark it as such. 
        // If it's a vertex shader, extract the vertex buffer information
        if(current_stage_type == VK_SHADER_STAGE_COMPUTE_BIT) is_compute_shader = true;
        if(current_stage_type == VK_SHADER_STAGE_VERTEX_BIT)
        {
            VkVertexInputBindingDescription *current_vertex_buffer     = null;
            VkVertexInputRate                current_buffer_input_rate = VK_VERTEX_INPUT_RATE_VERTEX;

            u32 buffer_attribute_count       = 0;
            u32 current_vertex_buffer_stride = 0;

            s32 vertex_buffer_count = -1;
            string_t current_structure_name = {};
            for(u32 index = 0;
                index < entry_point->input_variable_count;
                ++index)
            {
                SpvReflectInterfaceVariable *interface = entry_point->input_variables[index];
                if((interface->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) == 0)
                {
                    string_t fullname = STR(interface->name);
                    u32      member_access_token = c_string_find_first_char_from_left(fullname,  '.');

                    string_t structure_name = c_string_sub_from_left(fullname,  member_access_token);
                    string_t member_name    = c_string_sub_from_right(fullname, member_access_token);
                    if(!c_string_compare(structure_name, current_structure_name))
                    {
                        if(current_vertex_buffer)
                        {
                            current_vertex_buffer->stride    = current_vertex_buffer_stride;
                            current_vertex_buffer->binding   = vertex_buffer_count;
                            current_vertex_buffer->inputRate = current_buffer_input_rate;

                            current_vertex_buffer_stride = 0;
                        }

                        current_buffer_input_rate = member_name.data[0] == 'i' ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                        current_structure_name    = structure_name;
                        vertex_buffer_count++;
                    }
                    current_vertex_buffer = result.vertex_buffer_binding_desc + vertex_buffer_count;

                    VkFormat attrib_format = (VkFormat)interface->format;
                    VkVertexInputAttributeDescription *attribute = result.buffer_attributes + buffer_attribute_count;
                    attribute->binding  = vertex_buffer_count;
                    attribute->location = buffer_attribute_count;
                    attribute->offset   = current_vertex_buffer_stride;
                    attribute->format   = attrib_format;

                    current_vertex_buffer_stride += vk_backend_get_vk_format_size(attrib_format);
                    ++buffer_attribute_count;
                }
            }

            // NOTE(Sleepster): 
            // We have to fill in the data right here before we're finished and set the pipeline_vertex_input_state, 
            // otherwise we just miss the final buffer. 
            if(current_vertex_buffer)
            {
                current_vertex_buffer->stride    = current_vertex_buffer_stride;
                current_vertex_buffer->binding   = vertex_buffer_count;
                current_vertex_buffer->inputRate = current_buffer_input_rate;
            }

            // NOTE(Sleepster): This is stored so that we can create pipelines as needed later on... 
            result.pipeline_vertex_input_state = {
                .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount   = (u32)(vertex_buffer_count + 1),
                .pVertexBindingDescriptions      = result.vertex_buffer_binding_desc,
                .vertexAttributeDescriptionCount = buffer_attribute_count,
                .pVertexAttributeDescriptions    = result.buffer_attributes,
            };
        }

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

    // NOTE(Sleepster): 
    // Graphics pipelines need a complete hash of pipeline data,
    // while compute shaders are fine with just one.
    if(result.pipeline_type == VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        c_hash_table_init(&result.pipeline_hash, 
                          MAX_SHADER_PIPELINE_COUNT, 
                          &result.shader_arena,
                          shader_arena_allocate,
                          null);
    }
    else
    {
        c_hash_table_init(&result.pipeline_hash, 
                          1, 
                          &result.shader_arena,
                          shader_arena_allocate,
                          null);
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
            
            // NOTE(Sleepster): For now we know that the size of the hash table for our constant buffers won't arbitrarily change
            shader_binding->type               = set_binding->descriptorType;
            shader_binding->name               = STR(binding->name);
            shader_binding->descriptor_count   = binding->count;
            shader_binding->buffer_hash_index  = c_fnv_hash_value(shader_binding->name.data, shader_binding->name.count);
            shader_binding->buffer_hash_index %= MAX_CONSTANT_BUFFERS;

            ++result.binding_count;
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

    // NOTE(Sleepster): 
    // Create the pipeline layout here, it's easy to just do it in place and doesn't really
    // cause problems.
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = result.descriptor_set_count,
        .pSetLayouts            = result.layouts,
        .pushConstantRangeCount = result.push_constant_count,
        .pPushConstantRanges    = result.push_constants,
    };
    vkAssert(vkCreatePipelineLayout(vulkan_context->device,
                                   &pipeline_layout_info,
                                    vulkan_context->cpu_allocation_callbacks,
                                   &result.pipeline_layout));

    if(result.pipeline_type == VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        // NOTE(Sleepster): Create the base pipeline for the pipeline hash. 
        string_t pipeline_key_data = {
            .data  = (u8*)&g_pipeline_default_state_key,
            .count = sizeof(g_pipeline_default_state_key)
        };
        u64 pipeline_state_hash = c_fnv_hash_value(pipeline_key_data.data, pipeline_key_data.count);
        pipeline_state_hash %= MAX_SHADER_PIPELINE_COUNT; 

        result.pipeline_hash.data[pipeline_state_hash] = vk_backend_create_render_pipeline(vulkan_context, 
                                                                                           &result, 
                                                                                           &g_pipeline_default_rasterization_state, 
                                                                                           &g_pipeline_default_depth_stencil_state,
                                                                                           &g_pipeline_default_blend_settings,
                                                                                           &result.pipeline_vertex_input_state);
        result.default_pipeline = result.pipeline_hash.data[pipeline_state_hash];
    }
    else if(result.pipeline_type == VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        VkComputePipelineCreateInfo pipeline_info = {};
        pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout = result.pipeline_layout;
        pipeline_info.stage  = result.stages->pipeline_stage_create_info;

        vkAssert(vkCreateComputePipelines(vulkan_context->device, null, 1, &pipeline_info, null, &result.pipeline_hash.data[0]));
    }

    return(result);
}
