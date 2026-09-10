/* ========================================================================
   $File: allocator_benchmark.cpp $
   $Date: September 10 2026 $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
//  !!!!!!!!!!!THIS ENTIRE FILE IS LLM GENERATED!!!!!!!!!!
//
//    I'm too lazy to write an actual test
//
// Benchmark: Custom allocator vs GlibC malloc on real workloads.
//
// Compile:  make sandboxes   (or: make BUILD_DIR=../build sandboxes)
// Run:      ../build/sandbox_allocator_benchmark
//
// The allocator is included from new_malloc.cpp with DEBUG=0 (release mode,
// no guard-page mprotect overhead) so we measure raw allocator speed.
// Define ALLOC_BENCH_DEBUG=1 below to test with guard pages enabled.
//
// To test with DEBUG (guard pages), set:
//   #define ALLOC_BENCH_DEBUG 1
// before the allocator include.
//
// Methodology (kept purely empirical / impartial):
//   - Every test runs the IDENTICAL workload on the custom allocator and
//     on glibc malloc (same sizes, same counts, same RNG seeds, same order).
//   - Each allocator gets an untimed warmup of the exact same workload
//     before its timed runs, so one-time costs (thread registration, first
//     page allocation, glibc arena init) do not bias the measurement.
//   - Measurements are best-of BENCH_ITERATIONS (7) wall-clock runs.
//   - Results are recorded into a registry and summarized per test section
//     at the end: Single-Threaded, Single-Threaded LARGE, Multi-Threaded,
//     Multi-Threaded LARGE, and Realistic workloads.
//
// NOTE on the custom allocator's 4 GB shared pool:
//   The pool capacity is GB(4). Test sizes are chosen so peak live
//   allocations stay well under that ceiling. All interleaved tests keep
//   only 1-2 blocks live at a time, and bulk tests free everything before
//   the next phase, so pages are reused from the free list rather than
//   re-reserved.
//
// NOTE on multithreading:
//   The custom allocator tracks per-thread contexts in a fixed array of
//   MAX_THREAD_COUNT (42) entries. MT benches therefore use ONE persistent
//   thread pool created once and reused for every MT test, so the total
//   number of distinct registered threads stays at (1 main + pool) and can
//   never overflow the array.

#include <c_base.h>
#include <c_types.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Configuration
// ============================================================================

#ifndef ALLOC_BENCH_DEBUG
#define ALLOC_BENCH_DEBUG 0
#endif

// Sanitizer builds (ASan/UBSan) instrument every allocation, so runs are
// several times slower. They exist to verify correctness, not to produce the
// final numbers, so under a sanitizer we scale work + iterations down. The
// release build (make sandboxes) runs the real counts.
#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
#if defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)
#define BENCH_SANITIZING 1
#endif
#endif
#ifndef BENCH_SANITIZING
#define BENCH_SANITIZING 0
#endif

#if BENCH_SANITIZING
#define BENCH_ITERATIONS    3
#define BENCH_SCALE(multi)  (((multi) + 3) / 4) // /4, min 1
#else
#define BENCH_ITERATIONS    7
#define BENCH_SCALE(multi)  ((multi))
#endif

#define BENCH_COUNT(x) (BENCH_SCALE(x))

// ============================================================================
// Pull in the allocator (release mode, no main)
// ============================================================================

#if ALLOC_BENCH_DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif
#define MAIN
#define MAX_MEMORY_SECTIONS 8192
#include "sandbox/new_malloc.cpp"

// ============================================================================
// Timing (SDL performance counters -- works on Linux + Windows)
// ============================================================================

static u64
timer_now(void)
{
    return(SDL_GetPerformanceCounter());
}

static double
timer_elapsed_ms(u64 start, u64 end)
{
    return((double)(end - start) * 1000.0 / (double)SDL_GetPerformanceFrequency());
}

// ============================================================================
// Simple LCG RNG (for random-size benchmarks)
// ============================================================================

static u32 g_rng_state = 123456789;

static u64
bench_random_size_range(u64 min_size, u64 max_size)
{
    g_rng_state = g_rng_state * 1103515245 + 12345;
    u32 raw = (g_rng_state >> 16) & 0x7FFF;
    u64 span = (max_size > min_size) ? (max_size - min_size) : 1;
    return(min_size + (raw % span));
}

// ============================================================================
// Results registry
// ============================================================================

typedef enum {
    BENCH_SEC_ST,          // single-threaded small/medium
    BENCH_SEC_ST_LARGE,    // single-threaded large allocations
    BENCH_SEC_MT,          // multi-threaded small/medium
    BENCH_SEC_MT_LARGE,    // multi-threaded large allocations
    BENCH_SEC_REALISTIC,   // realistic workload mixes
    BENCH_SEC_COUNT
} bench_section_e;

static const char *bench_section_names[BENCH_SEC_COUNT] = {
    "Single-Threaded (small/medium)",
    "Single-Threaded LARGE (MB-GB)",
    "Multi-Threaded (small/medium)",
    "Multi-Threaded LARGE (MB-GB)",
    "Realistic Workload",
};

#define MAX_BENCH_RECORDS 256

typedef struct {
    bench_section_e section;
    char            label[128];
    double          custom_ms;
    double          glibc_ms;
    s64             ops;      // allocation ops in the timed run
    u64             bytes;    // total requested bytes in the timed run
} bench_record_t;

static bench_record_t g_records[MAX_BENCH_RECORDS];
static s32 g_record_count = 0;

static void
bench_record(bench_section_e section, const char *label,
             double custom_ms, double glibc_ms, s64 ops, u64 bytes)
{
    if(g_record_count >= MAX_BENCH_RECORDS) return;
    bench_record_t *rec = &g_records[g_record_count++];
    rec->section = section;
    snprintf(rec->label, sizeof(rec->label), "%s", label);
    rec->custom_ms = custom_ms;
    rec->glibc_ms  = glibc_ms;
    rec->ops       = ops;
    rec->bytes     = bytes;
}

// ============================================================================
// Helpers
// ============================================================================

static void
print_header(const char *title)
{
    printf("\n----------------------------------------------------------------\n");
    printf("  %s\n", title);
    printf("----------------------------------------------------------------\n");
}

static void
print_row_result(const char *label, double custom_ms, double glibc_ms, s64 ops, u64 bytes)
{
    double custom_ops = (double)ops / (custom_ms / 1000.0);
    double glibc_ops  = (double)ops / (glibc_ms / 1000.0);
    double custom_mbs = (double)bytes / (custom_ms / 1000.0) / 1000000.0;
    double glibc_mbs  = (double)bytes / (glibc_ms / 1000.0) / 1000000.0;
    double ratio      = glibc_ms / custom_ms;
    printf("  %-52s custom %10.2f ms  glibc %10.2f ms  ratio %7.2fx  (%13.1fM ops/s | %14.1f MB/s)\n",
           label, custom_ms, glibc_ms, ratio, custom_ops / 1000000.0, custom_mbs);
    (void)custom_ops; (void)glibc_ops; (void)glibc_mbs;
}

// ============================================================================
//  Benchmark 1: Single-threaded bulk alloc then bulk free
//
//  Allocates `count` blocks of `alloc_size`, stores every pointer,
//  then frees them all.  Measures both phases separately.
// ============================================================================

static void
st_bulk_work_custom(u64 alloc_size, s64 count, void **ptrs)
{
    for(s64 i = 0; i < count; i++) { ptrs[i] = alloc(alloc_size, TAG_STATIC); }
    for(s64 i = 0; i < count; i++) { free_alloc(ptrs[i]); }
}

static void
st_bulk_work_glibc(u64 alloc_size, s64 count, void **ptrs)
{
    for(s64 i = 0; i < count; i++) { ptrs[i] = malloc(alloc_size); }
    for(s64 i = 0; i < count; i++) { free(ptrs[i]); }
}

static void
bench_st_bulk_alloc_free(u64 alloc_size, s64 count, bench_section_e section)
{
    char title[256];
    char label[128];
    const char *size_label;
    if(alloc_size == 64)          size_label = "64 B";
    else if(alloc_size == 256)    size_label = "256 B";
    else if(alloc_size == KB(4))  size_label = "4 KB";
    else if(alloc_size == KB(64)) size_label = "64 KB";
    else if(alloc_size == MB(1))  size_label = "1 MB";
    else if(alloc_size == MB(16)) size_label = "16 MB";
    else if(alloc_size == MB(64)) size_label = "64 MB";
    else if(alloc_size == MB(256))size_label = "256 MB";
    else if(alloc_size == GB(1))  size_label = "1 GB";
    else                          size_label = "???";

    snprintf(label, sizeof(label), "Bulk %s x %lld", size_label, (long long)count);
    snprintf(title, sizeof(title), "ST Bulk Alloc+Free  (%s x %lld)", size_label, (long long)count);
    print_header(title);

    void **ptrs = (void**)malloc(count * sizeof(void*));

    // untimed warmup: identical workload for both allocators
    st_bulk_work_custom(alloc_size, count, ptrs);
    st_bulk_work_glibc(alloc_size, count, ptrs);

    // --- custom allocator ---
    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_bulk_work_custom(alloc_size, count, ptrs);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    // --- glibc malloc ---
    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_bulk_work_glibc(alloc_size, count, ptrs);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    free(ptrs);
    bench_record(section, label, best_custom, best_glibc, count, (u64)count * alloc_size);
    print_row_result(label, best_custom, best_glibc, count, (u64)count * alloc_size);
}

// ============================================================================
//  Benchmark 2: Single-threaded interleaved alloc/free
//
//  Each iteration allocates one block and immediately frees it.
//  This stresses the allocator's ability to recycle quickly.
// ============================================================================

static void
st_interleaved_work_custom(u64 alloc_size, s64 count)
{
    for(s64 i = 0; i < count; i++)
    {
        void *p = alloc(alloc_size, TAG_STATIC);
        free_alloc(p);
    }
}

static void
st_interleaved_work_glibc(u64 alloc_size, s64 count)
{
    for(s64 i = 0; i < count; i++)
    {
        void *p = malloc(alloc_size);
        free(p);
    }
}

static void
bench_st_interleaved(u64 alloc_size, s64 count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "Interleaved %llu B x %lld",
             (unsigned long long)alloc_size, (long long)count);
    snprintf(title, sizeof(title), "ST Interleaved Alloc/Free  (size=%llu B, count=%lld)",
             (unsigned long long)alloc_size, (long long)count);
    print_header(title);

    st_interleaved_work_custom(alloc_size, count);
    st_interleaved_work_glibc(alloc_size, count);

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_interleaved_work_custom(alloc_size, count);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_interleaved_work_glibc(alloc_size, count);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    bench_record(section, label, best_custom, best_glibc, count, (u64)count * alloc_size);
    print_row_result(label, best_custom, best_glibc, count, (u64)count * alloc_size);
}

// ============================================================================
//  Benchmark 3: Single-threaded random mixed sizes
//
//  Allocates blocks of random size, stores all pointers, then frees them
//  all.  Simulates a workload with heterogeneous allocation sizes.
// ============================================================================

static void
bench_st_random_mixed(u64 size_min, u64 size_max, s64 count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "Random %llu-%llu B x %lld",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)count);
    snprintf(title, sizeof(title), "ST Random Mixed Sizes  (%llu-%llu B, count=%lld)",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)count);
    print_header(title);

    void **ptrs = (void**)malloc(count * sizeof(void*));
    u64 *sizes  = (u64*)malloc(count * sizeof(u64));

    // generate the size sequence once, use it identically everywhere
    g_rng_state = 0xDEADBEEF;
    u64 bytes = 0;
    for(s64 i = 0; i < count; i++)
    {
        sizes[i] = bench_random_size_range(size_min, size_max);
        bytes   += sizes[i];
    }

    // untimed warmup: identical workload for both allocators
    {
        for(s64 i = 0; i < count; i++) { ptrs[i] = alloc(sizes[i], TAG_STATIC); }
        for(s64 i = 0; i < count; i++) { free_alloc(ptrs[i]); }
    }
    {
        for(s64 i = 0; i < count; i++) { ptrs[i] = malloc(sizes[i]); }
        for(s64 i = 0; i < count; i++) { free(ptrs[i]); }
    }

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++) { ptrs[i] = alloc(sizes[i], TAG_STATIC); }
        for(s64 i = 0; i < count; i++) { free_alloc(ptrs[i]); }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++) { ptrs[i] = malloc(sizes[i]); }
        for(s64 i = 0; i < count; i++) { free(ptrs[i]); }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    free(sizes);
    free(ptrs);
    bench_record(section, label, best_custom, best_glibc, count, bytes);
    print_row_result(label, best_custom, best_glibc, count, bytes);
}

// ============================================================================
//  Benchmark 4: Single-threaded interleaved RANDOM sizes
//
//  Each iteration allocates a block of a (deterministic) random size and
//  immediately frees it.  Stresses recycling across many sizes at once.
// ============================================================================

static void
bench_st_random_interleaved(u64 size_min, u64 size_max, s64 count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "Random interleaved %llu-%llu B x %lld",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)count);
    snprintf(title, sizeof(title), "ST Interleaved Random Sizes  (%llu-%llu B, count=%lld)",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)count);
    print_header(title);

    u64 *sizes = (u64*)malloc(count * sizeof(u64));
    g_rng_state = 0xFEEDFACE;
    u64 bytes = 0;
    for(s64 i = 0; i < count; i++)
    {
        sizes[i] = bench_random_size_range(size_min, size_max);
        bytes   += sizes[i];
    }

    // untimed warmup
    for(s64 i = 0; i < count; i++)
    {
        void *p = alloc(sizes[i], TAG_STATIC);
        free_alloc(p);
    }
    for(s64 i = 0; i < count; i++)
    {
        void *p = malloc(sizes[i]);
        free(p);
    }

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++)
        {
            void *p = alloc(sizes[i], TAG_STATIC);
            free_alloc(p);
        }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++)
        {
            void *p = malloc(sizes[i]);
            free(p);
        }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    free(sizes);
    bench_record(section, label, best_custom, best_glibc, count, bytes);
    print_row_result(label, best_custom, best_glibc, count, bytes);
}

// ============================================================================
//  Benchmark 5: Single-threaded alloc-then-free-all (arena-like)
//
//  Allocates `count` blocks without freeing any during the timed region,
//  then frees all at once afterward.  Measures pure allocation throughput
//  at a given peak live memory footprint.
// ============================================================================

static void
bench_st_alloc_only(u64 alloc_size, s64 count, bench_section_e section)
{
    char title[256];
    char label[128];
    const char *size_label;
    if(alloc_size == 64)          size_label = "64 B";
    else if(alloc_size == 256)    size_label = "256 B";
    else if(alloc_size == KB(4))  size_label = "4 KB";
    else if(alloc_size == KB(64)) size_label = "64 KB";
    else if(alloc_size == MB(1))  size_label = "1 MB";
    else if(alloc_size == MB(16)) size_label = "16 MB";
    else                          size_label = "???";

    snprintf(label, sizeof(label), "Alloc-only %s x %lld", size_label, (long long)count);
    snprintf(title, sizeof(title), "ST Alloc-only  (size=%s, count=%lld)",
             size_label, (long long)count);
    print_header(title);

    void **ptrs = (void**)malloc(count * sizeof(void*));

    // warmup
    for(s64 i = 0; i < count; i++) { ptrs[i] = alloc(alloc_size, TAG_STATIC); }
    for(s64 i = 0; i < count; i++) { free_alloc(ptrs[i]); }
    for(s64 i = 0; i < count; i++) { ptrs[i] = malloc(alloc_size); }
    for(s64 i = 0; i < count; i++) { free(ptrs[i]); }

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++) { ptrs[i] = alloc(alloc_size, TAG_STATIC); }
        u64 t1 = timer_now();
        for(s64 i = 0; i < count; i++) { free_alloc(ptrs[i]); }
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s64 i = 0; i < count; i++) { ptrs[i] = malloc(alloc_size); }
        u64 t1 = timer_now();
        for(s64 i = 0; i < count; i++) { free(ptrs[i]); }
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    free(ptrs);
    bench_record(section, label, best_custom, best_glibc, count, (u64)count * alloc_size);
    print_row_result(label, best_custom, best_glibc, count, (u64)count * alloc_size);
}

// ============================================================================
//  Benchmark 6: Free-after-every-N (bucketed allocation)
//
//  Allocates in groups of N, then frees the group. Tests how well each
//  allocator handles freeing recently-allocated memory in batches (common
//  in frame-based game engines).
// ============================================================================

static void
bench_st_bucketed_free(u64 alloc_size, s64 total_count, s64 bucket_size, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "Bucketed %llu B (bucket %lld) x %lld",
             (unsigned long long)alloc_size, (long long)bucket_size, (long long)total_count);
    snprintf(title, sizeof(title), "ST Bucketed Free  (size=%llu B, bucket=%lld, total=%lld)",
             (unsigned long long)alloc_size, (long long)bucket_size, (long long)total_count);
    print_header(title);

    void **ptrs = (void**)malloc(bucket_size * sizeof(void*));

    // warmup
    {
        s64 allocated = 0;
        while(allocated < total_count)
        {
            s64 this_batch = bucket_size;
            if(allocated + this_batch > total_count) this_batch = total_count - allocated;
            for(s64 i = 0; i < this_batch; i++) ptrs[i] = alloc(alloc_size, TAG_STATIC);
            for(s64 i = 0; i < this_batch; i++) free_alloc(ptrs[i]);
            allocated += this_batch;
        }
    }
    {
        s64 allocated = 0;
        while(allocated < total_count)
        {
            s64 this_batch = bucket_size;
            if(allocated + this_batch > total_count) this_batch = total_count - allocated;
            for(s64 i = 0; i < this_batch; i++) ptrs[i] = malloc(alloc_size);
            for(s64 i = 0; i < this_batch; i++) free(ptrs[i]);
            allocated += this_batch;
        }
    }

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        s64 allocated = 0;
        while(allocated < total_count)
        {
            s64 this_batch = bucket_size;
            if(allocated + this_batch > total_count) this_batch = total_count - allocated;
            for(s64 i = 0; i < this_batch; i++) ptrs[i] = alloc(alloc_size, TAG_STATIC);
            for(s64 i = 0; i < this_batch; i++) free_alloc(ptrs[i]);
            allocated += this_batch;
        }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        s64 allocated = 0;
        u64 t0 = timer_now();
        while(allocated < total_count)
        {
            s64 this_batch = bucket_size;
            if(allocated + this_batch > total_count) this_batch = total_count - allocated;
            for(s64 i = 0; i < this_batch; i++) ptrs[i] = malloc(alloc_size);
            for(s64 i = 0; i < this_batch; i++) free(ptrs[i]);
            allocated += this_batch;
        }
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    free(ptrs);
    bench_record(section, label, best_custom, best_glibc, total_count, (u64)total_count * alloc_size);
    print_row_result(label, best_custom, best_glibc, total_count, (u64)total_count * alloc_size);
}

// ============================================================================
//  Benchmark 7: Tagged bulk free
//
//  Tests the custom allocator's tagged free feature against manually
//  tracking glibc pointers. Allocates with mixed tags, then frees
//  one tag at a time.
// ============================================================================

static void
bench_st_tagged_free(bench_section_e section)
{
    const s32 objects_per_tag = 500;
    const s32 tag_count = TAG_COUNT - 1; // exclude TAG_CLEAR
    const s32 total = objects_per_tag * tag_count;
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "Tagged bulk free (%d x %d tags)", objects_per_tag, tag_count);
    snprintf(title, sizeof(title), "ST Tagged Bulk Free  (%d x %d tags, free by tag)",
             objects_per_tag, tag_count);
    print_header(title);

    // warmup
    {
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            for(s32 i = 0; i < objects_per_tag; i++) alloc(64, (s32)tag);
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            free_tagged_allocations((s32)tag);
    }

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            for(s32 i = 0; i < objects_per_tag; i++) alloc(64, (s32)tag);
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            free_tagged_allocations((s32)tag);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        void **ptrs = (void**)malloc(total * sizeof(void*));
        s32 idx = 0;
        u64 t0 = timer_now();
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            for(s32 i = 0; i < objects_per_tag; i++) ptrs[idx++] = malloc(64);
        idx = 0;
        for(s32 tag = 1; tag < TAG_COUNT; tag++)
            for(s32 i = 0; i < objects_per_tag; i++) free(ptrs[idx++]);
        u64 t1 = timer_now();
        free(ptrs);
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    bench_record(section, label, best_custom, best_glibc, total, (u64)total * 64);
    print_row_result(label, best_custom, best_glibc, total, (u64)total * 64);
}

// ============================================================================
//  Benchmark 8: Multi-Threaded -- SINGLE PERSISTENT THREAD POOL
//
//  One pool of threads is created once and reused by every MT benchmark,
//  so the custom allocator only ever registers (1 main + N pool) thread
//  contexts, well below MAX_THREAD_COUNT (42). A generic job dispatch
//  lets every MT test share the exact same synchronization + timing code.
// ============================================================================

typedef void (*mt_work_fn_t)(void *data);

typedef struct {
    mt_work_fn_t work;
    void        *data;
} mt_role_t;

typedef struct {
    bool8 use_custom;
} mt_job_common_t;

static s32            mt_thread_count = 0;
static sys_thread_t   mt_handles[MAX_THREAD_COUNT];
static mt_role_t      mt_roles[MAX_THREAD_COUNT];
static sys_semaphore_t mt_start_sem;
static sys_semaphore_t mt_done_sem;
static volatile bool8  mt_pool_stop;

static PLATFORM_THREAD_PROC(mt_pool_thread_proc)
{
    s32 t = (s32)(uintptr_t)(void*)user_data;
    mt_role_t *role = &mt_roles[t];

    for(;;)
    {
        sys_semaphore_wait(&mt_start_sem, 0);
        if(mt_pool_stop) break;
        role->work(role->data);
        sys_semaphore_release(&mt_done_sem, 1);
    }
    return(0);
}

static void
mt_pool_init(s32 thread_count)
{
    mt_thread_count = thread_count;
    mt_pool_stop = false;
    mt_start_sem = sys_semaphore_create(0, thread_count);
    mt_done_sem  = sys_semaphore_create(0, thread_count);
    for(s32 t = 0; t < thread_count; t++)
    {
        mt_roles[t].work = null;
        mt_roles[t].data = null;
        mt_handles[t] = sys_thread_create(mt_pool_thread_proc, (void*)(uintptr_t)t, false);
    }
    SDL_Delay(10);
}

static void
mt_pool_shutdown(void)
{
    mt_pool_stop = true;
    sys_semaphore_release(&mt_start_sem, mt_thread_count);
    for(s32 t = 0; t < mt_thread_count; t++)
        sys_thread_wait(&mt_handles[t]);
    sys_semaphore_close(&mt_start_sem);
    sys_semaphore_close(&mt_done_sem);
    mt_thread_count = 0;
}

static void
mt_warmup_round(bool8 use_custom, void **datas)
{
    for(s32 t = 0; t < mt_thread_count; t++)
        ((mt_job_common_t*)datas[t])->use_custom = use_custom;
    sys_semaphore_release(&mt_start_sem, mt_thread_count);
    for(s32 t = 0; t < mt_thread_count; t++)
        sys_semaphore_wait(&mt_done_sem, 0);
}

static double
mt_run_best_of(bool8 use_custom, void **datas)
{
    for(s32 t = 0; t < mt_thread_count; t++)
        ((mt_job_common_t*)datas[t])->use_custom = use_custom;

    double best_ms = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        sys_semaphore_release(&mt_start_sem, mt_thread_count);
        for(s32 t = 0; t < mt_thread_count; t++)
            sys_semaphore_wait(&mt_done_sem, 0);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_ms) best_ms = ms;
    }
    return(best_ms);
}

static void
mt_run_bench(const char *label, bench_section_e section,
             mt_work_fn_t work, void **datas, s64 ops_total, u64 bytes_total)
{
    for(s32 t = 0; t < mt_thread_count; t++)
    {
        mt_roles[t].work = work;
        mt_roles[t].data = datas[t];
    }

    // untimed warmup: identical round for both allocators (registers MT
    // threads with the custom allocator, warms pages + glibc arenas)
    mt_warmup_round(true,  datas);
    mt_warmup_round(false, datas);

    double custom_ms = mt_run_best_of(true,  datas);
    double glibc_ms  = mt_run_best_of(false, datas);

    bench_record(section, label, custom_ms, glibc_ms, ops_total, bytes_total);
    print_row_result(label, custom_ms, glibc_ms, ops_total, bytes_total);
}

// --- MT job: bulk (alloc all, then free all) ---

typedef struct {
    mt_job_common_t common;
    u64             alloc_size;
    s64             alloc_count;
    void          **ptrs;
} mt_bulk_job_t;

static void
mt_bulk_job_fn(void *data)
{
    mt_bulk_job_t *j = (mt_bulk_job_t*)data;
    if(j->common.use_custom)
    {
        for(s64 i = 0; i < j->alloc_count; i++) j->ptrs[i] = alloc(j->alloc_size, TAG_STATIC);
        for(s64 i = 0; i < j->alloc_count; i++) free_alloc(j->ptrs[i]);
    }
    else
    {
        for(s64 i = 0; i < j->alloc_count; i++) j->ptrs[i] = malloc(j->alloc_size);
        for(s64 i = 0; i < j->alloc_count; i++) free(j->ptrs[i]);
    }
}

static void
bench_mt_bulk(u64 alloc_size, s64 per_thread_count, bench_section_e section)
{
    char title[256];
    char label[128];
    const char *size_label;
    if(alloc_size == 64)          size_label = "64 B";
    else if(alloc_size == 256)    size_label = "256 B";
    else if(alloc_size == KB(4))  size_label = "4 KB";
    else if(alloc_size == KB(64)) size_label = "64 KB";
    else if(alloc_size == MB(1))  size_label = "1 MB";
    else if(alloc_size == MB(16)) size_label = "16 MB";
    else if(alloc_size == MB(64)) size_label = "64 MB";
    else if(alloc_size == MB(256))size_label = "256 MB";
    else                          size_label = "???";

    snprintf(label, sizeof(label), "MT Bulk %s x %lld/thr", size_label, (long long)per_thread_count);
    snprintf(title, sizeof(title), "MT Bulk Alloc+Free  (%s, %d threads x %lld)",
             size_label, mt_thread_count, (long long)per_thread_count);
    print_header(title);

    mt_bulk_job_t *jobs = (mt_bulk_job_t*)calloc(mt_thread_count, sizeof(mt_bulk_job_t));
    void **datas = (void**)calloc(mt_thread_count, sizeof(void*));

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        jobs[t].alloc_size  = alloc_size;
        jobs[t].alloc_count = per_thread_count;
        jobs[t].ptrs = (void**)malloc(per_thread_count * sizeof(void*));
        datas[t] = &jobs[t];
    }

    mt_run_bench(label, section, mt_bulk_job_fn, datas,
                 (s64)mt_thread_count * per_thread_count,
                 (u64)mt_thread_count * (u64)per_thread_count * alloc_size);

    for(s32 t = 0; t < mt_thread_count; t++) free(jobs[t].ptrs);
    free(datas);
    free(jobs);
}

// --- MT job: interleaved (alloc + free each op) ---

typedef struct {
    mt_job_common_t common;
    u64             alloc_size;
    s64             alloc_count;
} mt_interleaved_job_t;

static void
mt_interleaved_job_fn(void *data)
{
    mt_interleaved_job_t *j = (mt_interleaved_job_t*)data;
    if(j->common.use_custom)
    {
        for(s64 i = 0; i < j->alloc_count; i++)
        {
            void *p = alloc(j->alloc_size, TAG_STATIC);
            free_alloc(p);
        }
    }
    else
    {
        for(s64 i = 0; i < j->alloc_count; i++)
        {
            void *p = malloc(j->alloc_size);
            free(p);
        }
    }
}

static void
bench_mt_interleaved(u64 alloc_size, s64 per_thread_count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "MT Interleaved %llu B x %lld/thr",
             (unsigned long long)alloc_size, (long long)per_thread_count);
    snprintf(title, sizeof(title), "MT Interleaved Alloc/Free  (%llu B, %d threads x %lld)",
             (unsigned long long)alloc_size, mt_thread_count, (long long)per_thread_count);
    print_header(title);

    mt_interleaved_job_t *jobs = (mt_interleaved_job_t*)calloc(mt_thread_count, sizeof(mt_interleaved_job_t));
    void **datas = (void**)calloc(mt_thread_count, sizeof(void*));

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        jobs[t].alloc_size  = alloc_size;
        jobs[t].alloc_count = per_thread_count;
        datas[t] = &jobs[t];
    }

    mt_run_bench(label, section, mt_interleaved_job_fn, datas,
                 (s64)mt_thread_count * per_thread_count,
                 (u64)mt_thread_count * (u64)per_thread_count * alloc_size);

    free(datas);
    free(jobs);
}

// --- MT job: random mixed sizes (alloc all, then free all) ---

typedef struct {
    mt_job_common_t common;
    s64             alloc_count;
    void          **ptrs;
    u64            *sizes;
} mt_random_job_t;

static void
mt_random_job_fn(void *data)
{
    mt_random_job_t *j = (mt_random_job_t*)data;
    if(j->common.use_custom)
    {
        for(s64 i = 0; i < j->alloc_count; i++) j->ptrs[i] = alloc(j->sizes[i], TAG_STATIC);
        for(s64 i = 0; i < j->alloc_count; i++) free_alloc(j->ptrs[i]);
    }
    else
    {
        for(s64 i = 0; i < j->alloc_count; i++) j->ptrs[i] = malloc(j->sizes[i]);
        for(s64 i = 0; i < j->alloc_count; i++) free(j->ptrs[i]);
    }
}

static void
bench_mt_random(u64 size_min, u64 size_max, s64 per_thread_count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "MT Random %llu-%llu B x %lld/thr",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)per_thread_count);
    snprintf(title, sizeof(title), "MT Random Mixed Sizes  (%llu-%llu B, %d threads x %lld)",
             (unsigned long long)size_min, (unsigned long long)size_max,
             mt_thread_count, (long long)per_thread_count);
    print_header(title);

    mt_random_job_t *jobs = (mt_random_job_t*)calloc(mt_thread_count, sizeof(mt_random_job_t));
    void **datas = (void**)calloc(mt_thread_count, sizeof(void*));
    u64 bytes_total = 0;

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        jobs[t].alloc_count = per_thread_count;
        jobs[t].ptrs  = (void**)malloc(per_thread_count * sizeof(void*));
        jobs[t].sizes = (u64*)malloc(per_thread_count * sizeof(u64));

        // each thread gets different random sizes (deterministic)
        u32 seed = 12345 + (u32)t * 7919;
        for(s64 i = 0; i < per_thread_count; i++)
        {
            seed = seed * 1103515245 + 12345;
            u32 raw = (seed >> 16) & 0x7FFF;
            u64 span = (size_max > size_min) ? (size_max - size_min) : 1;
            jobs[t].sizes[i] = size_min + (raw % span);
            bytes_total += jobs[t].sizes[i];
        }
        datas[t] = &jobs[t];
    }

    mt_run_bench(label, section, mt_random_job_fn, datas,
                 (s64)mt_thread_count * per_thread_count, bytes_total);

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        free(jobs[t].ptrs);
        free(jobs[t].sizes);
    }
    free(datas);
    free(jobs);
}

// --- MT job: interleaved RANDOM sizes (alloc + free each op) ---

typedef struct {
    mt_job_common_t common;
    s64             alloc_count;
    u64            *sizes;
} mt_random_interleaved_job_t;

static void
mt_random_interleaved_job_fn(void *data)
{
    mt_random_interleaved_job_t *j = (mt_random_interleaved_job_t*)data;
    if(j->common.use_custom)
    {
        for(s64 i = 0; i < j->alloc_count; i++)
        {
            void *p = alloc(j->sizes[i], TAG_STATIC);
            free_alloc(p);
        }
    }
    else
    {
        for(s64 i = 0; i < j->alloc_count; i++)
        {
            void *p = malloc(j->sizes[i]);
            free(p);
        }
    }
}

static void
bench_mt_random_interleaved(u64 size_min, u64 size_max, s64 per_thread_count, bench_section_e section)
{
    char title[256];
    char label[128];
    snprintf(label, sizeof(label), "MT Random interleaved %llu-%llu B x %lld/thr",
             (unsigned long long)size_min, (unsigned long long)size_max, (long long)per_thread_count);
    snprintf(title, sizeof(title), "MT Interleaved Random Sizes  (%llu-%llu B, %d threads x %lld)",
             (unsigned long long)size_min, (unsigned long long)size_max,
             mt_thread_count, (long long)per_thread_count);
    print_header(title);

    mt_random_interleaved_job_t *jobs =
        (mt_random_interleaved_job_t*)calloc(mt_thread_count, sizeof(mt_random_interleaved_job_t));
    void **datas = (void**)calloc(mt_thread_count, sizeof(void*));
    u64 bytes_total = 0;

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        jobs[t].alloc_count = per_thread_count;
        jobs[t].sizes = (u64*)malloc(per_thread_count * sizeof(u64));

        u32 seed = 0xFEED + (u32)t * 7919;
        for(s64 i = 0; i < per_thread_count; i++)
        {
            seed = seed * 1103515245 + 12345;
            u32 raw = (seed >> 16) & 0x7FFF;
            u64 span = (size_max > size_min) ? (size_max - size_min) : 1;
            jobs[t].sizes[i] = size_min + (raw % span);
            bytes_total += jobs[t].sizes[i];
        }
        datas[t] = &jobs[t];
    }

    mt_run_bench(label, section, mt_random_interleaved_job_fn, datas,
                 (s64)mt_thread_count * per_thread_count, bytes_total);

    for(s32 t = 0; t < mt_thread_count; t++) free(jobs[t].sizes);
    free(datas);
    free(jobs);
}

// ============================================================================
//  Benchmark 9: Realistic game-frame workload (single-threaded)
//
//  Simulates one "frame" of a game engine:
//    1. Allocate N entity structs (medium)
//    2. Allocate M component structs (small)
//    3. Free a fraction of entities
//    4. Allocate more entities
//    5. Allocate a batch of short-lived strings (small)
//    6. Free all strings
//    7. Free all remaining entities + components
// ============================================================================

static s64
st_realistic_work_custom(s32 frame_iterations)
{
    const s32 num_entities = 200;
    const s32 num_components = 500;
    const s32 num_free_some = 80;
    const s32 num_more_entities = 100;
    const s32 num_strings = 1000;

    for(s32 it = 0; it < frame_iterations; it++)
    {
        void *ent_buf[num_entities];
        for(s32 i = 0; i < num_entities; i++) ent_buf[i] = alloc(128, TAG_STATIC);

        void *comp_buf[num_components];
        for(s32 i = 0; i < num_components; i++) comp_buf[i] = alloc(48, TAG_STATIC);

        for(s32 i = 0; i < num_free_some; i++) free_alloc(ent_buf[i]);

        void *more_buf[num_more_entities];
        for(s32 i = 0; i < num_more_entities; i++) more_buf[i] = alloc(128, TAG_STATIC);

        void *str_buf[num_strings];
        for(s32 i = 0; i < num_strings; i++) str_buf[i] = alloc(32 + (i % 96), TAG_TEMP);

        for(s32 i = 0; i < num_strings; i++) free_alloc(str_buf[i]);

        for(s32 i = num_free_some; i < num_entities; i++) free_alloc(ent_buf[i]);
        for(s32 i = 0; i < num_components; i++) free_alloc(comp_buf[i]);
        for(s32 i = 0; i < num_more_entities; i++) free_alloc(more_buf[i]);
    }
    return(frame_iterations);
}

static s64
st_realistic_work_glibc(s32 frame_iterations)
{
    const s32 num_entities = 200;
    const s32 num_components = 500;
    const s32 num_free_some = 80;
    const s32 num_more_entities = 100;
    const s32 num_strings = 1000;

    for(s32 it = 0; it < frame_iterations; it++)
    {
        void *ent_buf[num_entities];
        for(s32 i = 0; i < num_entities; i++) ent_buf[i] = malloc(128);

        void *comp_buf[num_components];
        for(s32 i = 0; i < num_components; i++) comp_buf[i] = malloc(48);

        for(s32 i = 0; i < num_free_some; i++) free(ent_buf[i]);

        void *more_buf[num_more_entities];
        for(s32 i = 0; i < num_more_entities; i++) more_buf[i] = malloc(128);

        void *str_buf[num_strings];
        for(s32 i = 0; i < num_strings; i++) str_buf[i] = malloc(32 + (i % 96));

        for(s32 i = 0; i < num_strings; i++) free(str_buf[i]);

        for(s32 i = num_free_some; i < num_entities; i++) free(ent_buf[i]);
        for(s32 i = 0; i < num_components; i++) free(comp_buf[i]);
        for(s32 i = 0; i < num_more_entities; i++) free(more_buf[i]);
    }
    return(frame_iterations);
}

static void
bench_realistic_frame(bench_section_e section)
{
    const s32 frame_iterations = 500;
    const s64 ops_per_frame = 200 + 500 + 100 + 1000;
    const u64 bytes_per_frame = (200 * 128) + (500 * 48) + (100 * 128) + (1000 * (32 + 128) / 2);
    char label[128];
    snprintf(label, sizeof(label), "Game frame x%d (realistic mix)", frame_iterations);
    print_header("Realistic Game-Frame Workload (single-threaded)");

    st_realistic_work_custom(frame_iterations);
    st_realistic_work_glibc(frame_iterations);

    double best_custom = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_realistic_work_custom(frame_iterations);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_custom) best_custom = ms;
    }

    double best_glibc = 1e18;
    for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
    {
        u64 t0 = timer_now();
        st_realistic_work_glibc(frame_iterations);
        u64 t1 = timer_now();
        double ms = timer_elapsed_ms(t0, t1);
        if(ms < best_glibc) best_glibc = ms;
    }

    bench_record(section, label, best_custom, best_glibc,
                 frame_iterations * ops_per_frame, (u64)frame_iterations * bytes_per_frame);
    print_row_result(label, best_custom, best_glibc,
                     frame_iterations * ops_per_frame, (u64)frame_iterations * bytes_per_frame);
}

// --- MT job: realistic game-frame workload per thread ---

typedef struct {
    mt_job_common_t common;
    s32             frame_iterations;
} mt_realistic_job_t;

static void
mt_realistic_job_fn(void *data)
{
    mt_realistic_job_t *j = (mt_realistic_job_t*)data;
    if(j->common.use_custom)
    {
        st_realistic_work_custom(j->frame_iterations);
    }
    else
    {
        st_realistic_work_glibc(j->frame_iterations);
    }
}

static void
bench_mt_realistic(bench_section_e section)
{
    const s32 frame_iterations = 200;
    const s64 ops_per_frame = 200 + 500 + 100 + 1000;
    const u64 bytes_per_frame = (200 * 128) + (500 * 48) + (100 * 128) + (1000 * (32 + 128) / 2);
    char label[128];
    snprintf(label, sizeof(label), "MT Game frame x%d/thr", frame_iterations);
    print_header("Realistic Game-Frame Workload (multi-threaded)");

    mt_realistic_job_t *jobs =
        (mt_realistic_job_t*)calloc(mt_thread_count, sizeof(mt_realistic_job_t));
    void **datas = (void**)calloc(mt_thread_count, sizeof(void*));

    for(s32 t = 0; t < mt_thread_count; t++)
    {
        jobs[t].frame_iterations = frame_iterations;
        datas[t] = &jobs[t];
    }

    mt_run_bench(label, section, mt_realistic_job_fn, datas,
                 (s64)mt_thread_count * (s64)frame_iterations * ops_per_frame,
                 (u64)mt_thread_count * (u64)frame_iterations * bytes_per_frame);

    free(datas);
    free(jobs);
}

// ============================================================================
//  Benchmark 10: Single-threaded throughput sweep
//
//  Allocates progressively larger blocks (16B -> 1MB) to show where
//  each allocator is strongest.
// ============================================================================

static void
bench_st_throughput_sweep(bench_section_e section)
{
    print_header("ST Throughput Sweep (alloc + free, 500 iterations)");

    struct { u64 size; const char *label; } sizes[] = {
        {  16,       "16 B"   },
        {  64,       "64 B"   },
        {  256,     "256 B"   },
        {  1024,      "1 KB"  },
        {  4096,      "4 KB"  },
        {  16384,    "16 KB"  },
        {  65536,    "64 KB"  },
        {  262144,  "256 KB"  },
        {  1048576,    "1 MB" },
    };
    s32 size_count = (s32)(sizeof(sizes) / sizeof(sizes[0]));
    s64 count = 500;

    for(s32 s = 0; s < size_count; s++)
    {
        u64 alloc_size = sizes[s].size;
        char label[128];
        snprintf(label, sizeof(label), "Sweep %s x 500", sizes[s].label);

        void **ptrs = (void**)malloc(count * sizeof(void*));

        st_bulk_work_custom(alloc_size, count, ptrs);
        st_bulk_work_glibc(alloc_size, count, ptrs);

        double custom_ms = 1e18;
        for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
        {
            u64 t0 = timer_now();
            st_bulk_work_custom(alloc_size, count, ptrs);
            u64 t1 = timer_now();
            double ms = timer_elapsed_ms(t0, t1);
            if(ms < custom_ms) custom_ms = ms;
        }

        double glibc_ms = 1e18;
        for(s32 iter = 0; iter < BENCH_ITERATIONS; iter++)
        {
            u64 t0 = timer_now();
            st_bulk_work_glibc(alloc_size, count, ptrs);
            u64 t1 = timer_now();
            double ms = timer_elapsed_ms(t0, t1);
            if(ms < glibc_ms) glibc_ms = ms;
        }

        free(ptrs);
        bench_record(section, label, custom_ms, glibc_ms, count, (u64)count * alloc_size);

        double ratio = glibc_ms / custom_ms;
        printf("  %-12s  %10.2f ms  %10.2f ms  %7.2fx\n",
               sizes[s].label, custom_ms, glibc_ms, ratio);
    }
}

static void
print_pool_usage(void)
{
    // new_malloc.cpp is included above, so `allocator` is visible here.
    u64 reserved = allocator.next_page_offset;
    printf("  Custom allocator pool usage so far: %llu MB / %llu MB (%.1f%%)\n",
           (unsigned long long)(reserved / MB(1)),
           (unsigned long long)(allocator.max_capacity / MB(1)),
           100.0 * (double)reserved / (double)allocator.max_capacity);
}

// ============================================================================
//  Section summaries
// ============================================================================

static void
print_section_summary(bench_section_e section)
{
    printf("\n================================================================\n");
    printf("  SUMMARY: %s\n", bench_section_names[section]);
    printf("================================================================\n");
    printf("  %-52s %10s %10s %12s %12s %14s %14s %7s  %s\n",
           "Test", "Custom ms", "Glibc ms", "Custom ops/s", "Glibc ops/s", "Custom MB/s", "Glibc MB/s", "Ratio", "Faster");
    printf("  %-52s %10s %10s %12s %12s %14s %14s %7s  %s\n",
           "----------------------------------------------------", "----------", "----------",
           "------------", "------------", "--------------", "--------------", "-------", "------");

    double custom_total = 0.0, glibc_total = 0.0;
    u64 bytes_total = 0;
    s64 ops_total = 0;
    s32 custom_wins = 0, glibc_wins = 0;

    for(s32 i = 0; i < g_record_count; i++)
    {
        bench_record_t *rec = &g_records[i];
        if(rec->section != section) continue;

        double custom_ops = (double)rec->ops / (rec->custom_ms / 1000.0);
        double glibc_ops  = (double)rec->ops / (rec->glibc_ms / 1000.0);
        double custom_mbs = (double)rec->bytes / (rec->custom_ms / 1000.0) / 1000000.0;
        double glibc_mbs  = (double)rec->bytes / (rec->glibc_ms / 1000.0) / 1000000.0;
        double ratio      = rec->glibc_ms / rec->custom_ms;
        const char *faster = (rec->custom_ms <= rec->glibc_ms) ? "custom" : "glibc";

        if(rec->custom_ms < rec->glibc_ms) custom_wins++;
        else if(rec->glibc_ms < rec->custom_ms) glibc_wins++;

        custom_total += rec->custom_ms;
        glibc_total  += rec->glibc_ms;
        bytes_total  += rec->bytes;
        ops_total    += rec->ops;

        printf("  %-52s %10.2f %10.2f %12.1f %12.1f %14.1f %14.1f %7.2f  %s\n",
               rec->label, rec->custom_ms, rec->glibc_ms,
               custom_ops / 1000000.0, glibc_ops / 1000000.0, custom_mbs, glibc_mbs, ratio, faster);
    }

    double total_ratio = glibc_total / custom_total;
    printf("  %-52s %10.2f %10.2f\n", "TOTAL", custom_total, glibc_total);
    printf("  Total ops: %lld   Total bytes: %llu (%.2f GB)\n",
           (long long)ops_total, (unsigned long long)bytes_total,
           (double)bytes_total / GB(1));
    printf("  Section ratio (glibc/custom): %.2fx  |  custom won %d / %d tests\n",
           total_ratio, custom_wins, custom_wins + glibc_wins);
}

static void
print_all_summaries(void)
{
    double custom_grand = 0.0, glibc_grand = 0.0;
    u64 bytes_grand = 0;
    s64 ops_grand = 0;
    s32 custom_wins = 0, glibc_wins = 0;

    for(bench_section_e sec = (bench_section_e)0; sec < BENCH_SEC_COUNT; sec = (bench_section_e)(sec + 1))
        print_section_summary(sec);

    // grand totals across everything
    for(s32 i = 0; i < g_record_count; i++)
    {
        bench_record_t *rec = &g_records[i];
        custom_grand += rec->custom_ms;
        glibc_grand  += rec->glibc_ms;
        bytes_grand  += rec->bytes;
        ops_grand    += rec->ops;
        if(rec->custom_ms < rec->glibc_ms) custom_wins++;
        else if(rec->glibc_ms < rec->custom_ms) glibc_wins++;
    }

    printf("\n================================================================\n");
    printf("  GRAND TOTAL (all %d tests)\n", g_record_count);
    printf("================================================================\n");
    printf("  Custom allocator: %12.2f ms total\n", custom_grand);
    printf("  Glibc malloc    : %12.2f ms total\n", glibc_grand);
    printf("  Overall ratio   : %6.2fx (glibc/custom; >1 means custom faster)\n",
           glibc_grand / custom_grand);
    printf("  Total ops       : %lld\n", (long long)ops_grand);
    printf("  Total bytes     : %llu (%.2f GB)\n", (unsigned long long)bytes_grand,
           (double)bytes_grand / GB(1));
    printf("  Custom wins     : %d / %d tests\n", custom_wins, custom_wins + glibc_wins);
}

// ============================================================================
//  main
// ============================================================================

int main(void)
{
    SDL_Init(0);

    // --- Initialize the custom allocator ---
    void *base_address = (void*)TB(2);
    u64   capacity     = GB(4);
    memory_allocator_init(base_address, capacity);

    s32 cpu_threads = sys_get_thread_count();
    s32 mt_threads  = cpu_threads;
    if(mt_threads > (MAX_THREAD_COUNT - 1)) mt_threads = MAX_THREAD_COUNT - 1; // leave room for main thread

    printf("\n");
    printf("================================================================\n");
    printf("  Allocator Benchmark: Custom Allocator vs Glibc malloc\n");
    printf("================================================================\n");
    printf("  CPU logical cores : %d\n", cpu_threads);
    printf("  MT pool threads   : %d (persistent, reused by all MT tests)\n", mt_threads);
    printf("  Alloc DEBUG mode  : %s\n", ALLOC_BENCH_DEBUG ? "ON (guard pages)" : "OFF (release)");
    printf("  Timing iterations : %d (best-of, after identical warmup round)\n", BENCH_ITERATIONS);
    printf("  Allocator pool    : %llu MB at %p\n", (unsigned long long)(capacity / MB(1)), base_address);
    printf("\n");

    // ====================================================================
    //  Single-threaded benchmarks (small/medium)
    // ====================================================================
    printf(">>> SINGLE-THREADED BENCHMARKS (small/medium) <<<\n");

    bench_st_bulk_alloc_free(64,         BENCH_COUNT(500), BENCH_SEC_ST);
    bench_st_bulk_alloc_free(KB(4),      BENCH_COUNT(500), BENCH_SEC_ST);
    bench_st_bulk_alloc_free(KB(64),     BENCH_COUNT(200), BENCH_SEC_ST);
    bench_st_bulk_alloc_free(MB(1),      BENCH_COUNT(100), BENCH_SEC_ST);

    bench_st_interleaved(64,             BENCH_COUNT(5000), BENCH_SEC_ST);
    bench_st_interleaved(KB(4),          BENCH_COUNT(2000), BENCH_SEC_ST);
    bench_st_interleaved(KB(64),         BENCH_COUNT(500), BENCH_SEC_ST);

    bench_st_random_mixed(64,            KB(4), BENCH_COUNT(500), BENCH_SEC_ST);
    bench_st_random_interleaved(64,     KB(64), BENCH_COUNT(10000), BENCH_SEC_ST);

    bench_st_alloc_only(64,              BENCH_COUNT(500), BENCH_SEC_ST);
    bench_st_alloc_only(KB(4),           BENCH_COUNT(500), BENCH_SEC_ST);
    bench_st_alloc_only(KB(64),          BENCH_COUNT(200), BENCH_SEC_ST);

    bench_st_bucketed_free(64,           BENCH_COUNT(20000), 100, BENCH_SEC_ST);

    bench_st_tagged_free(BENCH_SEC_ST);

    bench_st_throughput_sweep(BENCH_SEC_ST);

    print_pool_usage();

    // ====================================================================
    //  Single-threaded LARGE allocation benchmarks (MB-GB sizes)
    //  NOTE: peak live memory per test stays well under the 4 GB pool.
    // ====================================================================
    printf("\n>>> SINGLE-THREADED LARGE BENCHMARKS (MB-GB) <<<\n");

    // bulk: alloc all (peak live = count * size), then free all.
    // ORDERED LARGEST-FIRST so freed sections from a big test are reused by
    // the following (smaller) tests instead of carving fresh pool pages.
    // The allocator never returns pages to the OS (per-thread page lists
    // persist), so the pool reservation is cumulative: budgeted well under
    // the 4 GB shared pool. See print_pool_usage() after each section.
    // Interleaved first so each distinct size reserves ~one fresh page
    // (then is reused), giving the biggest live blocks for reuse afterward.
    // Peak reservation for this whole section ~1.4 GB.
    bench_st_interleaved(MB(16),           500, BENCH_SEC_ST_LARGE);
    bench_st_interleaved(MB(64),           200, BENCH_SEC_ST_LARGE);
    bench_st_interleaved(MB(256),          100, BENCH_SEC_ST_LARGE);
    bench_st_interleaved(GB(1),              5, BENCH_SEC_ST_LARGE);

    bench_st_bulk_alloc_free(MB(256),       4, BENCH_SEC_ST_LARGE); // 1.0 GB live peak
    bench_st_bulk_alloc_free(MB(64),       16, BENCH_SEC_ST_LARGE); // reuses 256 MB sections
    bench_st_bulk_alloc_free(MB(16),       64, BENCH_SEC_ST_LARGE); // reuses

    // random interleaved across 1 MB - 64 MB (up to ~0.1 GB reserve)
    bench_st_random_interleaved(MB(1),   MB(64), 20, BENCH_SEC_ST_LARGE);

    print_pool_usage();

    // ====================================================================
    //  Multi-threaded benchmarks (single persistent thread pool)
    // ====================================================================
    mt_pool_init(mt_threads);
    printf("\n>>> MULTI-THREADED BENCHMARKS (%d threads) <<<\n", mt_threads);

    bench_mt_bulk(64,           BENCH_COUNT(500), BENCH_SEC_MT);
    bench_mt_bulk(KB(4),        BENCH_COUNT(500), BENCH_SEC_MT);
    bench_mt_bulk(KB(64),       BENCH_COUNT(200), BENCH_SEC_MT);

    bench_mt_interleaved(64,    BENCH_COUNT(5000), BENCH_SEC_MT);
    bench_mt_interleaved(KB(4), BENCH_COUNT(2000), BENCH_SEC_MT);
    bench_mt_interleaved(KB(64), BENCH_COUNT(500), BENCH_SEC_MT);

    bench_mt_random(64,         KB(4),  BENCH_COUNT(500), BENCH_SEC_MT);
    bench_mt_random_interleaved(64, KB(64), BENCH_COUNT(2000), BENCH_SEC_MT);

    bench_mt_realistic(BENCH_SEC_MT);

    // ====================================================================
    //  Multi-threaded LARGE allocation benchmarks (MB-GB sizes)
    //  NOTE: the pool reservation is cumulative per thread (pages are never
    //  returned to the OS), so per-thread sizes/counts are budgeted so the
    //  total reservation across all 8 pool threads + main stays under the
    //  4 GB shared pool. Largest-first for reuse.
    // ====================================================================
    printf("\n>>> MULTI-THREADED LARGE BENCHMARKS (MB-GB) <<<\n");

    bench_mt_bulk(MB(64),         2, BENCH_SEC_MT_LARGE); // 8 * 2 * 64 MB = 1.0 GB live
    bench_mt_bulk(MB(16),         8, BENCH_SEC_MT_LARGE); // 8 * 8 * 16 MB = 1.0 GB
    bench_mt_bulk(MB(1),         32, BENCH_SEC_MT_LARGE); // 8 * 32 * 1 MB = 256 MB

    bench_mt_interleaved(MB(16), 200, BENCH_SEC_MT_LARGE);
    bench_mt_interleaved(MB(64),  50, BENCH_SEC_MT_LARGE);

    bench_mt_random_interleaved(MB(1), MB(64), 100, BENCH_SEC_MT_LARGE);

    mt_pool_shutdown();

    print_pool_usage();

    // ====================================================================
    //  Realistic workload
    // ====================================================================
    printf("\n>>> REALISTIC WORKLOAD <<<\n");

    bench_realistic_frame(BENCH_SEC_REALISTIC);

    // ====================================================================
    //  Detailed per-section overview
    // ====================================================================
    printf("\n================================================================\n");
    printf("  DETAILED OVERVIEW\n");
    printf("================================================================\n");

    print_all_summaries();

    printf("\n");
    SDL_Quit();
    return(0);
}
