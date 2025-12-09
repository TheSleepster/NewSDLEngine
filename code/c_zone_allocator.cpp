/* ========================================================================
   $File: c_zone_allocator.cpp $
   $Date: December 08 2025 08:04 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <c_zone_allocator.h>
#include <p_platform_data.h>

///////////////////
// ZONE ALLOCATOR
///////////////////

zone_allocator_t*
c_za_create(u64 block_size)
{
    zone_allocator_t *result = null;
    void *base          = sys_allocate_memory(block_size + sizeof(zone_allocator_t));

    result              = (zone_allocator_t*)base;
    result->base        = (u8*)base + sizeof(zone_allocator_t);
    result->capacity    = block_size;
    result->mutex       = sys_mutex_create();


    zone_allocator_block_t *block      = (zone_allocator_block_t *)(result->base);
    result->first_block.prev_block     = block;
    result->first_block.next_block     = result->first_block.prev_block;

    result->first_block.is_allocated   = true;
    result->first_block.allocation_tag = ZA_TAG_STATIC;
    result->first_block.block_id       = DEBUG_ZONE_ID;
    result->cursor                     = block;

    block->next_block             = &result->first_block;
    block->prev_block             =  block->next_block;
    block->next_block->prev_block =  block;
    block->is_allocated           =  false;
    block->block_size             =  result->capacity;
    block->block_id               =  DEBUG_ZONE_ID;

    return(result);
}

void
c_za_destroy(zone_allocator_t *zone)
{
    sys_free_memory(zone, zone->capacity + sizeof(zone_allocator_t));
    zone = null;
}

byte*
c_za_alloc(zone_allocator_t *zone, u64 size_init, za_allocation_tag_t tag)
{
    byte *result = null;
    u64 size = (size_init + 15) & ~15;
    size     = size + sizeof(zone_allocator_block_t);

    zone_allocator_block_t *base_block = zone->cursor;
    if(!base_block->prev_block->is_allocated)
    {
        base_block = base_block->prev_block;
    }

    zone_allocator_block_t *block_cursor   = base_block;
    zone_allocator_block_t *starting_block = base_block->prev_block;

    while(base_block->is_allocated || base_block->block_size < size)
    {
        if(block_cursor->is_allocated)
        {
            if(block_cursor->allocation_tag >= ZA_TAG_PURGELEVEL)
            {
                // NOTE(Sleepster): Free this block... 
                base_block = base_block->prev_block;
                c_za_free(zone, (byte *)block_cursor + sizeof(zone_allocator_block_t));

                base_block   = base_block->next_block;
                block_cursor = base_block->next_block;
            }
            else
            {
                // NOTE(Sleepster): This block cannot be freed, go next 
                block_cursor  = block_cursor->next_block;
                base_block    = block_cursor;
            }
        }
        else
        {
            block_cursor = block_cursor->next_block;
        }

        if(block_cursor == starting_block)
        {
            log_fatal("failed to allocate memory to the zone allocator... allocation size of: %d...\n", size);
            return(result);
        }
    }

    u64 leftover_memory = base_block->block_size - size;
    if(leftover_memory > MAX_MEMORY_FRAGMENTATION)
    {
        zone_allocator_block_t *new_block = (zone_allocator_block_t *)((byte*)base_block + size);
        new_block->block_size     = leftover_memory;
        new_block->is_allocated   = false;
        new_block->allocation_tag = ZA_TAG_NONE;
        new_block->prev_block     = base_block;
        new_block->next_block     = base_block->next_block;
        new_block->next_block->prev_block = new_block;
        new_block->block_id = 0;

        base_block->next_block = new_block;
        base_block->block_size = size;
    }

    base_block->is_allocated   = true;
    base_block->allocation_tag = tag;
    base_block->block_id       = DEBUG_ZONE_ID;
    zone->cursor               = base_block->next_block;

    result = (byte*)base_block + sizeof(zone_allocator_block_t);
    memset(result, 0, size - sizeof(zone_allocator_block_t));

    log_info("Zone Allocated: %d bytes...\n", size);
    return(result);
}

void
c_za_free(zone_allocator_t *zone, void *data)
{
    zone_allocator_block_t *block = null;
    zone_allocator_block_t *other = null;

    block = (zone_allocator_block_t *)((byte*)data - sizeof(zone_allocator_block_t));
    Assert(block->block_id == DEBUG_ZONE_ID);
    u64 block_size                = block->block_size;
    za_allocation_tag_t block_tag = (za_allocation_tag_t)block->allocation_tag;
    if(block->is_allocated)
    {
        block->is_allocated   = false;
        block->allocation_tag = ZA_TAG_NONE;
        block->block_id       = 0;
        
        other = block->prev_block;
        if(!other->is_allocated)
        {
            other->block_size += block->block_size;
            other->next_block  = block->next_block;
            other->next_block->prev_block = other;
            if(block == zone->cursor)
            {
                zone->cursor = other;
            }
            block = other;
        }

        other = block->next_block;
        if(!other->is_allocated)
        {
            block->block_size            += other->block_size;
            block->next_block             = other->next_block;
            block->next_block->prev_block = block;
            if(other == zone->cursor)
            {
                zone->cursor = block;
            }
        }
        data = null;
        log_info("Freed a zone block with a size of '%d'... had an allocation tag of '%d'...\n", block_size, block_tag);
    }
    else
    {
        log_error("Attempted to free a block in the zone allocator that has not been allocated...\n");
    }
}

void
c_za_free_zone_tag(zone_allocator_t *zone, za_allocation_tag_t tag)
{
    for(zone_allocator_block_t *current_block = &zone->first_block;
        current_block;
        current_block = current_block->next_block)
    {
        if(current_block->is_allocated)
        {
            if((za_allocation_tag_t)current_block->allocation_tag == tag)
            {
                c_za_free(zone, current_block);
                log_info("Freed block with tag: '%d'...\n", tag);
            }
        }
    }
}

void
c_za_free_zone_tag_range(zone_allocator_t *zone, za_allocation_tag_t low_tag, za_allocation_tag_t high_tag)
{
    log_info("Freed blocks with tag range: '%d' to '%d'...\n", low_tag, high_tag);
    for(zone_allocator_block_t *current_block = &zone->first_block;
        current_block;
        current_block = current_block->next_block)
    {
        if(current_block->is_allocated)
        {
            if((za_allocation_tag_t)current_block->allocation_tag > low_tag &&
               (za_allocation_tag_t)current_block->allocation_tag < (za_allocation_tag_t)high_tag)
            {
                c_za_free_zone_tag(zone, (za_allocation_tag_t)current_block->allocation_tag);
                log_info("Freed block with tag: '%d'...\n", current_block->allocation_tag);
            }
        }
    }
}

void
c_za_change_zone_tag(zone_allocator_t *zone, void *pointer, za_allocation_tag_t new_tag)
{
    zone_allocator_block_t *block = (zone_allocator_block_t *)((byte*)pointer - sizeof(zone_allocator_block_t));
    if(block->block_id != DEBUG_ZONE_ID)
    {
        log_error("Cannot change the tag of this zone, the block_id is invalid...\n");
    }

    block->allocation_tag = new_tag;
}

void
c_za_DEBUG_print_block_list(zone_allocator_t *zone)
{
    zone_allocator_block_t *block = zone->first_block.next_block;
    log_info("Block List State:");
    while(block != &zone->first_block)
    {
        log_info("  Block at %u: size= %u, allocated= %u, tag= %d, id= %u",
            block, block->block_size, block->is_allocated, block->allocation_tag, block->block_id);
        block = block->next_block;
    }
    log_info("  Cursor at %u...", zone->cursor);
}

void
c_za_DEBUG_validate_block_list(zone_allocator_t *zone)
{
    zone_allocator_block_t *block = zone->first_block.next_block;
    if(block == &zone->first_block)
    {
        log_error("Zone Allocator block list is empty...");
    }

    for(;;)
    {
        Assert(block->prev_block->next_block == block);
        Assert(block->next_block->prev_block == block);

        block = block->next_block;
        if(block == &zone->first_block)
        {
            break;
        }
    }
    Assert(block == &zone->first_block);
}

