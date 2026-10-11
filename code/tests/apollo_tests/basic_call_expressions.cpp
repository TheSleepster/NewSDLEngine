/* ========================================================================
   $File: basic_call_expressions.cpp $
   $Date: October 11 2026 12:25 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

struct world_chunk_t
{
    int x;
    int y;
    int z;
};

template <typename T> void *type_info()

void *type_info(const char *name);
void *type_info(const void *variable);

void
metaprogram(void)
{
    world_chunk_t chunk;

    void *type_information = type_info<world_chunk_t>();
    void *string_request   = type_info("world_chunk_t");
    void *variable_request = type_info(chunk);

    (void)chunk;
    (void)type_information;
    (void)string_request;
    (void)variable_request;
}
