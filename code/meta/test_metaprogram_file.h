struct type_info_t {
    const char *name;
    const u32   size;
    const u32   metatype;
};

struct type_info_struct_t: public type_info_t {
    const u32          member_count;
    const type_info_t *member_array;
};

struct type_info_procedure_t: public type_info_t {
    const u32          argument_count;
    const type_info_t *argument_array;
};

struct type_info_string_t: public type_info_struct_t {
	const type_info_t *data;
	const type_info_t *count;
};

struct type_info_RHI_pipeline_state_t: public type_info_struct_t {
	const type_info_t *blend_enabled;
	const type_info_t *src_color_blend_mode;
	const type_info_t *dst_color_blend_mode;
	const type_info_t *src_alpha_blend_mode;
	const type_info_t *dst_alpha_blend_mode;
	const type_info_t *color_blend_op;
	const type_info_t *alpha_blend_op;
	const type_info_t *depth_testing_enabled;
	const type_info_t *depth_writing_enabled;
	const type_info_t *depth_func;
	const type_info_t *stencil_enabled;
	const type_info_t *stencil_state;
	const type_info_t *stencil_keep;
	const type_info_t *polygon_mode;
	const type_info_t *primitive_type;
};

struct type_info_asset_handle_t: public type_info_struct_t {
	const type_info_t *is_valid;
	const type_info_t *slot;
	const type_info_t *texture;
	const type_info_t *shader;
	const type_info_t *material_info;
	const type_info_t *dynamic_render_font;
};

struct type_info_material_instance_t: public type_info_struct_t {
	const type_info_t *ID;
	const type_info_t *version;
	const type_info_t *name;
	const type_info_t *renderer_effect_flags;
	const type_info_t *shader_uniform_count;
	const type_info_t *pipeline_state;
	const type_info_t *archetype;
};

struct type_info_material_archetype_t: public type_info_struct_t {
	const type_info_t *ID;
	const type_info_t *version;
	const type_info_t *name;
	const type_info_t *shader_binary_name;
	const type_info_t *shader_handle;
	const type_info_t *base_instance;
};

constexpr type_info_t *APOLLO_type_table[] = {
	&type_info_string_t,
	&type_info_RHI_pipeline_state_t,
	&type_info_material_archetype_t,
	&type_info_material_archetype_t,
};
