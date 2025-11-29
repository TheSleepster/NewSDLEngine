/* ========================================================================
   $File: main.cpp $
   $Date: November 28 2025 06:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>

#include <stdio.h>

#include <c_types.h>
#include <c_math.h>

struct string_t 
{
    byte *data;
    u32   count;
};

#define MAX_VERTICES (1000)
struct vertex_t
{
    vec4_t position;
};

string_t 
c_read_file(char *path)
{
    string_t result;

    FILE *file_handle = fopen(path, "rb");
    Expect(file_handle != null);

    fseek(file_handle, 0, SEEK_END);
    u32 file_size = ftell(file_handle);
    fseek(file_handle, 0, SEEK_SET);

    Expect(file_size > 0);

    result.count = file_size;
    result.data  = (byte*)AllocSize(sizeof(byte) * file_size);
    Expect(result.data);

    fread(result.data, 
          sizeof(byte), 
          file_size, 
          file_handle); 
    fclose(file_handle);

    return(result);
}

int
main(void)
{
    if(SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Window *window = SDL_CreateWindow("Window", 1280, 720, SDL_WINDOW_BORDERLESS);
        if(window)
        {
            SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, null);
            Expect(device);

            Expect(SDL_ClaimWindowForGPUDevice(device, window));

            string_t vertex_shader_code   = c_read_file("../code/shaders/test_vert.spv");
            string_t fragment_shader_code = c_read_file("../code/shaders/test_frag.spv");

            SDL_GPUShaderCreateInfo v_shader_info = {};
            v_shader_info.code_size  = vertex_shader_code.count;
            v_shader_info.code       = vertex_shader_code.data;
            v_shader_info.entrypoint = "vertex_main";
            v_shader_info.format     = SDL_GPU_SHADERFORMAT_SPIRV;
            v_shader_info.stage      = SDL_GPU_SHADERSTAGE_VERTEX;

            SDL_GPUShaderCreateInfo f_shader_info = {};
            f_shader_info.code_size  = fragment_shader_code.count;
            f_shader_info.code       = fragment_shader_code.data;
            f_shader_info.entrypoint = "fragment_main";
            f_shader_info.format     = SDL_GPU_SHADERFORMAT_SPIRV;
            f_shader_info.stage      = SDL_GPU_SHADERSTAGE_FRAGMENT;

            SDL_GPUShader *v_shader = SDL_CreateGPUShader(device, &v_shader_info);
            SDL_GPUShader *f_shader = SDL_CreateGPUShader(device, &f_shader_info);

            Expect(v_shader);
            Expect(f_shader);
            
            SDL_GPUBufferCreateInfo v_buffer_info = {};
            v_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            v_buffer_info.size  = sizeof(vertex_t) * MAX_VERTICES;
            SDL_GPUBuffer *vertex_buffer = SDL_CreateGPUBuffer(device, &v_buffer_info);
            Expect(vertex_buffer);

            SDL_GPUColorTargetDescription first_color_target = {
                .format = SDL_GetGPUSwapchainTextureFormat(device, window),
            };

            SDL_GPUVertexBufferDescription vertex_buffer_desc = {};
            vertex_buffer_desc.slot = 0;
            vertex_buffer_desc.pitch = sizeof(vertex_t);
            vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
            vertex_buffer_desc.instance_step_rate = 0;

            SDL_GPUVertexAttribute vertex_attributes[1] = {};
            vertex_attributes[0].location = 0;
            vertex_attributes[0].buffer_slot = 0;
            vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
            vertex_attributes[0].offset = 0;


            SDL_GPUVertexInputState vertex_input_state = {};
            vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
            vertex_input_state.num_vertex_buffers = 1;
            vertex_input_state.vertex_attributes = vertex_attributes;
            vertex_input_state.num_vertex_attributes = 1;

            SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
                .vertex_shader      = v_shader,
                .fragment_shader    = f_shader,
                .vertex_input_state = vertex_input_state,
                .primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
                .target_info = {
                    .color_target_descriptions = &first_color_target,
                    .num_color_targets = 1,
                },
            };

            pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
            SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
            printf("Error: '%s'...\n", SDL_GetError());
            Expect(pipeline);
        }
        else
        {
            printf("Error... Failure to create the SDL Window...: '%s'\n", SDL_GetError());
        }
    }
    else
    {
        printf("Error... Failure to init SDL3: '%s'\n", SDL_GetError());
    }

    return(0);
}
