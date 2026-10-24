struct type_info_t {
    const char *name;
    const unsigned long size;
    const unsigned long metatype;
};

struct type_info_member_t {
    const type_info_t  member_data;
    const type_info_t *type_info;
    const unsigned long offset;
    const unsigned long value_type;
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

struct type_info_struct_t {
    const type_info_t   info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array;
        struct {}members;
    };
};

struct type_info_procedure_t {
    const type_info_t   info;
    const unsigned long argument_count;
    union {
        const type_info_member_t *argument_array;
        struct {}arguments;
    };
};

extern const type_info_t *const APOLLO_type_table[];
extern const type_info_t *const APOLLO_member_table[];

struct type_info_byte { const type_info_t info; };
struct type_info_int64_t { const type_info_t info; };
struct type_info_string_t {
    const type_info_t info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array[2];
        struct {
			const type_info_member_t *data;
			const type_info_member_t *count;
		}members;
	};
};

struct type_info_s32 { const type_info_t info; };
struct type_info_uint32_t { const type_info_t info; };
struct type_info_RHI_pipeline_state_t {
    const type_info_t info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array[15];
        struct {
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
		}members;
	};
};

struct type_info_uint64_t { const type_info_t info; };
struct type_info_bool32 { const type_info_t info; };
struct type_info_asset_handle_t {
    const type_info_t info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array[6];
        struct {
			const type_info_member_t *is_valid;
			const type_info_member_t *slot;
			const type_info_member_t *texture;
			const type_info_member_t *shader;
			const type_info_member_t *material_info;
			const type_info_member_t *dynamic_render_font;
		}members;
	};
};

struct type_info_material_instance_t {
    const type_info_t info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array[7];
        struct {
			const type_info_member_t *ID;
			const type_info_member_t *version;
			const type_info_member_t *name;
			const type_info_member_t *renderer_effect_flags;
			const type_info_member_t *shader_uniform_count;
			const type_info_member_t *pipeline_state;
			const type_info_member_t *archetype;
		}members;
	};
};

struct type_info_material_archetype_t {
    const type_info_t info;
    const unsigned long member_count;
    union {
        const type_info_member_t *member_array[6];
        struct {
			const type_info_member_t *ID;
			const type_info_member_t *version;
			const type_info_member_t *name;
			const type_info_member_t *shader_binary_name;
			const type_info_member_t *shader_handle;
			const type_info_member_t *base_instance;
		}members;
	};
};

constexpr type_info_byte DEFAULT_byte = {.info = {.name = "byte", .size = 0, .metatype = 0}};
constexpr type_info_int64_t DEFAULT_int64_t = {.info = {.name = "int64_t", .size = 0, .metatype = 0}};
constexpr type_info_member_t DEFAULT_member_string_t_data = {
	.member_data = {.name = "string_t", .size = 8, .metatype = 0 },
	.type_info = &DEFAULT_byte.info,
};

constexpr type_info_member_t DEFAULT_member_string_t_count = {
	.member_data = {.name = "string_t", .size = 8, .metatype = 0 },
	.type_info = &DEFAULT_s64.info,
};

constexpr type_info_string_t DEFAULT_string_t = {
	.info = { .name = "string_t", .size = 16, .metatype = 1 },
	.member_count = 2,
	.members = {
		&DEFAULT_member_string_t_data,
		&DEFAULT_member_string_t_count,
	},
};
constexpr type_info_s32 DEFAULT_s32 = {.info = {.name = "s32", .size = 0, .metatype = 0}};
constexpr type_info_uint32_t DEFAULT_uint32_t = {.info = {.name = "uint32_t", .size = 0, .metatype = 0}};
constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_blend_enabled = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_bool32.info,
	.value_type = 2,
	.value = {
		.signed_long = 1,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_src_color_blend_mode = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 6,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_dst_color_blend_mode = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 7,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_src_alpha_blend_mode = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 1,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_dst_alpha_blend_mode = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_color_blend_op = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_alpha_blend_op = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_depth_testing_enabled = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_bool32.info,
	.value_type = 2,
	.value = {
		.signed_long = 1,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_depth_writing_enabled = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_bool32.info,
	.value_type = 2,
	.value = {
		.signed_long = 1,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_depth_func = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 1,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_stencil_enabled = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_bool32.info,
	.value_type = 2,
	.value = {
		.signed_long = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_stencil_state = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_stencil_keep = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_polygon_mode = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 0,
	},
};

constexpr type_info_member_t DEFAULT_member_RHI_pipeline_state_t_primitive_type = {
	.member_data = {.name = "RHI_pipeline_state_t", .size = 4, .metatype = 0 },
	.type_info = &DEFAULT_u32.info,
	.value_type = 3,
	.value = {
		.unsigned_value = 3,
	},
};

constexpr type_info_RHI_pipeline_state_t DEFAULT_RHI_pipeline_state_t = {
	.info = { .name = "RHI_pipeline_state_t", .size = 60, .metatype = 1 },
	.member_count = 15,
	.members = {
		&DEFAULT_member_RHI_pipeline_state_t_blend_enabled,
		&DEFAULT_member_RHI_pipeline_state_t_src_color_blend_mode,
		&DEFAULT_member_RHI_pipeline_state_t_dst_color_blend_mode,
		&DEFAULT_member_RHI_pipeline_state_t_src_alpha_blend_mode,
		&DEFAULT_member_RHI_pipeline_state_t_dst_alpha_blend_mode,
		&DEFAULT_member_RHI_pipeline_state_t_color_blend_op,
		&DEFAULT_member_RHI_pipeline_state_t_alpha_blend_op,
		&DEFAULT_member_RHI_pipeline_state_t_depth_testing_enabled,
		&DEFAULT_member_RHI_pipeline_state_t_depth_writing_enabled,
		&DEFAULT_member_RHI_pipeline_state_t_depth_func,
		&DEFAULT_member_RHI_pipeline_state_t_stencil_enabled,
		&DEFAULT_member_RHI_pipeline_state_t_stencil_state,
		&DEFAULT_member_RHI_pipeline_state_t_stencil_keep,
		&DEFAULT_member_RHI_pipeline_state_t_polygon_mode,
		&DEFAULT_member_RHI_pipeline_state_t_primitive_type,
	},
};

constexpr const type_info_t *const APOLLO_type_table[] = {
};
constexpr const type_info_t *const APOLLO_member_table[] = {
};
