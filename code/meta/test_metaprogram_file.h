
struct type_info_t {
    const char *name;
    const u32   size;
};

struct type_info_struct_t: public type_info_t {
    const u32          member_count;
    const type_info_t *member_array;
};
struct type_info_string {
};
struct type_info_byte {
};
struct type_info_s64 {
};
struct type_info_RHI_pipeline_state {
};
struct type_info_bool32 {
};
struct type_info_u32 {
};
struct type_info_material_archetype {
};
struct type_info_u64 {
};
struct type_info_string_t {
};
struct type_info_asset_handle_t {
};
struct type_info_material_instance_t {
};
