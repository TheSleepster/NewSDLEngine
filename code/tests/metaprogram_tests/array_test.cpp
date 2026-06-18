struct item_t {
    char *data[3][3];
};

struct item2_t {
    char *data[6];
};

#define DynArray_t(type) TypeOf((type*)NULL)

struct dynarray_thing {
    DynArray_t(u32) items;
};