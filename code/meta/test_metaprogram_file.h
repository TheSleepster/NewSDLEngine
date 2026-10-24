struct type_info_t {
    const char *name;
    const unsigned long size = 0;
    const unsigned long metatype = 0;
};

struct type_info_member_t: public type_info_t {
    const type_info_t *type_info;
    const unsigned long offset = 0;
    const unsigned long value_type = 0;
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
    const unsigned long member_count = 0;
    union {
        const type_info_member_t *member_array;
        struct {}members;
    };
};

struct type_info_procedure_t: public type_info_t {
    const unsigned long argument_count = 0;
    union {
        const type_info_member_t *argument_array;
        struct {}arguments;
    };
};

extern const type_info_t *const APOLLO_type_table[];
extern const type_info_t *const APOLLO_member_table[];

struct type_info_byte: public type_info_t { long padding; };
struct type_info_int64_t: public type_info_t { long padding; };
struct type_info_string_t: public type_info_struct_t {
	union {
		const type_info_member_t *member_array[2];
		struct {
			const type_info_member_t *data;
			const type_info_member_t *count;
		}members;
	};
};

struct type_info_s32: public type_info_t { long padding; };
struct type_info_uint32_t: public type_info_t { long padding; };
struct type_info_RHI_pipeline_state_t: public type_info_struct_t {
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

struct type_info_uint64_t: public type_info_t { long padding; };
struct type_info_bool32: public type_info_t { long padding; };
struct type_info_asset_handle_t: public type_info_struct_t {
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

struct type_info_material_instance_t: public type_info_struct_t {
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

struct type_info_material_archetype_t: public type_info_struct_t {
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

//constexpr type_info_byte DEFAULT_byte = {};
//constexpr type_info_int64_t DEFAULT_int64_t = {};
constexpr type_info_string_t DEFAULT_string_t = {
	.members = {
		.data = {
			.name = "data",
			.size = 8,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 0,
		},
		.count = {
			.name = "count",
			.size = 8,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 8,
		},
	},
};
//constexpr type_info_s32 DEFAULT_s32 = {};
//constexpr type_info_uint32_t DEFAULT_uint32_t = {};
constexpr type_info_RHI_pipeline_state_t DEFAULT_RHI_pipeline_state_t = {
	.members = {
		.blend_enabled = {
			.name = "blend_enabled",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 0,
			.value_type = 2,
			.value = {
				.signed_long = 1,
			},
		},
		.src_color_blend_mode = {
			.name = "src_color_blend_mode",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 4,
			.value_type = 3,
			.value = {
				.unsigned_value = 6,
			},
		},
		.dst_color_blend_mode = {
			.name = "dst_color_blend_mode",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 8,
			.value_type = 3,
			.value = {
				.unsigned_value = 7,
			},
		},
		.src_alpha_blend_mode = {
			.name = "src_alpha_blend_mode",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 12,
			.value_type = 3,
			.value = {
				.unsigned_value = 1,
			},
		},
		.dst_alpha_blend_mode = {
			.name = "dst_alpha_blend_mode",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 16,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.color_blend_op = {
			.name = "color_blend_op",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 20,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.alpha_blend_op = {
			.name = "alpha_blend_op",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 24,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.depth_testing_enabled = {
			.name = "depth_testing_enabled",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 28,
			.value_type = 2,
			.value = {
				.signed_long = 1,
			},
		},
		.depth_writing_enabled = {
			.name = "depth_writing_enabled",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 32,
			.value_type = 2,
			.value = {
				.signed_long = 1,
			},
		},
		.depth_func = {
			.name = "depth_func",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 36,
			.value_type = 3,
			.value = {
				.unsigned_value = 1,
			},
		},
		.stencil_enabled = {
			.name = "stencil_enabled",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 40,
			.value_type = 2,
			.value = {
				.signed_long = 0,
			},
		},
		.stencil_state = {
			.name = "stencil_state",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 44,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.stencil_keep = {
			.name = "stencil_keep",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 48,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.polygon_mode = {
			.name = "polygon_mode",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 52,
			.value_type = 3,
			.value = {
				.unsigned_value = 0,
			},
		},
		.primitive_type = {
			.name = "primitive_type",
			.size = 4,
			.metatype = 0,
			.type_info = nullptr,
			.offset = 56,
			.value_type = 3,
			.value = {
				.unsigned_value = 3,
			},
		},
	},
};

constexpr const type_info_t *const APOLLO_type_table[] = {
};
constexpr const type_info_t *const APOLLO_member_table[] = {
};
