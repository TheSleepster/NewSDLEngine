/* ========================================================================
   $File: old_metagen.cpp $
   $Date: February 05 2026 04:40 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
c_dynarray_for(ast->structures, type_index)
{
    meta_struct_t *structure = c_dynarray_get_ptr(ast->structures, type_index);
    string_t struct_canonical_type_name = get_canonical_type_name(&structure->type_data);

    if((structure->type_data.modifier_flags & META_TYPE_FLAGS_PrivatelyDeclared) == 0)
    {
        fprintf(stdout, "const static %.*s GENERATED_DEFAULT_%.*s = {};\n",
                struct_canonical_type_name.count, C_STR(struct_canonical_type_name),
                struct_canonical_type_name.count, C_STR(struct_canonical_type_name));
    }

    c_string_builder_sprintf(struct_info_builder, "const static type_info_struct_%.*s type_info_struct_%.*s_const_data = {\n", 
                             structure->type_data.type_name.count, C_STR(structure->type_data.type_name),
                             structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
    c_string_builder_sprintf(struct_info_builder, "\t.name = \"%.*s,\",\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
    c_string_builder_sprintf(struct_info_builder, "\t.type = TYPE_%.*s,\n", structure->type_data.type_name.count, C_STR(structure->type_data.type_name));

    string_t structure_kind = get_metatype_kind_string(structure->type_data.kind);
    c_string_builder_sprintf(struct_info_builder, "\t.kind = %.*s,\n", structure_kind.count, C_STR(structure_kind));

    c_string_builder_sprintf(struct_info_builder, "\t.modifier_flags = ");
    append_item_modifier_flags(struct_info_builder, structure->type_data.flag_counter, structure->type_data.modifier_flags);
    c_string_builder_sprintf(struct_info_builder, ",\n");
    c_string_builder_sprintf(struct_info_builder, "\t.flag_counter = %d,\n", structure->type_data.flag_counter);

    // TODO(Sleepster): We are probably better off baking the list of nodes into a single string, so instead of the node list being constantly built,
    // we just build it once and use it the whole iteration
    meta_struct_t *struct_data = structure;

    // NOTE(Sleepster): Print out the sizeof list 
    c_string_builder_sprintf(struct_info_builder, "\t.element_size = sizeof(GENERATED_DEFAULT_%.*s",
                             struct_data->type_data.type_name.count, C_STR(struct_data->type_data.type_name));

    // TODO(Sleepster): THIS IS THE RIGHT ONE, EVERY OTHER VERSION IS WRONG. BREAK THIS OUT INTO SOMETHING
    u32 current_depth = 0;
    for(meta_struct_t *current_structure = struct_data->last_nested;
        current_structure && current_depth < Max(struct_data->nesting_depth, 0);
        current_structure = current_structure->last_nested)
    {
        c_string_builder_sprintf(struct_info_builder, ".%.*s",
                                 current_structure->type_data.type_name.count, C_STR(current_structure->type_data.type_name));
        ++current_depth;
    }

    if(structure->nesting_depth > 0)
    {
        c_string_builder_sprintf(struct_info_builder, ".%.*s);\n",
                                 structure->type_data.type_name.count, C_STR(structure->type_data.type_name));
    }
    else
    {
        c_string_builder_sprintf(struct_info_builder, ");\n");
    }
    c_string_builder_sprintf(struct_info_builder, "\t.member_count = %d,\n", structure->member_count);

    c_string_builder_sprintf(struct_info_builder, "\t.members = {\n");
    c_dynarray_for(structure->members, member_index)
    {
        meta_member_t *member = c_dynarray_get_ptr(structure->members, member_index);

        string_t kind_string         = get_metatype_kind_string(member->type_info.kind);
        string_t canonical_type_name = get_canonical_type_name(&member->type_info);
        if((structure->type_data.modifier_flags & META_TYPE_FLAGS_PrivatelyDeclared) == 0 &&
           member->type_info.kind != META_TYPE_KIND_Struct)
        {
            fprintf(stdout, "const static %.*s GENERATED_DEFAULT_%.*s = {};\n",
                    canonical_type_name.count, C_STR(canonical_type_name),
                    canonical_type_name.count, C_STR(canonical_type_name));
        }


        c_string_builder_sprintf(struct_info_builder, "\t\t.%.*s = {.name = \"%.*s\", .type = TYPE_%.*s, .kind = %.*s, .modifier_flags = ",
                                 member->name.count, C_STR(member->name),               // member_name
                                 member->name.count, C_STR(member->name),               // .name 
                                 canonical_type_name.count, C_STR(canonical_type_name), // .type
                                 kind_string.count, C_STR(kind_string));                // .kind
        append_item_modifier_flags(struct_info_builder, member->type_info.flag_counter, member->type_info.modifier_flags);
        if(member->nested_struct)                                                                                                                     
        {
            meta_struct_t *struct_data = structure;

            // NOTE(Sleepster): Print the items that don't need to be looped over 
            c_string_builder_sprintf(struct_info_builder, ", .flag_counter = %d, .pointer_depth = %d, .array_size = %d, ",
                                     member->type_info.flag_counter,                                              // flag counter 
                                     member->type_info.pointer_depth,                                             // pointer depth
                                     member->type_info.array_size);                                               // array size

            // NOTE(Sleepster): Print out the sizeof list 
            c_string_builder_sprintf(struct_info_builder, ".size = sizeof(GENERATED_DEFAULT_%.*s",
                                     struct_data->type_data.type_name.count, C_STR(struct_data->type_data.type_name));

            current_depth = 0;
            for(meta_struct_t *current_structure = struct_data->last_nested;
                current_structure && current_depth < Max(member->nested_struct->nesting_depth, 0);
                current_structure = current_structure->last_nested)
            {
                c_string_builder_sprintf(struct_info_builder, ".%.*s",
                                         current_structure->type_data.type_name.count, C_STR(current_structure->type_data.type_name));
                ++current_depth;
            }

            // NOTE(Sleepster): Print out the OffsetOf list 
            c_string_builder_sprintf(struct_info_builder, "), .offset = IntFromPtr(OffsetOf(GENERATED_DEFAULT_%.*s",
                                     struct_data->type_data.type_name.count, C_STR(struct_data->type_data.type_name));
            current_depth = 0;
            for(meta_struct_t *current_structure = struct_data->last_nested;
                current_structure && current_depth < Max(member->nested_struct->nesting_depth - 1, 0);
                current_structure = current_structure->last_nested)
            {
                c_string_builder_sprintf(struct_info_builder, ".%.*s",
                                         current_structure->type_data.type_name.count, C_STR(current_structure->type_data.type_name));
                ++current_depth;
            }
            c_string_builder_sprintf(struct_info_builder, ", %.*s))},\n",
                                     member->name.count, C_STR(member->name));
        }
        else
        {
            c_string_builder_sprintf(struct_info_builder, ", .flag_counter = %d, .pointer_depth = %d, .array_size = %d, .size = sizeof(GENERATED_DEFAULT_%.*s), .offset = IntFromPtr(OffsetOf(GENERATED_DEFAULT_%.*s, %.*s))},\n",
                                     member->type_info.flag_counter,                                              // flag counter 
                                     member->type_info.pointer_depth,                                             // pointer depth
                                     member->type_info.array_size,                                                // array size
                                     member->type_info.type_name.count, C_STR(member->type_info.type_name),       // size of
                                     structure->type_data.type_name.count, C_STR(structure->type_data.type_name), // parent name
                                     member->name.count, C_STR(member->name));                                    // type_name
        }
    }
    c_string_builder_sprintf(struct_info_builder, "\t}\n");
    c_string_builder_sprintf(struct_info_builder, "};\n");
}
