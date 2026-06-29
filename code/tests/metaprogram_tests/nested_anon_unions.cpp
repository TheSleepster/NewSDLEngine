typedef struct test_structure {
    union {
        int internal_to_union;

        struct {
            int internal_to_structure_inside_union;
        };

        struct {
            int other_internal_to_structure_inside_union[2];
        };
    };
}test_structure_t;
