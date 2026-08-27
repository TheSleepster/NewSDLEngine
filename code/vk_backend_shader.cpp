/* ========================================================================
   $File: vk_backend_shader.cpp $
   $Date: February 18 2026 04:23 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <spirv_reflect.h>

#include <vk_backend_shader.h>
#include <s_RHI_core.h>

#include <c_tokenizer.h>

internal_api void*
shader_arena_allocate(void *allocator, u32 allocation_size)
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
vk_backend_shader_create_spirv_reflect(vulkan_context_t *vulkan_context, string_t shader_source)
{
    vulkan_shader_t result = {};
    result.shader_arena = c_arena_create(MB(10));

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

            result.vertex_buffer_binding_descs = c_arena_push_array(&result.shader_arena, VkVertexInputBindingDescription,   MAX_BUFFER_BINDING_DESCS);
            result.buffer_attributes           = c_arena_push_array(&result.shader_arena, VkVertexInputAttributeDescription, MAX_BUFFER_ATTRIBUTES);

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

                        Assert(vertex_buffer_count <= (s32)MAX_BUFFER_BINDING_DESCS);
                    }
                    current_vertex_buffer = result.vertex_buffer_binding_descs + vertex_buffer_count;

                    VkFormat attrib_format = (VkFormat)interface->format;
                    VkVertexInputAttributeDescription *attribute = result.buffer_attributes + buffer_attribute_count;
                    attribute->binding  = vertex_buffer_count;
                    attribute->location = buffer_attribute_count;
                    attribute->offset   = current_vertex_buffer_stride;
                    attribute->format   = attrib_format;

                    current_vertex_buffer_stride += vk_backend_get_vk_format_size(attrib_format);
                    ++buffer_attribute_count;

                    Assert(buffer_attribute_count <= MAX_BUFFER_ATTRIBUTES);
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
                .pVertexBindingDescriptions      = result.vertex_buffer_binding_descs,
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
        result.pipeline_hash = c_hash_table_create<VkPipeline>(MAX_SHADER_PIPELINE_COUNT, 
                                                               &result.shader_arena,
                                                               shader_arena_allocate,
                                                               null);
    }
    else
    {
        result.pipeline_hash = c_hash_table_create<VkPipeline>(1, 
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
            shader_binding->buffer_hash_index  = c_hash_table_hash_key(string_t{shader_binding->name.data, shader_binding->name.count});
            shader_binding->buffer_hash_index %= RHI_MAX_CONSTANT_BUFFERS;

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
        u64 pipeline_state_hash = c_hash_table_hash_key(pipeline_key_data);
        pipeline_state_hash %= MAX_SHADER_PIPELINE_COUNT; 

        result.shader_id = pipeline_state_hash;
        (result.pipeline_hash.items[pipeline_state_hash]).item = vk_backend_create_render_pipeline(vulkan_context, 
                                                                                                   &result, 
                                                                                                   vulkan_context->primary_renderpass,
                                                                                                   &g_pipeline_default_rasterization_state, 
                                                                                                   &g_pipeline_default_depth_stencil_state,
                                                                                                   &g_pipeline_default_blend_settings,
                                                                                                   &result.pipeline_vertex_input_state);
        result.default_pipeline = (result.pipeline_hash.items[pipeline_state_hash]).item;
    }
    else if(result.pipeline_type == VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        VkComputePipelineCreateInfo pipeline_info = {};
        pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout = result.pipeline_layout;
        pipeline_info.stage  = result.stages->pipeline_stage_create_info;

        vkAssert(vkCreateComputePipelines(vulkan_context->device, null, 1, &pipeline_info, null, &(result.pipeline_hash.items[0]).item));
    }

    return(result);
}

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>

static Slang::ComPtr<slang::IGlobalSession> global_session;

internal_api VkShaderStageFlagBits 
slang_stage_to_vk_stage(SlangStage stage)
{
    VkShaderStageFlagBits result = VK_SHADER_STAGE_ALL;
    switch(stage)
    {
        case SLANG_STAGE_VERTEX:         { result =  VK_SHADER_STAGE_VERTEX_BIT;                  }break;
        case SLANG_STAGE_FRAGMENT:       { result =  VK_SHADER_STAGE_FRAGMENT_BIT;                }break;
        case SLANG_STAGE_COMPUTE:        { result =  VK_SHADER_STAGE_COMPUTE_BIT;                 }break;
        case SLANG_STAGE_GEOMETRY:       { result =  VK_SHADER_STAGE_GEOMETRY_BIT;                }break;
        case SLANG_STAGE_HULL:           { result =  VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;    }break;
        case SLANG_STAGE_DOMAIN:         { result =  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; }break;
        case SLANG_STAGE_MISS:           { result =  VK_SHADER_STAGE_MISS_BIT_KHR;                }break;
        case SLANG_STAGE_CLOSEST_HIT:    { result =  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;         }break;
        case SLANG_STAGE_RAY_GENERATION: { result =  VK_SHADER_STAGE_RAYGEN_BIT_KHR;              }break;
        case SLANG_STAGE_ANY_HIT:        { result =  VK_SHADER_STAGE_ANY_HIT_BIT_KHR;             }break;
        case SLANG_STAGE_INTERSECTION:   { result =  VK_SHADER_STAGE_INTERSECTION_BIT_KHR;        }break;
        case SLANG_STAGE_MESH:           { result =  VK_SHADER_STAGE_MESH_BIT_EXT;                }break;
        case SLANG_STAGE_AMPLIFICATION:  { result =  VK_SHADER_STAGE_TASK_BIT_EXT;                }break;
        default:                         { InvalidCodePath;                                       }break;
    }

    return(result);
}

internal_api VkDescriptorType
slang_type_to_vulkan_type(slang::TypeReflection *type)
{
    VkDescriptorType result = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    if(type->getKind() == slang::TypeReflection::Kind::Array)
    {
        type = type->getElementType();
    }

    slang::TypeReflection::Kind kind = type->getKind();
    if(kind == slang::TypeReflection::Kind::ConstantBuffer)
    {
        result = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    else if(kind == slang::TypeReflection::Kind::SamplerState)
    {
        result = VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    else if(kind == slang::TypeReflection::Kind::Resource)
    {
        SlangResourceAccess access     = type->getResourceAccess();
        SlangResourceShape  dimensions = type->getResourceShape();
        SlangResourceShape  base       = (SlangResourceShape)(dimensions & SLANG_RESOURCE_BASE_SHAPE_MASK);

        bool8 is_combined_sampler    = (dimensions & SLANG_BINDING_TYPE_COMBINED_TEXTURE_SAMPLER) != 0;
        bool8 is_read_write_accessed = (access == SLANG_RESOURCE_ACCESS_READ_WRITE ||
                                       access == SLANG_RESOURCE_ACCESS_RASTER_ORDERED);
        if(base == SLANG_STRUCTURED_BUFFER || base == SLANG_BYTE_ADDRESS_BUFFER)
        {
            result = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
        else if(is_combined_sampler)
        {
            result = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
        else if(is_read_write_accessed)
        {
            result = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        else
        {
            result = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        }
    }

    return(result);
}

internal_api slang::ParameterCategory
slang_binding_category(slang::TypeReflection *type)
{
    slang::ParameterCategory result = slang::ParameterCategory::None;

    // NOTE(Sleepster): If it's an array, figure out what kind of array. 
    if(type->getKind() == slang::TypeReflection::Kind::Array)
    {
        type = type->getElementType();
    }

    slang::TypeReflection::Kind kind = type->getKind();
    if(kind == slang::TypeReflection::Kind::ConstantBuffer ||
       kind == slang::TypeReflection::Kind::ParameterBlock)
    {
        result = slang::ParameterCategory::ConstantBuffer;
    }
    else if(kind == slang::TypeReflection::Kind::SamplerState)
    {
        result = slang::ParameterCategory::SamplerState;
    }
    else if(kind == slang::TypeReflection::Kind::Resource)
    {
        SlangResourceAccess access = type->getResourceAccess();
        if(access == SLANG_RESOURCE_ACCESS_READ_WRITE ||
           access == SLANG_RESOURCE_ACCESS_RASTER_ORDERED)
        {
            result = slang::ParameterCategory::UnorderedAccess;
        }
        else
        {
            result = slang::ParameterCategory::ShaderResource;
        }
    }
    else
    {
        InvalidCodePath;
    }

    return(result);
}

internal_api VkFormat 
slang_type_to_vulkan_format(slang::TypeReflection *type_data, slang::TypeLayoutReflection *type_layout)
{
    VkFormat result = VK_FORMAT_UNDEFINED;

    Assert(type_layout);
    Assert(type_data);

    slang::TypeReflection::Kind type_kind = type_data->getKind();
    slang::TypeReflection::ScalarType scalar = slang::TypeReflection::ScalarType::None;

    // NOTE(Sleepster): 4 wide is 128 bytes (or float4) 
    u32 scalar_width = 1;

    if(type_kind == slang::TypeReflection::Kind::Scalar)
    {
        scalar = type_data->getScalarType();
    }
    else if(type_kind == slang::TypeReflection::Kind::Vector)
    {
        scalar = type_data->getElementType()->getScalarType();
        scalar_width = (u32)type_data->getElementCount();
    }
    else
    {
        // NOTE(Sleepster): If it's neither a scalar or a vector, why are we here? 
        InvalidCodePath;
    }

    if(scalar == slang::TypeReflection::ScalarType::Float32)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R32_SFLOAT;          }break;
            case 2: { result = VK_FORMAT_R32G32_SFLOAT;       }break;
            case 3: { result = VK_FORMAT_R32G32B32_SFLOAT;    }break;
            case 4: { result = VK_FORMAT_R32G32B32A32_SFLOAT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::Float16)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R16_SFLOAT;          }break;
            case 2: { result = VK_FORMAT_R16G16_SFLOAT;       }break;
            case 3: { result = VK_FORMAT_R16G16B16_SFLOAT;    }break;
            case 4: { result = VK_FORMAT_R16G16B16A16_SFLOAT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::Int32)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R32_SINT;          }break;
            case 2: { result = VK_FORMAT_R32G32_SINT;       }break;
            case 3: { result = VK_FORMAT_R32G32B32_SINT;    }break;
            case 4: { result = VK_FORMAT_R32G32B32A32_SINT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::UInt32)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R32_UINT;          }break;
            case 2: { result = VK_FORMAT_R32G32_UINT;       }break;
            case 3: { result = VK_FORMAT_R32G32B32_UINT;    }break;
            case 4: { result = VK_FORMAT_R32G32B32A32_UINT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::Int16)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R16_SINT;          }break;
            case 2: { result = VK_FORMAT_R16G16_SINT;       }break;
            case 3: { result = VK_FORMAT_R16G16B16_SINT;    }break;
            case 4: { result = VK_FORMAT_R16G16B16A16_SINT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::UInt16)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R16_UINT;          }break;
            case 2: { result = VK_FORMAT_R16G16_UINT;       }break;
            case 3: { result = VK_FORMAT_R16G16B16_UINT;    }break;
            case 4: { result = VK_FORMAT_R16G16B16A16_UINT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::Int8)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R8_SINT;       }break;
            case 2: { result = VK_FORMAT_R8G8_SINT;     }break;
            case 3: { result = VK_FORMAT_R8G8B8_SINT;   }break;
            case 4: { result = VK_FORMAT_R8G8B8A8_SINT; }break;
        }
    }
    else if(scalar == slang::TypeReflection::ScalarType::UInt8)
    {
        switch(scalar_width)
        {
            case 1: { result = VK_FORMAT_R8_UINT;       }break;
            case 2: { result = VK_FORMAT_R8G8_UINT;     }break;
            case 3: { result = VK_FORMAT_R8G8B8_UINT;   }break;
            case 4: { result = VK_FORMAT_R8G8B8A8_UINT; }break;
        }
    }

    return(result);
}

vulkan_shader_t
vk_backend_shader_create_slang_reflect(vulkan_context_t *vulkan_context, string_t shader_source)
{
    vulkan_shader_t result = {};
    result.shader_arena = c_arena_create(MB(10));

    // NOTE(Sleepster): Create global session if it's invalid 
    if(global_session == nullptr)
    {
        if(SLANG_FAILED(slang::createGlobalSession(global_session.writeRef())))
        {
            log_fatal("Could not create the slang::IGlobalSession");
        }
        else
        {
            log_info("Slang IGlobalSession created successfully...\n");
        }
    }

    // NOTE(Sleepster): Set the compilation target information 
    slang::TargetDesc target_desc = {};
    target_desc.format  = SLANG_SPIRV;
    target_desc.profile = global_session->findProfile("sm_6_3");
    target_desc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

    // NOTE(Sleepster): Setup the session's compilation flags and other information 
    slang::SessionDesc session_desc = {};
    session_desc.targets     = &target_desc;
    session_desc.targetCount = 1;

    slang::CompilerOptionEntry options[3] = {};
    options[0].name              = slang::CompilerOptionName::MatrixLayoutColumn;
    options[0].value.kind        = slang::CompilerOptionValueKind::Int;
    options[0].value.intValue0   = 1;

    options[1].name              = slang::CompilerOptionName::Optimization;
    options[1].value.kind        = slang::CompilerOptionValueKind::Int;
    options[1].value.intValue0   = SLANG_OPTIMIZATION_LEVEL_NONE;

    options[2].name              = slang::CompilerOptionName::DebugInformation;
    options[2].value.kind        = slang::CompilerOptionValueKind::Int;
    options[2].value.intValue0   = SLANG_DEBUG_INFO_LEVEL_STANDARD;

    session_desc.compilerOptionEntries     = options;
    session_desc.compilerOptionEntryCount  = 3;

    // NOTE(Sleepster): Create a child session from the global session 
    Slang::ComPtr<slang::ISession> session = {};
    Expect(!SLANG_FAILED(global_session->createSession(session_desc, session.writeRef())), "Failed to create the Slang::ISession");

    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule *shader_module = session->loadModuleFromSourceString("fucking hate this shit", null, (char*)shader_source.data, diagnostics.writeRef());
    if(diagnostics && diagnostics->getBufferSize() > 0)
    {
        log_error("[SLANG]: %s\n", (const char *)diagnostics->getBufferPointer());
    }

    s32 entry_point_count = shader_module->getDefinedEntryPointCount();
    Expect(entry_point_count > 0, "This file contains no entry_points...\n");
    if(entry_point_count > 0)
    {
        result.stages = c_arena_push_array(&result.shader_arena, vulkan_shader_stage_t, entry_point_count);
    }
    
    Slang::ComPtr<slang::IEntryPoint> entry_points[10];
    slang::IComponentType*            components[10 + 1];

    components[0] = shader_module;

    for(s32 entry_index = 0;
        entry_index < entry_point_count;
        ++entry_index)
    {
        Assert(!SLANG_FAILED(shader_module->getDefinedEntryPoint(entry_index, entry_points[entry_index].writeRef())));
        components[entry_index + 1] = entry_points[entry_index].get();
    }

    Slang::ComPtr<slang::IComponentType> shader_program;
    Assert(!SLANG_FAILED(session->createCompositeComponentType(components, (SlangInt)(entry_point_count + 1), shader_program.writeRef(), diagnostics.writeRef())));

    Slang::ComPtr<slang::IComponentType> linked_program;
    Assert(!SLANG_FAILED(shader_program->link(linked_program.writeRef(), diagnostics.writeRef())));

    slang::ProgramLayout *layout = linked_program->getLayout(0);
    Assert(layout);

    bool8 is_compute_shader = false;
    result.stage_count = entry_point_count;

    result.push_constant_count  = 0;
    result.descriptor_set_count = 0;

    VkShaderStageFlags descriptor_set_stage_flags[MAX_DESCRIPTOR_SET_BINDINGS] = {};

    s32 unique_descriptor_sets[MAX_DESCRIPTOR_SET_BINDINGS];
    memset(unique_descriptor_sets, -1, sizeof(s32) * MAX_DESCRIPTOR_SET_BINDINGS);

    u32 unique_set_count    = 0;
    u32 push_constant_count = 0;

    u32 param_count = layout->getParameterCount();
    for(u32 param_index = 0;
        param_index < param_count;
        ++param_index)
    {
        slang::VariableLayoutReflection *variable  = layout->getParameterByIndex(param_index);
        slang::TypeReflection           *type_data = variable->getTypeLayout()->getType();
        if(variable->getCategory() != slang::ParameterCategory::PushConstantBuffer)
        {
            slang::ParameterCategory binding_category = slang_binding_category(type_data);
            u32 set = (u32)variable->getBindingSpace(binding_category);

            bool8 found = false;
            for(u32 set_binding_index = 0;
                set_binding_index < MAX_DESCRIPTOR_SET_BINDINGS;
                ++set_binding_index)
            {
                if((u32)unique_descriptor_sets[set_binding_index] == set)
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                Assert(unique_set_count <= MAX_DESCRIPTOR_SET_BINDINGS);

                unique_descriptor_sets[unique_set_count] = set;
                ++unique_set_count;
            }
        }
        else
        {
            ++push_constant_count;
        }
    }

    result.push_constant_count  = push_constant_count;
    result.descriptor_set_count = unique_set_count;
    result.stage_count          = entry_point_count;
    if(result.descriptor_set_count > 0)
    {
        result.layouts = c_arena_push_array(&result.shader_arena, VkDescriptorSetLayout, result.descriptor_set_count);
        Assert(result.layouts);
    }

    if(result.push_constant_count > 0)
    {
        result.push_constants = c_arena_push_array(&result.shader_arena, VkPushConstantRange,   result.push_constant_count);
        Assert(result.push_constants);
    }
    Assert(result.stages);

    Slang::ComPtr<slang::IBlob>     *kernels  = c_arena_push_array(&result.shader_arena, Slang::ComPtr<slang::IBlob>,     entry_point_count);
    Slang::ComPtr<slang::IMetadata> *metadata = c_arena_push_array(&result.shader_arena, Slang::ComPtr<slang::IMetadata>, entry_point_count);
    for(s32 entry_index = 0;
        entry_index < entry_point_count;
        ++entry_index)
    {
        slang::EntryPointLayout *entry_point_layout = layout->getEntryPointByIndex(entry_index);
        VkShaderStageFlags       shader_stage       = slang_stage_to_vk_stage(layout->getEntryPointByIndex(entry_index)->getStage());
        vulkan_shader_stage_t   *stage_info         = result.stages + entry_index;

        Slang::ComPtr<slang::IBlob>     kernel = kernels[entry_index];
        Slang::ComPtr<slang::IMetadata> meta   = metadata[entry_index];

        SlangResult success = linked_program->getEntryPointCode(entry_index, 0, kernel.writeRef(), diagnostics.writeRef());
        Assert(!SLANG_FAILED(success));

        if(!diagnostics || diagnostics->getBufferSize() == 0)
        {
            success = linked_program->getEntryPointMetadata(entry_index, 0, meta.writeRef(), diagnostics.writeRef());
            Assert(!SLANG_FAILED(success));

            // NOTE(Sleepster): If this is a vertex shader, extract the vertex buffer data... 
            if(shader_stage == VK_SHADER_STAGE_COMPUTE_BIT) is_compute_shader = true;
            if(shader_stage == VK_SHADER_STAGE_VERTEX_BIT)
            {
                result.vertex_buffer_binding_descs = c_arena_push_array(&result.shader_arena, VkVertexInputBindingDescription,   MAX_BUFFER_BINDING_DESCS);
                result.buffer_attributes           = c_arena_push_array(&result.shader_arena, VkVertexInputAttributeDescription, MAX_BUFFER_ATTRIBUTES);

                u32  buffer_attribute_count       =  0;
                u32  current_vertex_buffer_stride =  0;
                s32  vertex_buffer_count          = -1;
                string_t current_structure_name   = {};
                VkVertexInputBindingDescription *current_vertex_buffer = null;

                VkVertexInputRate current_buffer_input_rate = VK_VERTEX_INPUT_RATE_VERTEX;
                for(u32 param_index = 0;
                    param_index < param_count;
                    ++param_index)
                {
                    slang::VariableLayoutReflection *variable = entry_point_layout->getParameterByIndex(param_index);
                    slang::ParameterCategory         category = variable->getCategory();
                    if(category == slang::ParameterCategory::VaryingInput ||
                       category == slang::ParameterCategory::Mixed)
                    {
                        slang::TypeLayoutReflection *type_layout = variable->getTypeLayout();
                        slang::TypeReflection       *type_data   = type_layout->getType();

                        // NOTE(Sleepster): This is the correct way to get the location... this API sucks and it makes me think they 
                        // kick puppies.
                        u32 param_base_location = (u32)variable->getOffset(slang::ParameterCategory::VaryingInput);

                        if(type_data->getKind() == slang::TypeReflection::Kind::Struct)
                        {
                            string_t structure_name = STR(type_layout->getName());
                            if(!c_string_compare(structure_name, current_structure_name))
                            {
                                if(current_vertex_buffer)
                                {
                                    current_vertex_buffer->stride    = current_vertex_buffer_stride;
                                    current_vertex_buffer->binding   = (u32)vertex_buffer_count;
                                    current_vertex_buffer->inputRate = current_buffer_input_rate;

                                    current_vertex_buffer_stride = 0;
                                    current_structure_name = c_string_make_copy(&gc->temporary_arena, structure_name);
                                }
                                current_buffer_input_rate = structure_name.data[0] == 'i' ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                                current_structure_name    = structure_name;
                                vertex_buffer_count++;

                                Assert(vertex_buffer_count < (s32)MAX_BUFFER_BINDING_DESCS);
                            }
                            current_vertex_buffer = result.vertex_buffer_binding_descs + vertex_buffer_count;

                            u32 field_count = type_data->getFieldCount();
                            for(u32 field_index = 0;
                                field_index < field_count;
                                ++field_index)
                            {
                                slang::VariableLayoutReflection *member = type_layout->getFieldByIndex(field_index);
                                slang::ParameterCategory category = member->getCategory();
                                if(category == slang::ParameterCategory::VaryingInput)
                                {
                                    VkFormat attrib_format = slang_type_to_vulkan_format(member->getType(), member->getTypeLayout());
                                    VkVertexInputAttributeDescription *attribute = result.buffer_attributes + buffer_attribute_count;
                                    attribute->binding  = vertex_buffer_count;
                                    attribute->location = param_base_location + (u32)member->getOffset(slang::ParameterCategory::VaryingInput);
                                    attribute->offset   = current_vertex_buffer_stride;
                                    attribute->format   = attrib_format;

                                    current_vertex_buffer_stride += vk_backend_get_vk_format_size(attrib_format);
                                    ++buffer_attribute_count;

                                    Assert(buffer_attribute_count <= MAX_BUFFER_ATTRIBUTES);
                                }
                            }
                        }
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
                    .pVertexBindingDescriptions      = result.vertex_buffer_binding_descs,
                    .vertexAttributeDescriptionCount = buffer_attribute_count,
                    .pVertexAttributeDescriptions    = result.buffer_attributes,
                };
            }

            // NOTE(Sleepster): Extract the push constant data from this stage
            u32 push_constant_index = 0;
            for(u32 param_index = 0;
                param_index < param_count; 
                ++param_index)
            {
                slang::VariableLayoutReflection *variable = layout->getParameterByIndex(param_index);
                if(variable->getCategory() == slang::ParameterCategory::PushConstantBuffer)
                {
                    slang::TypeLayoutReflection *type_layout = variable->getTypeLayout();
                    VkPushConstantRange         *range       = result.push_constants + push_constant_index;

                    range->offset      = (u32)variable->getOffset(slang::ParameterCategory::Uniform);
                    range->size        = (u32)type_layout->getSize(slang::ParameterCategory::Uniform);
                    range->stageFlags |= shader_stage;

                    ++push_constant_index;
                }
            }

            // NOTE(Sleepster): Set the descriptor set stage flags 
            for(u32 descriptor_set_index = 0;
                descriptor_set_index < result.descriptor_set_count;
                ++descriptor_set_index)
            {
                descriptor_set_stage_flags[descriptor_set_index] |= shader_stage;
            }

            size_t kernel_size = kernel->getBufferSize();

            VkShaderModuleCreateInfo create_info = {};
            create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.pCode    = (const u32*)kernel->getBufferPointer();
            create_info.codeSize = kernel_size;

            vkAssert(vkCreateShaderModule(vulkan_context->device,
                                          &create_info,
                                          vulkan_context->cpu_allocation_callbacks,
                                          &stage_info->handle));

            stage_info->pipeline_stage_create_info = (VkPipelineShaderStageCreateInfo) {
                .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage  = (VkShaderStageFlagBits)shader_stage,
                .module = stage_info->handle,
                .pName  = "main" 
            };

            result.pipeline_type = is_compute_shader ? VK_PIPELINE_BIND_POINT_COMPUTE : 
                                                       VK_PIPELINE_BIND_POINT_GRAPHICS;
        }
        else
        {
            log_error("\n[SLANG]: Failed to emit spirv for entry point: '%s'...\n",
                      entry_point_layout->getName());
        }
    }

    // NOTE(Sleepster): Extract the descriptor set layout information 
    for(u32 descriptor_set_index = 0;
        descriptor_set_index < result.descriptor_set_count;
        ++descriptor_set_index)
    {
        u32                current_set = unique_descriptor_sets[descriptor_set_index];
        VkShaderStageFlags stage_flags = descriptor_set_stage_flags[descriptor_set_index];

        u32 binding_count = 0;
        VkDescriptorSetLayoutBinding bindings[MAX_DESCRIPTOR_SET_BINDINGS] = {}; 

        for(u32 param_index = 0;
            param_index < param_count;
            ++param_index)
        {
            slang::VariableLayoutReflection *variable = layout->getParameterByIndex(param_index);
            slang::TypeReflection *type_data          = variable->getTypeLayout()->getType();
            slang::ParameterCategory binding_category = slang_binding_category(type_data);

            VkDescriptorSetLayoutBinding *binding = bindings + binding_count;

            if(variable->getCategory() != slang::ParameterCategory::PushConstantBuffer && 
          (u32)variable->getBindingSpace(binding_category) == current_set              &&
               binding_category != slang::ParameterCategory::None)
            {
                VkDescriptorType descriptor_type = slang_type_to_vulkan_type(type_data);
                Assert(descriptor_type != VK_DESCRIPTOR_TYPE_MAX_ENUM);

                u32 descriptor_count = 1;
                if(type_data->getKind() == slang::TypeReflection::Kind::Array)
                {
                    size_t elem_count  = type_data->getElementCount();
                    descriptor_count   = elem_count;
                }


                u32 binding_index = (u32)variable->getOffset(binding_category);
                binding->binding            = binding_index;
                binding->descriptorType     = descriptor_type;
                binding->descriptorCount    = descriptor_count;
                binding->stageFlags         = stage_flags;
                binding->pImmutableSamplers = null;

                vulkan_shader_binding_t *shader_binding = result.bindings + binding_index;
                shader_binding->type               = descriptor_type;
                shader_binding->descriptor_count   = descriptor_count;
                shader_binding->name               = STR(variable->getName());
                shader_binding->buffer_hash_index  = c_hash_table_hash_key(shader_binding->name);
                shader_binding->buffer_hash_index %= RHI_MAX_CONSTANT_BUFFERS;

                ++result.binding_count;
                ++binding_count;
            }
        }

        VkDescriptorSetLayoutCreateInfo layout_create_info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = binding_count,
            .pBindings    = bindings,
        };
        vkAssert(vkCreateDescriptorSetLayout(vulkan_context->device,
                                             &layout_create_info,
                                              null,
                                              result.layouts + descriptor_set_index));
    }

    // NOTE(Sleepster): 
    // Graphics pipelines need a complete hash of pipeline data,
    // while compute shaders are fine with just one.
    if(result.pipeline_type == VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        result.pipeline_hash = c_hash_table_create<VkPipeline>(MAX_SHADER_PIPELINE_COUNT, 
                                                               &result.shader_arena,
                                                               shader_arena_allocate,
                                                               null);
    }
    else
    {
        result.pipeline_hash = c_hash_table_create<VkPipeline>(1, 
                                                               &result.shader_arena,
                                                               shader_arena_allocate,
                                                               null);
    }

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
        u64 pipeline_state_hash = c_hash_table_hash_key(pipeline_key_data);
        pipeline_state_hash %= MAX_SHADER_PIPELINE_COUNT; 

        result.shader_id = pipeline_state_hash;
        (result.pipeline_hash.items[pipeline_state_hash]).item = vk_backend_create_render_pipeline(vulkan_context, 
                                                                                                   &result, 
                                                                                                   vulkan_context->primary_renderpass,
                                                                                                   &g_pipeline_default_rasterization_state, 
                                                                                                   &g_pipeline_default_depth_stencil_state,
                                                                                                   &g_pipeline_default_blend_settings,
                                                                                                   &result.pipeline_vertex_input_state);
        result.default_pipeline = (result.pipeline_hash.items[pipeline_state_hash]).item;
    }
    else if(result.pipeline_type == VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        VkComputePipelineCreateInfo pipeline_info = {};
        pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout = result.pipeline_layout;
        pipeline_info.stage  = result.stages->pipeline_stage_create_info;

        vkAssert(vkCreateComputePipelines(vulkan_context->device, null, 1, &pipeline_info, null, &(result.pipeline_hash.items[0]).item));
    }

    return(result);
}
