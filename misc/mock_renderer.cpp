/* ========================================================================
   $File: mock_renderer.cpp $
   $Date: March 01 2026 11:58 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#if 0 
// NOTE(Sleepster): 
// For the lightmap we need to run a forward clustered compute shader that will fill the light's 
// array of indices that index into the array of shadow geometry. The purpose of this array is to
// make the each of the lights only run occlusion checking over ONLY the peices of geometry that 
// they are affected by.
//
// This means that in the shader we'd have something like this:
//
// // light buffer
// uniform (layout = std140, binding = 0) buffer lights[] = {
//      render_light_t lights[]; <- read only
//      u32            shadow_indices[]; <- This is what the compute shader would write into
// };
//
// uniform (layout = std140, binding = 1) readonly buffer shadow_data = {
//      shadow_geometry shadows[];
// };
//
// There was a way to do this or something similar (perhaps a seperate buffer for the light indices)
// before, I just need to remember.

// NOTE(Sleepster): 
// Render all the items into the game target. The order looks like so:
//
// [PER-GAME RENDERING]
// Lightmap Contents: 
//     - Color buffer. 8Bit red texture. Full 1.0 value means max intensity, 0.0 means no effect.
//
// STEPS FOR THIS PHASE
// 1.) Run the lighting compute shader to get the appropriate data needed for overlaying lights
// 2.) Render the lights and their shadows into the lightmap using the shadow data processed by
//     the compute shader.
//
// [GAME RENDER TARGET]
// NOTE: Relies on lighting
// 
// Game Target Contents:
//      - Game Color Buffer (RGBA32 8 bits per pixel)
//      - Game Emmision Buffer (perhaps only another R8 texture?)
//      - Depth Buffer
//
// STEPS FOR THIS PHASE
// 1.) Render the game's opaque geometry into the game render_target, recording the brightest pixels
//     into one of it's buffers for later processing of emmision.
// 2.) Overlay the shadows and the lights over the scene, darkening items either receiving no light,
//     or very little.
// 3.) Perform some kind of "bloom" pass on the brightest pixels recorded from before for emmision
// 4.) Render the game's transparent geometry into this buffer.
//
// [POST FX TARGET (MUST BE THE SAME DIMENSIONS AS THE GAME TARGET)]
// Contents:
//      - Another RGBA32 (8 bit per color) texture
//      - Perhaps other textures.
//
// NOTE: Reads from the game pass's data
//
// STEPS FOR THIS PHASE
// 2.) Just run whatever post effects here into this texture using whatever input you need.
//
//
// [GAME EFFECT COMPOSITION TARGET]
// NOTE: relies on both the game texture and the postfx texture.  
// Game Target Contents:
//      - Just a 32bit 8 bit per channel color buffer 
//
// STEPS FOR THIS PHASE
// 1.) Composite the game texture and the post_fx_target. THESE TWO NEED TO BE THE SAME RESOLUTION
//
// [UI TARGET]
// NOTE: This rely's on nothing but the data used to render the UI, meaning it can be done
// in parallel
//
// Contents:
//      - An RGBA32 (8 bit per color) texture
//      - Depth buffer
//
// STEPS FOR THIS PHASE
// 1.) Render the ui into this texture
// 2.) Perform any effects needed using forward rendering.
//
// [FINAL TARGET (THIS IMAGE WILL BE BLIT TO THE SWAPCHAIN)]
//
// NOTE: Relies on both the game texture and the post-fx composed texture 
//
//  Contents:
//      - Whatever it needs, likely just a single RGBA32 (8 bit per color) texture.
//
// STEPS FOR THIS PHASE
// 1.) Use the ui target's depth buffer to compose the ui texture overtop of the game/postfx rneder
// target when appropriate.

// NOTE(Sleepster): INIT TIME 
render_target_t *game_target    = {};
render_target_t *lightmap       = {};
render_target_t *post_fx_target = {};
render_target_t *ui_target      = {};
render_target_t *final_target   = {};

image_create_info_t game_color_buffer_image_info = ...;
image_t game_color_buffer_image = s_renderer_image_create(renderer_state, &game_color_buffer_image_info);

image_create_info_t game_depth_buffer_image_info = ...;
image_t game_depth_buffer_image = s_renderer_image_create(renderer_state, &game_depth_buffer_image_info);

render_target_attachment_info_t game_target_color_buffer = {
    .attachment = color_buffer_texture,
    .attachment_type = RTAT_ColorBuffer,
};

render_target_attachment_info_t game_depth_buffer = {
    .attachment = game_depth_buffer_image,
    .attachment_type = RTAT_DepthBuffer,
};

render_target_create_info_t game_info = {
    .width = 320,
    .height = 180,
    .attachments = {game_target_color_buffer, game_depth_buffer},
    .attachment_count = 2,
    .resize_with_window = false,
};
game_target_t = s_renderer_render_target_create(renderer_state, &game_target_create_info);
// repeat this process for each target...

// NOTE(Sleepster): PERFORMED EACH FRAME 
// Above all this, is the update loop, for this hypothetical example, we don't include this data.
// There are a few things to note however, despite the lacking of the update loop for this example, 
// it is safe to assume that the update loop:
// - handles entity updates, including adding and removing entities from the entity pool as needed.
// - handles lights, the game treats lights as game objects, the game OWNS the lights.
//   and since it needs owns the lights, it needs to generate shadow geometry for each light.
//   and handle that SSBO accordingly
//
// This means that there is an SSBO that is implicitly handled by the engine and out of the user's
// control. This SSBO is:
// - The RenderInstances SSBO
//
// Why? Because this is where ALL render_instances go.
render_command_list_t *command_list = s_renderer_get_command_list(renderer_state);

r_cmd_reset_frame(command_list);
r_cmd_bind_shader(command_list, light_cluster_compute_shader);

// TODO(Sleepster): set constant buffer size.

// NOTE(Sleepster): This should store this information into a CPU side buffer. 
//
// One would think lights are owned by the renderer, however in our case we want the
// user to have the greatest degree of control and expression, therefore the user
// can just define their own lights and shadow geoemetry if they wish to use those items.
//
//
// This is purely an exampel of the kind of behavior we want, in reality you probably don't want to do this.
// The idea here is that for the user it looks like it's OpenGL "style" where you supply uniform data, render,
// repeat. But for the backend, all of these constant buffer updates get merged into a single buffer and we just have
// some easy way of indexing into the data inside the shader.
void *lights  = ...;
r_cmd_update_constant_buffer(command_list,   light_constant_buffer, lights, sizeof(light) * light_count);

// NOTE(Sleepster): 
// This is an example of how I'd like to set the constant buffer's size before we write too it on the gpu, maybe we don't need this part of the API?
// This seems weird to have... but also maybe necessary since the backend can't infer the size of the buffer after it has been written too...
void *shadows = ...;
r_cmd_set_constant_buffer_size(command_list, shadow_geometry_constant_buffer, sizeof(shadow_instance) * shadow_count);
r_cmd_update_constant_buffer(command_list,   shadow_geometry_constant_buffer, shadows, sizeof(shadow_instance) * shadow_count);

// dispath count x, dispath count y, dispatch count z
//
// NOTE: These numbers are arbitrary, I will do something ACTUALLY smart in a real implementation.
r_cmd_dispatch_compute(command_list, 20, 20, 1);

// NOTE(Sleepster): We set memory barrier manually here.
r_cmd_wait_for_compute(command_list);

// NOTE(Sleepster): Render the into the lightmap 
r_cmd_bind_render_target(command_list, lightmap);
r_cmd_bind_shader(command_list, lightmap_shader);

// NOTE(Sleepster): On the backend, all renderpasses support viewport and scissor dynamic state. 
r_cmd_set_viewport(command_list, 0, 0, lightmap_width, lightmap_height);
r_cmd_set_scissor(command_list,  0, 0, lightmap_width, lightmap_height);

// NOTE(Sleepster): 
// Draw each of the lights, we allow the user to optionally pass their own render instance incase they want to use the data present in the render instance
// in a unique way. The renderer will simply pass this along and now touch it.
r_cmd_begin_render_group(command_list);
for(u32 light_index = 0;
    light_index < light_count;
    ++light_index)
{
    // NOTE(Sleepster): Render each of the lights into the lightmap 
    render_light_t *light = lights + light_index;

    render_instance_t light_data = ...;
    r_cmd_render_instance(command_list, &light_data);
}
r_cmd_end_render_group(command_list);

// NOTE(Sleepster): 
// We render the shadows into the lightmap after drawing each of the lights, allowing the shadows to 
// act as a "cutout" for the lightmap in much the same way Celeste does their lighting...
r_cmd_bind_shader(command_list, shadowmap_shader);
r_cmd_begin_render_group(command_list);
for(u32 shadow_index = 0;
    shadow_index < shadow_count;
    ++shadow_index)
{
    render_shadow_t *render_shadow = shadows + shadow_index;
    render_instance_t shadow_data = ...;
    r_cmd_render_instance(command_list, &shadow_data);
}
r_cmd_end_render_group(command_list);

// NOTE(Sleepster): Render the game items 
r_cmd_bind_render_target(command_list, game_render_target);
r_cmd_update_constant_buffer(command_list, game_camera_constant_buffer, &game_camera, sizeof(camera));

r_cmd_set_viewport(command_list, 0, 0, game_render_target_width, game_render_target_height);
r_cmd_set_scissor(command_list,  0, 0, game_render_target_width, game_render_target_height);

// ... we'd render types of entities like so:
// NOTE: While we render these, both the opaque and the transparent passes will record the brightest pixels into the emmision buffer
//
// NOTE: THESE ARE ALL OPAQUE
for(u32 entity_type_index = 0;
    entity_type_index < entity_type_count;
    ++entity_type_index)
{
    entity_t *entity_type = entities[entity_type_index];
    if(entity->material.opacity != MATERIAL_OPACITY_TRANSPARENT)
    {
        r_cmd_bind_material(command_list, entity_type->material);
        r_cmd_begin_render_group();
        for(u32 entity_index = 0;
            entity_index < entity_count;
            ++entity_index)
        {
            entity_t *entity = entity_type + entity_index;
            r_cmd_render_bitmap(command_list, entity->bitmap);
        }
        r_cmd_end_render_group();
    }
}

// NOTE(Sleepster): 
// Render the lights into the game texture using the lightmap
r_cmd_bind_shader(command_list, emmisive_shader);
r_cmd_begin_render_group(command_list);
for(u32 light_index = 0;
    light_index < light_count;
    ++light_index)
{
    // NOTE(Sleepster): 
    // Here we're using the custom render instances to say "render from this texture". It might be useful to look at input attachments? But I'm not sure.
    render_light_t *render_light = lights + light_index;
    render_instance_t instance = {};
    instance.bitmap = lightmap;
    ...

    r_cmd_draw_instance(...);
}
r_cmd_end_render_group(command_list);

// NOTE(Sleepster): Process the bloom/emmision buffer 
//
// we again render the emmision items overtop of the game color bufer
r_cmd_bind_shader(command_list, emmision_shader);
for(u32 emmisive_quad_index = 0;
    emmisive_quad_index < emmisive_quad_count;
    ++emmisive_quad_index)
{
    // NOTE(Sleepster): Same thing we did for the lightmap and the cutout shadows... 
    ... 
}

// NOTE(Sleepster): The idea is simple, we are able to set the details needed for each of the render_groups
// without effecting the other groups. So, when we go to execute the command, the render_instance being rnedered
// would use the data associated with the render_group. The rencder_group handles information like
// what the contents of constant buffers are at the time "begin" is called, what the used shader is (materials 
// are just wrappers around a shader and some preset constants the user may want in the shader), and other such things
// like push constants.

// NOTE(Sleepster): These are transparent 
for(u32 entity_type_index = 0;
    entity_type_index < entity_type_count;
    ++entity_type_index)
{
    entity_t *entity_type = entities[entity_type_index];
    // NOTE(Sleepster): Same as above 
    // ...
    if(entity->material.opacity == MATERIAL_OPACITY_TRANSPARENT)
    {
        // ...
    }
}
// NOTE(Sleepster): The user's shader(s) will handle rendering the items into the emmision buffer

r_cmd_bind_render_target(command_list, post_fx_target);
#if 0
// NOTE(Sleepster): Maybe something like this since e KNOW we need to read the contents of this item? 
// The idea is that on the backend we might be able to do something related to making this readable from
// a shader and/or a renderpass to prevent the need to blit or take up a GPU texture slot. Not sure
// how possible this is  though... Just an idea.
r_cmd_set_input_texture(command_list, post_fx_target, game_color_buffer_image);
#endif

// in here we'd render whatever postfx then blit the game texture to the new post_fx texture...
//
// We'd really only need to blit the primary color buffer, we can ignore the emmision buffer and the depth buffer
// no good way to really denote that right now.
r_cmd_blit_render_target(command_list, post_fx_target, game_target);

// NOTE(Sleepster): 
// The rest after (for the ui and the final target blit) is more of the same.
//
// Render the ui like we did the game, then blit the post_fx_target into the final render target (which will upscale the game texture will all the effects
// processed) to the size of the user's screen. Then, we can just render the ui overtop of the upscaled texture using the ui's depth buffer in case we want game elements
// to go overtop of the ui for some gameplay reason.
// 
// Finally:
r_cmd_present(command_list);
#endif
