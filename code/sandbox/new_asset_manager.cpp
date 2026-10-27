/* ========================================================================
   $File: new_asset_manager.cpp $
   $Date: October 26 2026 07:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include <stdio.h>

template <typename T>
struct asset_catalog_t
{
    struct asset_slot_t
    {
        u64      ID;
        u32      slot_state;
        string_t name;

        T        asset;
    };

    hash_table_t<asset_slot_t> asset_lookup;
    dynarray_t<asset_slot_t*>  loaded_assets;
};

int
main(void)
{
    printf("Hello World!\n");

#if 0 
    
#endif
}
