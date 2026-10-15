
struct type_info_t {
    const char *name;
    const u32   size;
};

struct type_info_struct_t: public type_info_t {
    const u32          member_count;
    const type_info_t *member_array;
};

struct type_info_string_t: public type_info_struct_t {
	type_info_t *data;
	type_info_t *count;
};

struct type_info_RHI_pipeline_state_t: public type_info_struct_t {
	type_info_t *blend_enabled;
	type_info_t *src_color_blend_mode;
	type_info_t *dst_color_blend_mode;
	type_info_t *src_alpha_blend_mode;
	type_info_t *dst_alpha_blend_mode;
	type_info_t *color_blend_op;
	type_info_t *alpha_blend_op;
	type_info_t *depth_testing_enabled;
	type_info_t *depth_writing_enabled;
	type_info_t *depth_func;
	type_info_t *stencil_enabled;
	type_info_t *stencil_state;
	type_info_t *stencil_keep;
	type_info_t *polygon_mode;
	type_info_t *primitive_type;
};

struct type_info_asset_handle_t: public type_info_struct_t {
	type_info_t *is_valid;
	type_info_t *slot;
	type_info_t *texture;
	type_info_t *shader;
	type_info_t *material_info;
	type_info_t *dynamic_render_font;
};

struct type_info_material_archetype: public type_info_struct_t {
	type_info_t *ID;
	type_info_t *version;
	type_info_t *name;
	type_info_t *renderer_effect_flags;
	type_info_t *shader_uniform_count;
	type_info_t *pipeline_state;
	type_info_t *archetype;
};

struct type_info_stored_material_type: public type_info_struct_t {
	type_info_t *ID;
	type_info_t *version;
	type_info_t *name;
	type_info_t *shader_binary_name;
	type_info_t *shader_handle;
	type_info_t *base_instance;
};

