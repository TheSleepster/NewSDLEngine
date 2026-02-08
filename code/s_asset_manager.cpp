/* ========================================================================
   $File: s_asset_manager.cpp $
   $Date: January 06 2026 11:43 am $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stb/stb_image.h>

#include <c_base.h>
#include <c_types.h>
#include <c_log.h>
#include <c_globals.h>
#include <c_memory_arena.h>
#include <c_zone_allocator.h>
#include <c_file_api.h>
#include <c_file_watcher.h>
#include <c_string.h>
#include <c_hash_table.h>
#include <c_dynarray.h>
#include <c_tokenizer.h>

// TODO(Sleepster): This is annoying. We need to figure out a better way of allowing people to use the RTTI in chunks.
//                  So that we don't have to include essentially every parsed header...
#include <c_program_flag_handler.h>
#include <g_game_state.h>
#include <g_entity.h>
#include <s_input_manager.h>
#include <s_nt_networking.h>
#include <r_vulkan_types.h>
//

#include <r_vulkan_core.h>
#include <r_render_group.h>

#include <asset_file_packer/jfd_asset_file.h>
#include <meta/GENERATED_program_RTTI.h>

/*===============================
  ========== TEXTURES ===========
  =============================== */

bitmap_t
s_asset_bitmap_create(asset_manager_t *asset_manager, 
                      asset_slot_t    *asset_slot, 
                      u32              width,
                      u32              height, 
                      u32              channels,
                      bitmap_format_t  format)
{
    bitmap_t result  = {};
    result.width     = width;
    result.height    = height;
    result.channels  = channels;
    result.format    = format;

    u32   pixel_count = width * height * channels;
    byte *pixel_data  = c_za_push_array(asset_manager->asset_allocator, byte, pixel_count, ZA_TAG_STATIC);

    result.pixels = {
        .data  = pixel_data,
        .count = pixel_count
    };

    return(result);
}

bitmap_t
s_asset_bitmap_init(string_t pixels, s32 width, s32 height, s32 channels, u32 format)
{
    bitmap_t result = {};
    result.width    = width;
    result.height   = height;
    result.channels = channels;
    result.format   = format;
    result.pixels   = pixels;

    return(result);
}

bitmap_t 
s_asset_bitmap_create(asset_manager_t *asset_manager, s32 width, s32 height, s32 channels, u32 format)
{
    bitmap_t result;

    u32 pixel_count = width * height * channels;
    string_t pixels = {
        .data  = c_za_alloc(asset_manager->asset_allocator, pixel_count, ZA_TAG_TEXTURE),
        .count = pixel_count 
    };

    result = s_asset_bitmap_init(pixels, width, height, channels, format);
    return(result);
}

texture2D_t 
s_asset_texture_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    texture2D_t result = {};
    string_t asset_data = slot->package_entry->asset_data;

    s32 width;
    s32 height;
    s32 channels;

    byte *pixel_data  = stbi_load_from_memory(asset_data.data, asset_data.count, &width, &height, &channels, 4);
    u32   pixel_count = (u32)(width * height * channels);
    string_t pixels = {
        .data  = pixel_data,
        .count = pixel_count 
    };
    Assert(pixel_data != null);
    Assert(pixel_count > 0);

    result.ID     = name_hash;
    result.bitmap = s_asset_bitmap_init(pixels, width, height, channels, BMF_RGBA32);
    return(result);
}

/*===============================
  =========== SHADERS ===========
  =============================== */

shader_t
s_asset_shader_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    shader_t result;
    result.ID          = name_hash;
    slot->ID           = name_hash;
    result.shader_data = r_vulkan_shader_create(asset_manager->render_context, slot->package_entry->asset_data);

    return(result);
}

/*===============================
  ========== MATERIALS ==========
  =============================== */

#if 0
internal_api void
material_file_parse_item(string_t              filename,
                         material_archetype_t *archetype, 
                         const type_info_t    *archetype_type_info, 
                         tokenizer_t          *tokenizer, 
                         token_data_t          item_name_token)
{
    string_t item_name = item_name_token.string; 
    const type_info_member_t *member_info = c_meta_get_member_info(archetype_type_info, item_name);
    if(member_info)
    {
    }
    else
    {
        log_error("Item of name: '%.*s' was found in the material file: '%s' however this item is not contained within the material_archetype_t structure...\n",
                  item_name.count, C_STR(item_name), C_STR(filename));

        log_info("Valid members for structure of type name: '%s' are as follows:\n", archetype_type_info->name);
        for(u32 member_index = 0;
            member_index < archetype_type_info->struct_info->member_count;
            ++member_index)
        {
            const type_info_member_t *member = archetype_type_info->struct_info->members + member_index;
            log_info("[%d]: %s...\n", member_index, member->name);
        }
    }
    
    string_t parent_name = parent_ident.string;

    const type_info_t *structure_data = c_meta_get_type_info_by_name(parent_name);
    Assert(structure_data);

    token_data_t next_token = c_tokenizer_peek_token(tokenizer);
    if(next_token.type == TT_Colon)
    {
        // NOTE(Sleepster): Eat the colon 
        c_tokenizer_get_next_token(tokenizer); 

        // NOTE(Sleepster): If we're in here, it's a single assignment 
        token_data_t value_token = c_tokenizer_get_next_token(tokenizer); 
        if(value_token.type == TT_Identifier)
        {
            const type_info_member_t *member = c_meta_get_member_info(structure_data, next_token.string);
            Assert(member);

            byte *data_ptr = (byte*)archetype + member->offset; 
            if(member->type == TYPE_string_t)
            {
                string_t *string_data = (string_t*)data_ptr;
                *string_data = value_token.string;
            }
            else
            {
                memcpy(data_ptr, value_token.string.data, member->size);
            }

            return;
        }
        else
        {
            log_error("Expected Identifier after token: ':'... Got: '%.*s'...\n", 
                      value_token.string.count, C_STR(value_token.string));
        }
    }
    else if(next_token.type == TT_OpeningBrace)
    {
        token_data_t token = next_token;
        while(token.type != TT_ClosingBrace)
        {
            material_file_parse_item(archetype, archetype_type_info, tokenizer, parent_ident);
            token = c_tokenizer_get_next_token(tokenizer);
        }
    }
    else
    {
        log_error("Expected to find ':' or '{' immediately after token: '%.*s'... Got: '%.*s'...\n", 
                  next_token.string.count, C_STR(next_token.string),
                  next_token.string.count, C_STR(next_token.string));
    }
}
#endif

// NOTE(Sleepster): We need to pass a pointer to the actual structure to actually store the file data
void
material_file_parse_item(string_t filename, void *parent_data, tokenizer_t *tokenizer, const type_info_t *parent_type_data, token_data_t name_token)
{
    // NOTE(Sleepster): Eating the colon... 
    c_tokenizer_get_next_token(tokenizer);
    token_data_t value_token = c_tokenizer_get_next_token(tokenizer);

    // NOTE(Sleepster): Handle enums stuff. 
    const type_info_member_t *member = c_meta_get_member_info(parent_type_data, name_token.string); 
    const type_info_struct_t *enum_data = c_meta_get_enum_type_info_from_member_string(value_token.string);
    if(member)
    {
        byte *data_ptr = (byte*)parent_data + member->offset; 
        if(member->type == TYPE_string_t)
        {
            // NOTE(Sleepster): We'll need to ignore quotes in the tokenizer 
            string_t *string_data = (string_t*)data_ptr;
            *string_data = value_token.string;
        }
        else if(enum_data)
        {
            // NOTE(Sleepster): rare instance of a do {}while(); loop being genuinely useful. We NEED this to happen at least once.
            token_data_t peek_token = c_tokenizer_peek_token(tokenizer);
            do {
                const type_info_member_t *enum_member = c_meta_get_member_info(enum_data, value_token.string);
                u32 *value = (u32*)data_ptr;

                *value |= enum_member->offset;

                peek_token  = c_tokenizer_peek_token(tokenizer);
                if(peek_token.type == TT_Seperator)
                {
                    peek_token  = c_tokenizer_get_next_token(tokenizer);
                    value_token = c_tokenizer_get_next_token(tokenizer);
                }
            }while(peek_token.type == TT_Seperator);
        }
        else
        {
            memcpy(data_ptr, value_token.string.data, member->size);
        }
    }
    else if(c_string_compare(name_token.string, STR("shader")))
    {
        log_warning("Currently, shader variables are not supported...\n");
    }
    else
    {
        log_error("Item of name: '%.*s' was found in the material file: '%s' however this item is not contained within the %s structure...\n",
                  name_token.string.count, C_STR(name_token.string), 
                  C_STR(filename),
                  parent_type_data->name);

        log_info("Valid members for structure of type name: '%s' are as follows:\n", parent_type_data->name);
        for(u32 member_index = 0;
            member_index < parent_type_data->struct_info->member_count;
            ++member_index)
        {
            const type_info_member_t *member = parent_type_data->struct_info->members + member_index;
            log_info("[%d]: %s...\n", member_index, member->name);
        }
    }
}

void 
material_file_parse_block_data(string_t filename, void *parent_data, tokenizer_t *tokenizer, const type_info_t *parent_type_data, token_data_t name_token)
{
    token_data_t token = c_tokenizer_get_next_token(tokenizer);
    while(token.type != TT_ClosingBrace)
    {
        token = c_tokenizer_get_next_token(tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                c_tokenizer_eat_lines(tokenizer, 1);
            }break;
            case TT_Identifier:
            {
                // NOTE(Sleepster): Peek to make sure this isn't a nested block. 
                token_data_t peek_token = c_tokenizer_peek_token(tokenizer);
                if(peek_token.type == TT_OpeningBrace)
                {
                    // NOTE(Sleepster): Nested block. 
                    if(c_string_compare(token.string, STR("render_pipeline_state")))
                    {
                        const type_info_member_t *render_pipeline_info = c_meta_get_member_info(parent_type_data, STR("pipeline_state"));
                        render_pipeline_state *state_data = (render_pipeline_state_t*)((byte*)parent_data + render_pipeline_info->offset);

                        const type_info_t *type_data = c_meta_get_type_info_by_name(STR("render_pipeline_state_t"));
                        material_file_parse_block_data(filename, state_data, tokenizer, type_data, token);

                    }
                    else
                    {
                        log_error("This nested block named '%.*s' inside of our '%s' block is not a valid subblock, the only valid one right now is 'render_pipeline_state'...\n",
                                  token.string.count, C_STR(token.string),
                                  parent_type_data->name);
                        return;
                    }
                }
                else
                {
                    material_file_parse_item(filename, parent_data, tokenizer, parent_type_data, token);
                }
            }break;
        }
    }
}

material_archetype_t
s_asset_material_archetype_create(asset_manager_t *asset_manager, asset_slot_t *slot, u64 name_hash)
{
    material_archetype_t result = {};
    slot->ID  = name_hash;
    result.ID = name_hash;
    slot->package_entry->asset_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                                              slot->package_entry->asset_data.count,
                                                              slot->package_entry->data_offset, 
                                                              null, 
                                                              asset_manager->asset_allocator, 
                                                              ZA_TAG_STATIC);
    string_t material_data = slot->package_entry->asset_data;
    tokenizer_t tokenizer = {};
    tokenizer.data = material_data;

    while(tokenizer.data.count > 0)
    {
        token_data_t token = c_tokenizer_get_next_token(&tokenizer);
        switch(token.type)
        {
            case TT_HashTag:
            {
                c_tokenizer_eat_lines(&tokenizer, 1);
            }break;
            case TT_Identifier:
            {
                token_data_t next_peeked = c_tokenizer_peek_token(&tokenizer);
                if(next_peeked.type == TT_Colon)
                {
                    // TODO(Sleepster): For now nothing, this requires us to know what kind of file we are in so that we can properly parse this item.
                    // If we are in a .m_arch file, then it should be material_archetype_t, if we are in a .m_inst file, it should be material_instance_t
                    // hence why we're doing nothing with it right now.
                    
                    //material_file_parse_item(&tokenizer, token);
                }
                else if(next_peeked.type == TT_OpeningBrace)
                {
                    // parse_item_structure();
                    const type_info_t *struct_info = null;
                    if(c_string_compare(token.string, STR("material_archetype"))) 
                    {
                        struct_info = c_meta_get_type_info_by_name(STR("material_archetype_t"));
                    }
                    else if(c_string_compare(token.string, STR("base_instance")))
                    {
                        struct_info = c_meta_get_type_info_by_name(STR("material_instance_t"));
                    }
                    else 
                    {
                        log_error("The intializer item is neither a material_archetype or material_instance block. These are the ONLY TWO valid items that can be in this scope... Found: '%.*s'\n",
                                  token.string.count, C_STR(token.string));
                        break;
                    }
 
                    material_file_parse_block_data(slot->owner_asset_file.file_name, &result, &tokenizer, struct_info, token);
                }
                else
                {
                    log_error("Expected to find ':' or '{' immediately after token: '%.*s'... Got: '%.*s'...\n", 
                              token.string.count, C_STR(token.string),
                              next_peeked.string.count, C_STR(next_peeked.string));
                }
            }break;
        }
    }

    return(result);
}

/*===============================
  ========= ASSET DATA ==========
  =============================== */

void
s_asset_manager_load_asset_data(asset_manager_t *asset_manager, asset_handle_t *handle, u64 name_hash)
{
    asset_slot_t *slot = handle->slot;
    Assert(slot->slot_state == ASLS_Unloaded || slot->slot_state == ASLS_ShouldReload);
    slot->package_entry->asset_data = c_file_read_from_offset(&slot->owner_asset_file, 
                                                              slot->package_entry->asset_data.count,
                                                              slot->package_entry->data_offset, 
                                                              null, 
                                                              asset_manager->asset_allocator, 
                                                              ZA_TAG_STATIC);
    Assert(slot->package_entry->asset_data.data != null);
    switch(slot->type)
    {
        case AT_Bitmap:
        {
            slot->texture = s_asset_texture_create(asset_manager, slot, name_hash);
            log_info("Loading texture data for bitmap: '%s'...\n", C_STR(handle->slot->name));
        }break;
        case AT_Shader:
        {
            slot->shader = s_asset_shader_create(asset_manager, slot, name_hash);
            log_info("Loading shader data for: '%s'...\n", C_STR(handle->slot->name));
        }break;
        case AT_Material:
        {
            slot->material = s_asset_material_archetype_create(asset_manager, slot, name_hash);
            log_info("Loading material data for: '%s'...\n", C_STR(handle->slot->name));
        }break;
        case AT_Font:
        {
            log_warning("Not loading font... not currently supported...\n");
        }break;
        case AT_Sound:
        {
            log_warning("Not loading sound... not currently supported...\n");
        }break;
    }
    slot->slot_state = ASLS_Loaded;
    AtomicIncrement32(&slot->package_generation);
}

internal_api
C_HASH_TABLE_ALLOCATE_IMPL(asset_manager_hash_arena_allocate)
{
    void *result = null;
    result = c_arena_push_size((memory_arena_t*)allocator, allocation_size);

    return(result);
}

// ===============================
// ========== ASSET MANAGER ======
// ===============================

// TODO(Sleepster): Generate default assets 
void
s_asset_manager_init(asset_manager_t *asset_manager)
{
    Assert(asset_manager->is_initialized == false);
    stbi_set_flip_vertically_on_load(0);

    asset_manager->manager_arena   = c_arena_create(MB(100));
    asset_manager->asset_allocator = c_za_create(GB(1));
    for(u32 catalog_index = 1;
        catalog_index < AT_Count;
        ++catalog_index)
    {
        asset_catalog *catalog = asset_manager->asset_catalogs + catalog_index;
        catalog->asset_manager = asset_manager;
        c_hash_table_init(&catalog->asset_lookup, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_manager->manager_arena, 
                           asset_manager_hash_arena_allocate,
                           null);
        catalog->catalog_type = (asset_type_t)(catalog_index);

        Assert(catalog->catalog_type < AT_Count);
        Assert(catalog->catalog_type > AT_Invalid);
    }
    c_hash_table_init(&asset_manager->asset_name_to_file, 
                       ASSET_CATALOG_MAX_LOOKUPS, 
                      &asset_manager->manager_arena, 
                       asset_manager_hash_arena_allocate,
                       null);
    // NOTE(Sleepster): Initializing all entries to -1 
    memset(asset_manager->asset_name_to_file.data, -1, sizeof(s32) * ASSET_CATALOG_MAX_LOOKUPS);
    asset_manager->is_initialized = true;
}

bool8
s_asset_manager_load_asset_file(asset_manager_t *asset_manager, string_t filepath)
{
    Assert(asset_manager->is_initialized);
    Assert(asset_manager->loaded_file_count + 1 <= ASSET_MANAGER_MAX_ASSET_FILES);
    
    bool8 result = true; 
    asset_manager_asset_file_data_t *asset_file = null;
    for(u32 file_index = 0;
        file_index < ASSET_MANAGER_MAX_ASSET_FILES;
        ++file_index)
    {
        asset_manager_asset_file_data_t *found = asset_manager->asset_files + file_index;
        if(found->is_initialized == false)
        {
            asset_file = found;
            break;
        }
    } 
    Assert(asset_file);
    Assert(!asset_file->is_initialized);
    
    asset_file->file_info = c_file_open(filepath, false);
    if(asset_file->file_info.handle != INVALID_FILE_HANDLE)
    {
        file_t *file_handle = &asset_file->file_info;

        asset_file->init_arena      = c_arena_create(MB(500));
        asset_file->is_initialized  = true;
        asset_file->ID              = asset_manager->loaded_file_count;

        jfd_file_header_t *header = (jfd_file_header_t*)(c_file_read(file_handle, sizeof(jfd_file_header_t), &asset_file->init_arena).data);
        Assert(header->magic_value == ASSET_FILE_HEADER_MAGIC);
        
        asset_file->package_entries = c_arena_push_array(&asset_file->init_arena, jfd_package_entry_t, header->entry_count);
        c_hash_table_init(&asset_file->entry_hash, 
                           ASSET_CATALOG_MAX_LOOKUPS, 
                          &asset_file->init_arena, 
                           asset_manager_hash_arena_allocate,
                           null);

        u64 current_file_offset = file_handle->current_read_offset;
        for(u32 entry_index = 0;
            entry_index < header->entry_count;
            ++entry_index)
        {
            u64 data_offset = current_file_offset + sizeof(jfd_package_chunk_header_t);
            jfd_package_entry_t *entry = asset_file->package_entries + entry_index;
            entry->entry_header = (jfd_package_chunk_header_t*)(c_file_read_from_offset(file_handle, 
                                                                                        sizeof(jfd_package_chunk_header_t), 
                                                                                        current_file_offset, 
                                                                                       &asset_file->init_arena).data);
            Assert(entry->entry_header->magic_value == ASSET_FILE_CHUNK_MAGIC);
            entry->filename = c_file_read_from_offset(file_handle, 
                                                      entry->entry_header->filename_size, 
                                                      data_offset, 
                                                     &asset_file->init_arena);
            entry->asset_data.count = entry->entry_header->entry_data_size;
            entry->data_offset   = data_offset + entry->filename.count;
            current_file_offset += entry->entry_header->total_entry_size;
             
            c_hash_table_insert_pair(&asset_file->entry_hash, entry->filename, (s32)entry_index);
            c_hash_table_insert_pair(&asset_manager->asset_name_to_file, entry->filename, (s32)asset_file->ID);
            u64 hash_value = c_hash_table_value_from_key(entry->filename.data, entry->filename.count, asset_manager->asset_name_to_file.header.max_entries);

            log_debug("Inserting asset with name: '%s' with a name length of: '%d' into the name_to_file hash with file_index: '%d' hash_value: '%llu'...\n", C_STR(entry->filename), entry->filename.count, asset_file->ID, hash_value);

            asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
            asset_slot_t    *slot    = c_hash_table_get_value_ptr(&catalog->asset_lookup, entry->filename);
            Assert(entry->entry_header->asset_type == catalog->catalog_type);
            Assert(slot);

            ZeroStruct(*slot);
            slot->slot_state       = ASLS_Unloaded;
            slot->type             = (asset_type_t)entry->entry_header->asset_type;
            slot->name             = entry->filename;
            slot->package_entry    = entry;
            slot->ref_counter      = 0;
            slot->owner_asset_file = asset_file->file_info;
        }
        asset_manager->loaded_file_count += 1;
    }
    else
    {
        log_error("Failure to open file: type'%s'... invalid handle...\n", C_STR(filepath));
        result = false;
    }

    return(result);
}

internal_api asset_slot_t *
s_asset_manager_get_asset_slot(asset_catalog_t *catalog, string_t name)
{
    asset_slot *result = null;
    result = c_hash_table_get_value_ptr(&catalog->asset_lookup, name);
    if(result == null)
    {
        log_error("Failure to fetch asset '%s' from this catalog...\n", C_STR(name));
    }
    else
    {
        AtomicIncrement32(&result->ref_counter);
    }

    return(result);
}

asset_handle_t
s_asset_manager_acquire_asset_handle(asset_manager_t *asset_manager, string_t name)
{
    asset_handle_t result;

    u64 hash_value = c_hash_table_value_from_key(name.data, name.count, asset_manager->asset_name_to_file.header.max_entries);
    log_info("hash index for: '%s' is '%llu'...\n", C_STR(name), hash_value);
    s32 file_index = c_hash_table_get_value(&asset_manager->asset_name_to_file, name);
    if(file_index != -1)
    {
        asset_manager_asset_file_data_t *asset_file = asset_manager->asset_files + file_index;
        s32 asset_entry_index = c_hash_table_get_value(&asset_file->entry_hash, name);
        // NOTE(Sleepster): This just SHOULD NOT be possible... 
        //                  An assert here would imply that we found the file inside of a package, but cannot locate it.
        //                  Which in any case is a bug and should be fixed immediately.
        Assert(asset_entry_index != -1);

        jfd_package_entry_t *entry = asset_file->package_entries + asset_entry_index;
        Assert(entry->entry_header->asset_type != AT_Invalid && entry->entry_header->asset_type != AT_Count);

        asset_catalog_t *catalog = asset_manager->asset_catalogs + entry->entry_header->asset_type;
        Assert(catalog);

        result.type = (asset_type_t)entry->entry_header->asset_type;
        result.slot = s_asset_manager_get_asset_slot(catalog, name);
        result.owner_asset_file_index         = file_index;
        result.is_valid                       = true;
        if(result.slot->slot_state == ASLS_Unloaded)
        {
            s_asset_manager_load_asset_data(asset_manager, &result, hash_value);
        }

        Assert(result.slot->slot_state != ASLS_Invalid);
    }
    else
    {
        log_error("Asset by name of: '%s' cannot be found in the asset file database...\n",
                  C_STR(name));
    }

    return(result);
}

// ===============================
// ======= TEXTURE ATLASES =======
// ===============================

texture_atlas_t*
s_texture_atlas_create(asset_manager_t *asset_manager, 
                       u32              size, 
                       u32              channel_count, 
                       u32              format, 
                       u32              initial_subtexture_count)
{
    texture_atlas_registry_t *registry = &asset_manager->atlas_registry;
    texture_atlas_t *atlas = null;
    for(u32 atlas_index = 0;
        atlas_index < ArrayCount(registry->atlases);
        ++atlas_index)
    {
        texture_atlas_t *found = registry->atlases + atlas_index;
        if(found->is_valid == false)
        {
            atlas = found;
            break;
        }
    }
    // TODO(Sleepster): Might wanna do something about this assert... I just don't care right now.
    Assert(atlas);
    Assert(atlas->is_valid == false)

    registry->current_atlas_count += 1;

    atlas->texture.bitmap = s_asset_bitmap_create(asset_manager, size, size, channel_count, format);
    atlas->bitmap_data    = &atlas->texture.bitmap;
    atlas->atlas_size     = size;

    atlas->textures_to_merge = c_dynarray_create(asset_handle_t*);
    c_dynarray_reserve(atlas->textures_to_merge, initial_subtexture_count);

    atlas->packed_subtextures = c_dynarray_create(subtexture_data_t);
    atlas->packed_subtextures = c_dynarray_reserve(atlas->packed_subtextures, initial_subtexture_count);

    atlas->ID       = registry->current_atlas_count - 1;
    atlas->is_valid = true;

    return(atlas);
}

void
s_texture_atlas_add_texture(texture_atlas_t *atlas, asset_handle_t *texture_handle)
{
    Assert(texture_handle);
    Assert(texture_handle->is_valid);
    Assert(texture_handle->type == AT_Bitmap);

    c_dynarray_push(atlas->textures_to_merge, texture_handle);
    atlas->merge_counter += 1;
}

void
s_texture_atlas_pack_added_textures(vulkan_render_context_t *render_context, texture_atlas_t *atlas)
{
    Assert(atlas->is_valid);

    if(atlas->merge_counter > 0)
    {
        u32 atlas_width       = atlas->bitmap_data->width;
        u32 atlas_height      = atlas->bitmap_data->height;
        u32 atlas_channels    = atlas->bitmap_data->channels;
        string_t atlas_pixels = atlas->bitmap_data->pixels;

        c_dynarray_for(atlas->textures_to_merge, texture_index)
        {
            asset_handle_t *asset        =  atlas->textures_to_merge[texture_index];
            bitmap_t       *asset_bitmap = &asset->slot->texture.bitmap;

            u32 padding = 1;
            u32 bitmap_width       = asset_bitmap->width;
            u32 bitmap_height      = asset_bitmap->height;
            u32 bitmap_channels    = asset_bitmap->channels;
            string_t bitmap_pixels = asset_bitmap->pixels;

            Assert(asset_bitmap->channels == atlas->bitmap_data->channels);

            // NOTE(Sleepster): Wrap to next y if needed. 
            if((atlas->atlas_cursor_x + bitmap_width) >= atlas_height)
            {
                atlas->atlas_cursor_x  = 0;
                atlas->atlas_cursor_y += atlas->tallest_y;
            }
            if(bitmap_height > atlas->tallest_y) atlas->tallest_y = bitmap_height;

            u32 atlas_cursor_x = atlas->atlas_cursor_x + padding;
            u32 atlas_cursor_y = atlas->atlas_cursor_y + padding;

            // NOTE(Sleepster): Copy by row. 
            for(u32 row_index = 0;
                row_index < bitmap_height;
                ++row_index)
            {
                u32 atlas_bitmap_offset  = ((atlas_cursor_y + row_index) * atlas_width + atlas_cursor_x) * atlas_channels;
                byte *atlas_pixel_offset = atlas_pixels.data + atlas_bitmap_offset;

                u32 bitmap_offset = (row_index * bitmap_width) * bitmap_channels;
                byte *bitmap_data_offset = bitmap_pixels.data + bitmap_offset;

                memcpy(atlas_pixel_offset, bitmap_data_offset, bitmap_width * bitmap_channels);
            }

            vec2_t uv_min = vec2(atlas_cursor_x, atlas_cursor_y);
            vec2_t uv_max = vec2(atlas_cursor_x + bitmap_width, atlas_cursor_y + bitmap_height);

            // NOTE(Sleepster): Create the subtexture, let the owner of the sprite know this is that subtexture. 
            subtexture_data_t *subtexture = atlas->packed_subtextures + atlas->packed_subtexture_count;
            asset->subtexture_data = subtexture;

            subtexture->uv_min                 = uv_min;
            subtexture->uv_max                 = uv_max;
            subtexture->offset                 = uv_min;
            subtexture->size                   = vec2(bitmap_width, bitmap_height);
            subtexture->atlas_subtexture_index = atlas->packed_subtexture_count++;
            subtexture->atlas                  = atlas;

            atlas->atlas_cursor_x = atlas_cursor_x + bitmap_width;
        }
        c_dynarray_clear(atlas->textures_to_merge);
        r_vulkan_make_gpu_texture(render_context, &atlas->texture);
    }
    else
    {
        log_info("There are no textures to pack currently...\n");
    }
}
