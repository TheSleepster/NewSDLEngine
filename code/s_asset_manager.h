#if !defined(S_ASSET_MANAGER_H)
/* ========================================================================
   $File: s_asset_manager.h $
   $Date: January 06 2026 04:57 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ASSET_MANAGER_H

#include <ft2build.h>
#include <freetype/ftadvanc.h>
#include FT_FREETYPE_H

#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_hash_table.h>
#include <c_threadpool.h>
#include <c_dynarray.h>
#include <c_duration_counter.h>

#include <s_RHI_image.h>
#include <vk_backend_image.h>
#include <vk_backend_shader.h>

#define ASSET_CATALOG_MAX_LOOKUPS         (4099)
#define ASSET_MANAGER_MAX_TEXTURE_ATLASES (128)
#define ASSET_MANAGER_MAX_ASSET_FILES     (32)

typedef struct asset_slot         asset_slot_t;
typedef struct subtexture_data    subtexture_data_t;
typedef struct texture_atlas      texture_atlas_t;
typedef struct texture2D          texture2D_t;
typedef struct shader             shader_t;
typedef struct material_archetype material_archetype_t;
typedef struct material_instance  material_instance_t;
typedef struct material_data      material_data_t;
#if 0
typedef struct animation_source2D animation_source2D_t;
typedef struct animation2D        animation2D_t;
#endif

typedef struct jfd_package_entry  jfd_package_entry_t;
typedef struct jfd_file_header    jfd_file_header_t;

struct asset_manager_t;
struct dynamic_render_font_t;
struct asset_catalog_t;

typedef enum asset_type
{
    AT_Invalid,
    AT_Bitmap,
    AT_Shader,
    AT_Material,
    AT_Font,
    AT_Sound,
    AT_Count
}asset_type_t;

typedef enum asset_slot_state
{
    ASLS_Invalid,
    ASLS_Unloaded,
    ASLS_LoadQueued,
    ASLS_Loaded,
    ASLS_ShouldUnload,
    ASLS_ShouldReload,
    ASLS_Count
}asset_slot_load_status_t;

typedef enum bitmap_format
{
    BMF_Invalid,
    BMF_R8,
    BMF_B8,
    BMF_G8,
    BMF_RGBA32_SRGB,
    BMF_RGBA32_UNORM,
    BMF_BGRA32_UNORM,
    BMF_RGB24_UNORM,
    BMF_RGB24_SRGB,
    BMF_D24_SFLOAT_S8,
    BMF_D32_SFLOAT_S8_UINT,
    BMF_D32_SFLOAT,
    BMF_Count,
}bitmap_format_t;

/* NOTE(Sleepster): 
 * The Asset handle is very simple, it is simply a means to only do the expensive hash lookups once, 
 * and then have a means to update the asset's state in a way that is much less expensive than the lookups
 * themselves. All actions relating to the asset performed through the handle. Actions like these are such as:
 * - Allocating from the asset file
 * - Freeing the asset's data
 */
typedef struct asset_handle
{
    bool32       *is_valid;
    asset_slot_t *slot;
    union {
        texture2D_t           *texture;
        shader_t              *shader;
        material_data_t       *material_info;
        dynamic_render_font_t *dynamic_render_font;
#if 0
        animation_source2D_t  *animation_source2D;
        animation2D_t         *animation2D;
#endif
    };
}asset_handle_t;

/*===========================================
  ================= TEXTURES ================
  ===========================================*/

// TODO(Sleepster): For now we just assume all bitmaps want NEAREST filtering... might not actually be the case.
typedef struct bitmap
{
    u32      width;
    u32      height;
    u32      channels;
    u32      format;

    // NOTE(Sleepster): Treated as a byte array
    string_t pixels;
}bitmap_t;

typedef struct texture2D
{
    u64         ID;
    bitmap_t    bitmap;
    RHI_image_t gpu_data;
}texture2D_t;

typedef struct subtexture_data
{
    vec2_t           uv_min;
    vec2_t           uv_max;

    vec2_t           offset;
    vec2_t           size;

    u32              atlased_generation;
    u32              atlas_subtexture_index;
    texture_atlas_t *atlas;
    asset_handle_t   packed_asset;
}subtexture_data_t;

/* TODO(Sleepster): 
 * We need to allow the texture atlas (which is technically an atlas manager)
 * to free textures and put their spaces into a "free list" of some sorts so that
 * when an asset is flagged for release, it does not stay in the atlas.
 */
typedef struct texture_atlas
{
    texture2D_t                   texture;
    bitmap_t                     *bitmap_data;

    u32                           ID;
    u32                           merge_counter;
    dynarray_t<asset_handle_t*>   textures_to_merge;

    // TODO(Sleepster): 
    // Technically we need not "add" to this array, just pull from it. What do we do once we 
    // pull more than what's in it? Guess it's not a problem for now.
    //
    // If we run out of indices in here, just expand the size of the array?
    dynarray_t<subtexture_data_t> packed_subtextures;
    u32                           packed_subtexture_count;
    bool32                        is_valid;

    u32                           atlas_cursor_x;
    u32                           atlas_cursor_y;
    u32                           tallest_y;
    u32                           atlas_size;
}texture_atlas_t;

/*===========================================
  ================== SHADERS ================
  =========================================== */

typedef struct shader 
{
    u64              ID;
    backend_shader_t shader_data;
}RHI_shader_t;

#if 0
// TODO(Sleepster): 
// Perhaps this is the way we would like to handle materials going forward.
// This setup is very simple. We have a generator analyze each of our material files
// and output structures EXACTLY like so.
//
// The idea is simple, for every material we don't want to have to perform hashing for
// finding and updating the material's data. This is because it's slow, and we can just know
// the offset into the buffer manually. This approach allows us to do things like so:
//
// NOTE(Sleepster): We can pass the size here, but again the generator should let us know that without having to pass this.
  asset_handle_t material_instance = r_create_material_instance(&render_state->shiny_material);
  r_set_constant_buffer_data(material_instance.set0.matrices, &shader->camera_matrices, sizeof(shader->camera_matrices));
// 
// Where since we already know the offsets, we can just pass the byte* (which to be clear, is predetermined from the size of the
// uniform that the generator would ALSO give us) which will be the offset of the uniform in question inside the buffer.
// Giving us an easy way to cheaply update the data for the uniform without much effort. 
//
// -Justin Febuary 11 2026

struct shiny_material_t: public material_instance_t
{
    struct set0 {
        constant_buffer_t constant_buffer;
        byte             *camera_matrices;
    };

    struct set1 {
        struct uniform_buffer {
            constant_buffer_t constant_buffer;
            byte             *material_uniform_buffer;
            byte             *sampler_data;
        };

        struct SSBO {
            constant_buffer_t  storage_buffer;
            byte              *render_instances;
        };
    };

    struct set2 {
        constant_buffer_t constant_buffer;
        byte             *model_matrix;
        byte             *texture_data;
    };
};
#endif

/* MATERIAL CONFIG:
 * - Shader name
 * - ID
 * - Name of the material
 * - Default pipeline state (blend mode, blend enabled, depth mode, depth enabled, etc.)
 */


/* NOTE(Sleepster): 
 * The idea is that you write out the base material you want to use using a .mat config file.
 * When you need the material, you acquire an asset handle too it. If you need to change
 * something about the base material such as "vibrance = 1.0f" instead of "vibrance = 0.8"
 * as is defined in the material config, you would simply be able to make a copy to that material,
 * then customize these settings. Keeping the base material untouched.
 */

// MATERIAL INSTANCE
typedef struct material_instance
{
    u64                           ID;
    u32                           version;
    string_t                      name;

    // TODO(Sleepster): Does this even serve a purpose??? 
    u32                           renderer_effect_flags;
    u32                           shader_uniform_count;

    RHI_pipeline_state_t          pipeline_state;
    material_archetype_t         *archetype;
}material_instance_t;

// MATERIAL ARCHETYPE
typedef struct material_archetype
{
    // TODO(Sleepster): 
    // Is dirty flag. If the contents of the material archetype change, we should
    // tell the asset system we require this to be reloaded. All instances based off of this archetype
    // will be fine since they store a pointer to the archetype.
    u64                 ID;
    u32                 version;

    string_t            name;
    string_t            shader_binary_name;
    asset_handle_t      shader_handle;

    VkDescriptorSet     descriptors[MAX_DESCRIPTOR_SET_BINDINGS];
    material_instance_t base_instance;
}material_archetype_t;

typedef enum stored_material_type
{
    SMT_Invalid,
    SMT_Instance,
    SMT_Archetype,
}stored_material_type_t;

typedef struct material_data
{
    stored_material_type_t material_type;
    union {
        material_archetype_t archetype;
        material_instance_t  instance;
    };
}material_data_t;


/*===========================================
  ================ FONT DATA  ===============
  ===========================================*/

constexpr u32 MAX_CACHED_TEMPORARY_FONT_GLYPHS = 1000;

const global_variable u8 UTF8_trailing_bytes[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

const global_variable u8 UTF8_initial_bytemask[] = {0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
const global_variable u8 UTF8_first_byte_mark[]  = {0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc};

const global_variable u32 UTF8_offsets[] = {
    0x00000000, 0x00003080, 0x000e2080, 
    0x03c82080, 0xfa082080, 0x82082080
};

#define UTF16_MAX_CHARACTER         0x0010FFFF
#define UTF32_MAX_CHARACTER         0x7FFFFFFF
#define UTF32_REPLACEMENT_CHARACTER 0x0000FFFD

inline s32
FT_ROUND(s32 X)
{
    if (X >= 0) return (X + 0x1f) >> 6;
    return -(((-X) + 0x1f) >> 6);
}

inline u8*
unicode_next_character(u8 *character)
{
    u8 character_bytes = 1 + UTF8_trailing_bytes[*character];
    return(character + character_bytes);
}

struct dynamic_render_font_t;
struct dynamic_render_font_varient_t;

struct glyph_metric_t 
{
    bool8  is_valid;
    bool8  is_fetched;

    s32    advance;
    s32    ascent; 
    s32    width;
    s32    height;

    s32    offset_x;
    s32    offset_y;

    vec2_t atlas_offset;
    vec2_t atlas_size;

    texture_atlas_t *owner_atlas;
};

struct temporary_glyph_t
{
    u32              utf32_codepoint;
    glyph_metric_t  *metrics;

    u32              glyph_width;
    u32              glyph_height;

    u32              cursor_x;
    u32              cursor_y;
};

struct dynamic_render_font_page_t 
{
    bool32                         is_dirty;
    bool32                         is_full;

    dynamic_render_font_t         *parent_font;
    dynamic_render_font_varient_t *varient;
    texture_atlas_t               *font_atlas;
    hash_table_t<glyph_metric_t*>  glyphs;
    u32                            loaded_glyph_count;

    // NOTE(Sleepster): 
    // These are a "transient glyph". They're meant to store intermediate information related
    // to the glyph so that it can be loaded and appended into the atlas at the end of the frame in parallel...
    temporary_glyph_t              temporary_glyphs[MAX_CACHED_TEMPORARY_FONT_GLYPHS];
    u32                            temporary_glyph_count;

    u32                            atlas_cursor_x;
    u32                            atlas_cursor_y;
    u32                            tallest_y;

    // NOTE(Sleepster): 
    // This is done so that if the font
    // atlas ever gets full, we have more area to store new glyphs
    dynamic_render_font_page_t *next_page;
};

struct dynamic_render_font_varient_t
{
    u32                         font_size;

    float64                     size_to_pixels;
    s64                         line_spacing;
    s64                         max_ascender;
    s64                         max_descender;
    s32                         y_center_offset;
    s32                         typical_ascender;
    s32                         typical_descender;
    s32                         em_width;
    s32                         default_unknown_character;
    s32                         default_utf32_unknown_character;

    dynamic_render_font_t      *parent_font;
    dynamic_render_font_page_t *first_page;
};

struct dynamic_render_font_t 
{
    memory_arena_t                             font_arena;
    FT_Face                                    font_face;
    dynarray_t<dynamic_render_font_varient_t*> varients;
};

/*===========================================
  ============= ASSET FILE DATA =============
  ===========================================*/

typedef struct asset_slot 
{
    u64                      ID;
    asset_slot_load_status_t slot_state;
    asset_type_t             type;
    bool32                   is_valid_for_handles;
    
    string_t                 name;
    file_t                   owner_asset_file;
    jfd_package_entry_t     *package_entry;

    subtexture_data_t       *subtexture_data;
    asset_manager_t         *asset_manager;
    asset_catalog_t         *catalog;

    // NOTE(Sleepster): Should only be modified using atomic_* functions 
    volatile u32             package_generation;
    // NOTE(Sleepster): This is here for access when reloading/unloading the asset. 
    volatile u32             loaded_asset_index;
    s32                      owner_asset_file_index;
    union {
        texture2D_t           texture;
        dynamic_render_font_t dynamic_render_font;
        shader_t              shader;
        material_data_t       material;
    };
}asset_slot_t;

// NOTE(Sleepster): Everything file related lives and dies with this arena. 
typedef struct asset_file_data
{
    bool8                     is_initialized;
    u32                       ID;
    volatile u32              current_package_generation;
    memory_arena_t            init_arena;

    asset_slot_load_status_t  load_status;
    file_t                    file_info;
 
    string_t                  raw_file_data;

    jfd_file_header_t        *header;
    jfd_package_entry_t      *package_entries;
    u32                       package_entry_count;
    hash_table_t<s32>         entry_hash;

    dynarray_t<asset_slot_t*> loaded_assets;
    jfd_file_header_t         *header_data;
}asset_file_data_t;

/*===========================================
  =========== ASSET MANAGER DATA ============
  ===========================================*/
constexpr u32 MAX_QUEUED_ASSETS = 256;

// TODO(Sleepster): 
// The asset catalogs should be unique for each asset type. The "Atlas Registry" is a prime example
// of why this is a good idea. Why do we need an "atlas registry" when we could just treat a texture atlas exactly
// as a normal texture stored inside of the texture catalog... it keeps everything centralized and meaningful.
struct asset_catalog_t
{
    u32                        ID;
    asset_type_t               catalog_type;
    asset_manager_t           *asset_manager;

    asset_handle_t             default_asset;

    // TODO(Sleepster): Should this be a * to asset_slots?
    hash_table_t<asset_slot_t> asset_lookup;
    dynarray_t<asset_slot_t*>  loaded_assets;
};

struct texture_manager_t 
{
    texture_atlas_t atlases[ASSET_MANAGER_MAX_TEXTURE_ATLASES];
    u32             current_atlas_count;
};

struct font_manager_t
{
    dynamic_render_font_page_t *pages_to_update[10];
    u32                         pages_queued;
};

// TODO(Sleepster): thread safety
struct texture_atlas_registry_t
{
    texture_atlas_t atlases[ASSET_MANAGER_MAX_TEXTURE_ATLASES];
    u32             current_atlas_count;
};

// NOTE(Sleepster): 
//
// Here's the plan for allocating assets, predetermined lifetimes. We'll have a:
// - Level lifetime asset arena
// - Permanent lifetime asset arena
//
// Simple. Doesn't deal with ref-counters or anything heap related.
struct asset_manager_t
{
    bool8                     is_initialized;
    memory_arena_t            manager_arena;

    // NOTE(Sleepster): For font generation at runtime.
    FT_Library                freetype_handle;

    // NOTE(Sleepster): Hash table for hashing asset filenames with thier associated asset file
    // Ex: "player.png" -> "/run_tree/res/main_asset_file.wad"
    // or even beter "player.png" -> index 0 of the asset_file array
    asset_file_data_t         asset_files[ASSET_MANAGER_MAX_ASSET_FILES];
    hash_table_t<s32>         asset_name_to_file;
    u32                       loaded_file_count;

    asset_slot_t             *asset_load_queue[MAX_QUEUED_ASSETS];
    u32                       load_queue_size;
        
    asset_slot_t             *asset_unload_queue[MAX_QUEUED_ASSETS];
    u32                       unload_queue_size;

    texture_atlas_registry_t  atlas_registry;

    // TODO(Sleepster): Replace this zone allocator thing. Not great for more than one thread... 
    zone_allocator_t         *asset_allocator;
    asset_catalog_t           asset_catalogs[AT_Count];
    asset_catalog_t          *texture_catalog;
    asset_catalog_t          *shader_catalog;
    asset_catalog_t          *material_catalog;
    asset_catalog_t          *font_catalog;
    asset_catalog_t          *sound_catalog;

    font_manager_t            font_manager;
    RHI_context_t            *RHI_context;
};

void s_asset_manager_init(asset_manager_t *asset_manager);
void s_asset_manager_update(asset_manager_t *asset_manager);
void s_asset_manager_queue_asset_load(asset_manager_t *asset_manager, asset_slot_t *slot);

// TODO(Sleepster): Unicode.cpp 
void s_UTF32_convert_to_UTF8(string_t *buffer, u32 character);
u32  s_UTF8_convert_UTF32(u8 *character);

bool8          s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath);
void           s_asset_manager_signal_asset_file_reload(asset_manager_t *asset_manager, string_t filename);
asset_handle_t s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name);

texture_atlas_t *s_texture_atlas_create(asset_manager_t *asset_manager, u32 size, u32 channel_count, u32 format, u32 initial_subtexture_count);
void             s_texture_atlas_add_texture(texture_atlas_t *atlas, asset_handle_t *texture_handle);
void             s_texture_atlas_pack_added_textures(asset_manager_t *asset_manager, texture_atlas_t *atlas);

true_inline void           s_asset_manager_set_handle_asset_data_pointer(asset_handle_t *handle, asset_slot_t *slot);
internal_api asset_slot_t *s_asset_manager_get_asset_slot(asset_catalog_t *catalog, string_t name);

shader_t*             s_asset_get_shader_from_handle(asset_handle_t *handle);
texture2D_t*          s_asset_get_texture_from_handle(asset_handle_t *handle);
material_data_t*      s_asset_get_material_data_from_handle(asset_handle_t *handle);
material_archetype_t* s_asset_get_material_archetype_from_handle(asset_handle_t *handle);
material_instance_t*  s_asset_get_material_instance_from_handle(asset_handle_t *handle);

vec2_t                         s_asset_font_get_string_size(asset_manager_t *asset_manager, string_t string, asset_handle_t *font_handle, u32 pixel_size, float32 *max_descender_out);
dynamic_render_font_varient_t *s_asset_font_acquire_font_at_size(asset_manager_t *asset_manager, asset_handle_t *font_handle, u32 font_size);

glyph_metric_t*
s_asset_font_fetch_glyph(asset_manager_t               *asset_manager,
                         dynamic_render_font_varient_t *varient,
                         byte                          *codepoint);

#endif // S_ASSET_MANAGER_H

