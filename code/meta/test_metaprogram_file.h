struct type_info_t {
    const char *name;
    const u32   size;
    const u32   metatype;
};

struct type_info_member_t: public type_info_t {
    const type_info_t *type_info;
    const u32          offset;
    const u32          value_type;
    union {
        int                int32_value;
        long long          int64_value;
        unsigned long      uint32_value;
        unsigned long long uint64_value;
        float              float32_value;
        double             float64_value;
        const char        *string_value;
    }value;
};

struct type_info_struct_t: public type_info_t {
    const u32                 member_count;
    const type_info_member_t *member_array;
};

struct type_info_procedure_t: public type_info_t {
    const u32                 argument_count;
    const type_info_member_t *argument_array;
};

extern constexpr type_info_t *APOLLO_type_table[];
extern constexpr type_info_t *APOLLO_member_table[];

struct type_info_string_t: public type_info_struct_t {
	const type_info_member_t *data;
	const type_info_member_t *count;
};

struct type_info_RHI_pipeline_state_t: public type_info_struct_t {
	const type_info_member_t *blend_enabled;
	const type_info_member_t *src_color_blend_mode;
	const type_info_member_t *dst_color_blend_mode;
	const type_info_member_t *src_alpha_blend_mode;
	const type_info_member_t *dst_alpha_blend_mode;
	const type_info_member_t *color_blend_op;
	const type_info_member_t *alpha_blend_op;
	const type_info_member_t *depth_testing_enabled;
	const type_info_member_t *depth_writing_enabled;
	const type_info_member_t *depth_func;
	const type_info_member_t *stencil_enabled;
	const type_info_member_t *stencil_state;
	const type_info_member_t *stencil_keep;
	const type_info_member_t *polygon_mode;
	const type_info_member_t *primitive_type;
};

struct type_info_asset_handle_t: public type_info_struct_t {
	const type_info_member_t *is_valid;
	const type_info_member_t *slot;
	const type_info_member_t *texture;
	const type_info_member_t *shader;
	const type_info_member_t *material_info;
	const type_info_member_t *dynamic_render_font;
};

struct type_info_material_instance_t: public type_info_struct_t {
	const type_info_member_t *ID;
	const type_info_member_t *version;
	const type_info_member_t *name;
	const type_info_member_t *renderer_effect_flags;
	const type_info_member_t *shader_uniform_count;
	const type_info_member_t *pipeline_state;
	const type_info_member_t *archetype;
};

struct type_info_material_archetype_t: public type_info_struct_t {
	const type_info_member_t *ID;
	const type_info_member_t *version;
	const type_info_member_t *name;
	const type_info_member_t *shader_binary_name;
	const type_info_member_t *shader_handle;
	const type_info_member_t *base_instance;
};

constexpr type_info_byte DEFAULT_byte = ;
constexpr type_info_int64_t DEFAULT_int64_t = ;
constexpr type_info_string DEFAULT_string = ;
constexpr type_info_s32 DEFAULT_s32 = ;
constexpr type_info_uint32_t DEFAULT_uint32_t = ;
constexpr type_info_RHI_pipeline_state DEFAULT_RHI_pipeline_state = ;

constexpr type_info_t *APOLLO_type_table[] = {
};
constexpr type_info_t *APOLLO_member_table[] = {
};
