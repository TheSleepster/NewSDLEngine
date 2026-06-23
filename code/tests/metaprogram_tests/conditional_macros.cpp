#if APPLES
struct apples {
    u32 we_should_not_see_this;
};
#elif ORANGES 
struct oranges {
    u32 still_should_not_see_this;
};
#else
struct grapes {
    u32 definitely_shouldnt_see_this;
};
#endif


#if !defined(BLAH)
#define BLAH

struct should_see_this {
    u32 blah_value;
};

#define LIST_ITEMS(X)
    X(ITEM_THING, "Thing") \
    X(ITEM_BLAH,  "BLAH")

char *items = {
#define X(enum, string) string,
    LIST_ITEMS(X)
#undef X
};

#endif