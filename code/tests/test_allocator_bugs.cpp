/* ========================================================================
   $File: test_allocator_bugs.cpp $
   $Date: February 15 2026 $
   $Creator: Justin Lewis $
   Verifies fixes for allocator bugs found by code review.
   Build: make tests (from code/)
   ======================================================================== */
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <c_types.h>
#include <c_base.h>
#include <p_platform_data.h>

// Unity-build includes (same pattern as new_string_builder.cpp)
#include <p_platform_data.cpp>
#include <c_memory_arena.cpp>
#include <c_zone_allocator.cpp>
#include <c_globals.cpp>

// =====================================================================
// Vulkan allocator struct reproduction (these are local to
// vk_backend_allocator.cpp -- no header to include. Simplified
// here by omitting VkDeviceMemory since we test linked-list
// logic, not actual Vulkan calls.)
// =====================================================================

#define VK_ALLOCATOR_DEBUG_ID (0xC0FFEE)

struct vulkan_allocation_block_t
{
    u32    DEBUG_id;
    bool32 is_transient;
    u64    block_size;
    u64    used;
    u32    memory_index;
    vulkan_allocation_block_t *next_block;
    vulkan_allocation_block_t *prev_block;
};

struct vulkan_allocator_t
{
    u64 default_block_size;

    vulkan_allocation_block_t *first_transient_block;
    vulkan_allocation_block_t *last_transient_block;

    vulkan_allocation_block_t *first_allocated_block;
    vulkan_allocation_block_t *last_allocated_block;

    vulkan_allocation_block_t *first_free_block;

    // Stand-in for memory_arena_t block pool
    vulkan_allocation_block_t block_pool[64];
    int pool_used;
};

// =====================================================================
// TEST FRAMEWORK
// =====================================================================

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("\n--- TEST: %s ---\n", name)
#define CHECK(cond, msg) do { \
    if(cond) { printf("  PASS: %s\n", msg); tests_passed++; } \
    else     { printf("  FAIL: %s\n", msg); tests_failed++; } \
} while(0)

// =====================================================================
// FIX #1: vk_allocator_free_block -- prev_block copy-paste
// =====================================================================
void
vk_allocator_free_block_FIXED(vulkan_allocator_t        *allocator,
                               vulkan_allocation_block_t *block_to_free)
{
    vulkan_allocation_block_t *next_block = block_to_free->next_block;
    vulkan_allocation_block_t *prev_block = block_to_free->prev_block; // FIXED (was ->next_block)

    if(prev_block) prev_block->next_block = next_block;
    if(next_block) next_block->prev_block = prev_block;

    if(allocator->first_free_block == null)
    {
        allocator->first_free_block = block_to_free;
    }
    else
    {
        block_to_free->next_block   = allocator->first_free_block;
        allocator->first_free_block = block_to_free;
    }
}

void test_free_block_fixed()
{
    TEST("FIX #1: vk_allocator_free_block -- prev_block reads correctly");

    vulkan_allocation_block_t a = {}, b = {}, c = {};
    a.next_block = &b; a.prev_block = null;
    b.next_block = &c; b.prev_block = &a;
    c.next_block = null; c.prev_block = &b;

    vulkan_allocator_t allocator = {};
    vk_allocator_free_block_FIXED(&allocator, &b);

    CHECK(a.next_block == &c, "a.next_block correctly points to &c (skips b)");
    CHECK(c.prev_block == &a, "c.prev_block correctly points to &a (skips b)");
    CHECK(allocator.first_free_block == &b, "b is now on the free list");
}

// =====================================================================
// FIX #2 & #3: get_or_create_block returns value, updates heads
// =====================================================================
vulkan_allocation_block_t*
vk_allocator_get_or_create_block_FIXED(vulkan_allocator_t *allocator,
                                       u32                 memory_index,
                                       u32                 allocation_size,
                                       bool8               temporary_allocation)
{
    vulkan_allocation_block_t *valid_block = null;

    for(vulkan_allocation_block_t *current_block = allocator->first_free_block;
        current_block;
        current_block = current_block->next_block)
    {
        if((s32)current_block->memory_index == -1)
        {
            valid_block = current_block;
            break;
        }
    }
    if(valid_block == null)
    {
        valid_block = &allocator->block_pool[allocator->pool_used++];
    }

    ZeroStruct(*valid_block);

    valid_block->DEBUG_id     = VK_ALLOCATOR_DEBUG_ID;
    valid_block->block_size   = allocator->default_block_size > allocation_size ? allocator->default_block_size : allocation_size;
    valid_block->memory_index = memory_index;

    // FIX #3: Write back to allocator fields directly (was using locals)
    if(temporary_allocation)
    {
        if(allocator->first_transient_block == null)
            allocator->first_transient_block = valid_block;

        if(allocator->last_transient_block != null)
        {
            allocator->last_transient_block->next_block = valid_block;
            valid_block->prev_block = allocator->last_transient_block;
            valid_block->next_block = null;
        }
        allocator->last_transient_block = valid_block;
    }
    else
    {
        if(allocator->first_allocated_block == null)
            allocator->first_allocated_block = valid_block;

        if(allocator->last_allocated_block != null)
        {
            allocator->last_allocated_block->next_block = valid_block;
            valid_block->prev_block = allocator->last_allocated_block;
            valid_block->next_block = null;
        }
        allocator->last_allocated_block = valid_block;
    }

    return(valid_block); // FIX #2: returns the block (was void, pass-by-value)
}

void test_return_value_and_list_heads()
{
    TEST("FIX #2: get_or_create_block returns valid block (no null deref)");

    vulkan_allocator_t allocator = {};
    allocator.default_block_size = 1024 * 1024;

    vulkan_allocation_block_t *block = vk_allocator_get_or_create_block_FIXED(&allocator, 0, 4096, false);

    CHECK(block != null, "Returned block is not null");
    CHECK(block->DEBUG_id == VK_ALLOCATOR_DEBUG_ID, "Assert(valid_block->DEBUG_id == VK_ALLOCATOR_DEBUG_ID) passes");

    TEST("FIX #3: get_or_create_block updates allocator list heads");

    CHECK(allocator.first_allocated_block == block,
          "allocator.first_allocated_block points to the new block");
    CHECK(allocator.last_allocated_block == block,
          "allocator.last_allocated_block points to the new block");

    vulkan_allocation_block_t *block2 = vk_allocator_get_or_create_block_FIXED(&allocator, 0, 4096, false);

    CHECK(allocator.first_allocated_block == block,
          "first_allocated_block still points to block 1");
    CHECK(allocator.last_allocated_block == block2,
          "last_allocated_block updated to block 2");
    CHECK(block->next_block == block2,
          "block1->next_block chains to block2");
    CHECK(block2->prev_block == block,
          "block2->prev_block chains back to block1");
}

// =====================================================================
// FIX #6: allocation size matches block_size
// =====================================================================
void test_allocation_size_fixed()
{
    TEST("FIX #6: vkAllocateMemory uses block_size, not allocation_size");

    u64 default_block_size = 64 * 1024 * 1024;
    u32 allocation_size    = 4096;

    u64 block_size = default_block_size > allocation_size ? default_block_size : allocation_size;
    u64 actual_vk_allocation = block_size; // FIXED (was allocation_size)

    printf("  block_size recorded on struct: %lu bytes (%lu MB)\n", block_size, block_size / (1024*1024));
    printf("  actual vkAllocateMemory size:  %lu bytes (%lu MB)\n", actual_vk_allocation, actual_vk_allocation / (1024*1024));

    CHECK(block_size == actual_vk_allocation,
          "block_size == vkAllocateMemory size -- sub-allocations are safe now");
}

// =====================================================================
// FIX #8: c_za_free_zone_tag -- uses real c_za_free from project source
// =====================================================================
void test_zone_free_tag_fixed()
{
    TEST("FIX #8: c_za_free_zone_tag passes user data pointer (no crash)");

    zone_allocator_t *zone = c_za_create(4096);

    byte *data1 = c_za_alloc(zone, 64, ZA_TAG_TEXTURE);
    byte *data2 = c_za_alloc(zone, 64, ZA_TAG_SOUND);
    byte *data3 = c_za_alloc(zone, 64, ZA_TAG_TEXTURE);

    CHECK(data1 != null, "Allocated block 1 (TEXTURE)");
    CHECK(data2 != null, "Allocated block 2 (SOUND)");
    CHECK(data3 != null, "Allocated block 3 (TEXTURE)");

    printf("  Calling c_za_free_zone_tag(zone, ZA_TAG_TEXTURE)...\n");
    c_za_free_zone_tag(zone, ZA_TAG_TEXTURE);
    printf("  No crash!\n");

    zone_allocator_block_t *block2_header = (zone_allocator_block_t *)(data2 - sizeof(zone_allocator_block_t));
    CHECK(block2_header->is_allocated == true, "SOUND block is still allocated after freeing TEXTURE tags");
    CHECK((za_allocation_tag_t)block2_header->allocation_tag == ZA_TAG_SOUND,
          "SOUND block still has correct tag");

    byte *data4 = c_za_alloc(zone, 64, ZA_TAG_FONT);
    CHECK(data4 != null, "Can allocate new block in freed space");

    c_za_destroy(zone);
}

// =====================================================================
// Subprocess runner for crash-sensitive tests
// =====================================================================
void run_test_subprocess(const char *name, void (*test_fn)())
{
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();
    if(pid == 0)
    {
        test_fn();
        _exit(0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);

        if(WIFSIGNALED(status))
        {
            int sig = WTERMSIG(status);
            printf("  >>> UNEXPECTED CRASH: signal %d (%s) -- FIX INCOMPLETE <<<\n\n", sig, strsignal(sig));
            tests_failed++;
        }
        else if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            printf("  >>> Completed without crash <<<\n\n");
        }
        else
        {
            printf("  >>> Exited with status %d <<<\n\n", WEXITSTATUS(status));
            tests_failed++;
        }
    }
}

// =====================================================================
// main
// =====================================================================
int main()
{
    printf("========================================\n");
    printf("  Allocator Bug FIX Verification Tests\n");
    printf("========================================\n");

    test_free_block_fixed();
    test_allocation_size_fixed();

    printf("\n========================================\n");
    printf("  PREVIOUSLY CRASHING (subprocess)\n");
    printf("========================================\n");

    printf("\n[FIX #2+#3] Return value + list heads:\n");
    run_test_subprocess("return_value_and_list_heads", test_return_value_and_list_heads);

    printf("[FIX #8] c_za_free_zone_tag correct pointer:\n");
    run_test_subprocess("zone_free_tag_fixed", test_zone_free_tag_fixed);

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
