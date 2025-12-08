#if !defined(C_ZONE_ALLOCATOR_H)
/* ========================================================================
   $File: c_zone_allocator.h $
   $Date: December 08 2025 08:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_ZONE_ALLOCATOR_H
#if OS_LINUX
#include <SDL3/SDL.h>
#endif

#include <c_base.h>
#include <c_types.h>
#include <c_synchronization.h>

#include <stdlib.h>

/*===========================================
  =========== ZONE ALLOCATOR API ============
  ===========================================*/
#define DEBUG_ZONE_ID            0x1d4a11
#define MAX_MEMORY_FRAGMENTATION 64

typedef enum za_allocation_tag
{
    ZA_TAG_NONE       = 0,
    ZA_TAG_STATIC     = 1, // Entire runtime
    ZA_TAG_TEXTURE    = 2,
    ZA_TAG_SOUND      = 3,
    ZA_TAG_FONT       = 4,

    // >= 100 are purgeable when needed
    ZA_TAG_PURGELEVEL = 100,
    ZA_TAG_CACHE      = 101,
}za_allocation_tag_t;

typedef struct zone_allocator_block
{
    u32                          block_id;
    bool8                        is_allocated;
    u64                          block_size;
    u64                          allocation_tag;

    struct zone_allocator_block *next_block;
    struct zone_allocator_block *prev_block;
}zone_allocator_block_t;

typedef struct zone_allocator
{
    sys_mutex_t             mutex;
    u64                     capacity;
    u8                     *base;

    zone_allocator_block_t  first_block;
    zone_allocator_block_t *cursor;
}zone_allocator_t;

//////////// ZONE ALLOCATOR API DEFINITIONS /////////////
#define c_za_push_struct(zone, type, tag)        (type*)c_za_alloc(zone, sizeof(type), tag);
#define c_za_push_array(zone, type, count, tag)  (type*)c_za_alloc(zone, sizeof(type) * count, tag);

zone_allocator_t* c_za_create(u64 block_size);
void              c_za_destroy(zone_allocator_t *zone);
byte*             c_za_alloc(zone_allocator_t *zone, u64 size_init, za_allocation_tag_t tag);
void              c_za_free(zone_allocator_t  *zone, void *data);
void              c_za_free_zone_tag(zone_allocator_t *zone, za_allocation_tag_t tag);
void              c_za_free_zone_tag_range(zone_allocator_t *zone, za_allocation_tag_t low_tag, za_allocation_tag_t high_tag);
void              c_za_change_zone_tag(zone_allocator_t *zone, void *pointer, za_allocation_tag_t new_tag);

// DEBUG FUNCTIONS
void c_za_DEBUG_print_block_list(zone_allocator_t *zone);
void c_za_DEBUG_validate_block_list(zone_allocator_t *zone);
/////////////////////////////////////////////////////////

#endif // C_ZONE_ALLOCATOR_H

