/* ========================================================================
   $File: test_math.cpp $
   $Date: February 15 2026 $
   $Creator: Justin Lewis $
   Verifies correctness and benchmarks SSE/AVX2 math functions in c_math.h.
   Build:  make (from code/)
   Usage:  ../build/test_test_math          -- correctness + benchmark
           ../build/test_test_math bench    -- benchmark only
   Rebuild without AVX2 to compare:
       clang++ -g -O2 -std=c++11 -msse4.1 -I. tests/test_math.cpp -o ../build/test_math_sse -lm
   ======================================================================== */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <c_types.h>
#include <c_base.h>

#define MATH_IMPLEMENTATION
#include <c_math.h>

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

static bool8
f32_near(float32 a, float32 b, float32 epsilon = 0.001f)
{
    float32 diff = a - b;
    if(diff < 0) diff = -diff;
    return(diff < epsilon);
}

static bool8
vec4_near(vec4_t a, vec4_t b, float32 epsilon = 0.001f)
{
    return(f32_near(a.x, b.x, epsilon) &&
           f32_near(a.y, b.y, epsilon) &&
           f32_near(a.z, b.z, epsilon) &&
           f32_near(a.w, b.w, epsilon));
}

static bool8
mat4_near(mat4_t a, mat4_t b, float32 epsilon = 0.001f)
{
    for(int i = 0; i < 16; ++i)
    {
        if(!f32_near(a.values[i], b.values[i], epsilon))
        {
            return(false);
        }
    }
    return(true);
}

static void
print_mat4(const char *label, mat4_t m)
{
    printf("  %s:\n", label);
    for(int col = 0; col < 4; ++col)
    {
        printf("    col%d: [%8.3f %8.3f %8.3f %8.3f]\n", col,
            m.columns[col].x, m.columns[col].y, m.columns[col].z, m.columns[col].w);
    }
}

static void
print_vec4(const char *label, vec4_t v)
{
    printf("  %s: [%.3f, %.3f, %.3f, %.3f]\n", label, v.x, v.y, v.z, v.w);
}

// =====================================================================
// BENCHMARK FRAMEWORK
// =====================================================================

#define BENCH_ITERATIONS (10 * 1000 * 1000)
#define BENCH_WARMUP     (100 * 1000)

static inline u64
bench_rdtsc()
{
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return((u64)hi << 32 | lo);
}

static inline double
get_time_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return((double)ts.tv_sec * 1e9 + (double)ts.tv_nsec);
}

#define BENCH(name, iterations, code) do { \
    for(int _w = 0; _w < BENCH_WARMUP; ++_w) { code; } \
    u64 _tsc_start = bench_rdtsc(); \
    double _ns_start = get_time_ns(); \
    for(int _i = 0; _i < (iterations); ++_i) { code; } \
    u64 _tsc_end = bench_rdtsc(); \
    double _ns_end = get_time_ns(); \
    u64    _tsc_delta = _tsc_end - _tsc_start; \
    double _ns_total  = _ns_end - _ns_start; \
    double _ns_per    = _ns_total / (double)(iterations); \
    printf("  %-24s  rdtsc: %12llu ticks  |  monotonic: %8.3f ms  (%6.2f ns/call)\n", \
           name, (unsigned long long)_tsc_delta, _ns_total / 1e6, _ns_per); \
} while(0)

// =====================================================================
// CORRECTNESS TESTS
// =====================================================================

void test_vec4_dot()
{
    TEST("vec4_dot");

    vec4_t a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4_t b = {5.0f, 6.0f, 7.0f, 8.0f};
    float32 result = vec4_dot(a, b);
    printf("  vec4_dot result: %.3f, expected: 70.000\n", result);
    CHECK(f32_near(result, 70.0f), "vec4_dot({1,2,3,4}, {5,6,7,8}) == 70");

    vec4_t c = {1.0f, 0.0f, 0.0f, 0.0f};
    vec4_t d = {0.0f, 1.0f, 0.0f, 0.0f};
    float32 ortho = vec4_dot(c, d);
    CHECK(f32_near(ortho, 0.0f), "orthogonal vectors dot to 0");

    vec4_t e = {3.0f, 4.0f, 0.0f, 0.0f};
    float32 self_dot = vec4_dot(e, e);
    CHECK(f32_near(self_dot, 25.0f), "self dot product == length squared");
}

void test_vec4_length()
{
    TEST("vec4_length");

    vec4_t a = {3.0f, 4.0f, 0.0f, 0.0f};
    float32 len = vec4_length(a);
    printf("  vec4_length result: %.3f, expected: 5.000\n", len);
    CHECK(f32_near(len, 5.0f), "length of {3,4,0,0} == 5");

    vec4_t b = {1.0f, 0.0f, 0.0f, 0.0f};
    CHECK(f32_near(vec4_length(b), 1.0f), "unit vector length == 1");

    vec4_t c = {0.0f, 0.0f, 0.0f, 0.0f};
    CHECK(f32_near(vec4_length(c), 0.0f), "zero vector length == 0");

    vec4_t d = {1.0f, 1.0f, 1.0f, 1.0f};
    CHECK(f32_near(vec4_length(d), 2.0f), "length of {1,1,1,1} == 2");
}

void test_vec4_lerp()
{
    TEST("vec4_lerp");

    vec4_t a = {0.0f, 0.0f, 0.0f, 0.0f};
    vec4_t b = {10.0f, 20.0f, 30.0f, 40.0f};

    vec4_t at_0 = vec4_lerp(a, b, 0.0f);
    CHECK(vec4_near(at_0, a), "lerp at t=0 returns A");

    vec4_t at_1 = vec4_lerp(a, b, 1.0f);
    CHECK(vec4_near(at_1, b), "lerp at t=1 returns B");

    vec4_t at_half = vec4_lerp(a, b, 0.5f);
    vec4_t expected_half = {5.0f, 10.0f, 15.0f, 20.0f};
    print_vec4("lerp at t=0.5", at_half);
    CHECK(vec4_near(at_half, expected_half), "lerp at t=0.5 returns midpoint");

    vec4_t c = {2.0f, 4.0f, 6.0f, 8.0f};
    vec4_t d = {10.0f, 12.0f, 14.0f, 16.0f};
    vec4_t at_quarter = vec4_lerp(c, d, 0.25f);
    vec4_t expected_quarter = {4.0f, 6.0f, 8.0f, 10.0f};
    CHECK(vec4_near(at_quarter, expected_quarter), "lerp at t=0.25 with non-zero A");
}

void test_vec4_transform()
{
    TEST("vec4_transform");

    mat4_t identity = mat4_identity();
    vec4_t v = {1.0f, 2.0f, 3.0f, 4.0f};

    vec4_t result = vec4_transform(identity, v);
    CHECK(vec4_near(result, v), "identity transform leaves vector unchanged");

    mat4_t scale = {};
    scale.columns[0].x = 2.0f;
    scale.columns[1].y = 3.0f;
    scale.columns[2].z = 4.0f;
    scale.columns[3].w = 1.0f;

    vec4_t scaled = vec4_transform(scale, v);
    vec4_t expected_scaled = {2.0f, 6.0f, 12.0f, 4.0f};
    print_vec4("scaled", scaled);
    CHECK(vec4_near(scaled, expected_scaled), "diagonal scale * {1,2,3,4} == {2,6,12,4}");
}

void test_mat4_add()
{
    TEST("mat4_add");

    mat4_t a = {};
    mat4_t b = {};
    for(int i = 0; i < 16; ++i)
    {
        a.values[i] = (float32)(i + 1);
        b.values[i] = (float32)(i + 1) * 2.0f;
    }

    mat4_t result = mat4_add(a, b);
    mat4_t expected = {};
    for(int i = 0; i < 16; ++i)
    {
        expected.values[i] = a.values[i] + b.values[i];
    }

    CHECK(mat4_near(result, expected), "mat4_add element-wise correct");

    mat4_t zero = {};
    mat4_t add_zero = mat4_add(a, zero);
    CHECK(mat4_near(add_zero, a), "A + 0 == A");
}

void test_mat4_subtract()
{
    TEST("mat4_subtract");

    mat4_t a = {};
    mat4_t b = {};
    for(int i = 0; i < 16; ++i)
    {
        a.values[i] = (float32)(i + 1) * 3.0f;
        b.values[i] = (float32)(i + 1);
    }

    mat4_t result = mat4_subtract(a, b);
    mat4_t expected = {};
    for(int i = 0; i < 16; ++i)
    {
        expected.values[i] = a.values[i] - b.values[i];
    }

    CHECK(mat4_near(result, expected), "mat4_subtract element-wise correct");

    mat4_t self_sub = mat4_subtract(a, a);
    mat4_t zero = {};
    CHECK(mat4_near(self_sub, zero), "A - A == 0");
}

void test_mat4_divide()
{
    TEST("mat4_divide");

    mat4_t a = {};
    mat4_t b = {};
    for(int i = 0; i < 16; ++i)
    {
        a.values[i] = (float32)(i + 1) * 6.0f;
        b.values[i] = (float32)(i + 1) * 2.0f;
    }

    mat4_t result = mat4_divide(a, b);
    mat4_t expected = {};
    for(int i = 0; i < 16; ++i)
    {
        expected.values[i] = a.values[i] / b.values[i];
    }

    CHECK(mat4_near(result, expected), "mat4_divide element-wise correct");
}

void test_mat4_reduce()
{
    TEST("mat4_reduce");

    mat4_t a = {};
    for(int i = 0; i < 16; ++i)
    {
        a.values[i] = (float32)(i + 1) * 4.0f;
    }

    mat4_t result = mat4_reduce(a, 4.0f);
    mat4_t expected = {};
    for(int i = 0; i < 16; ++i)
    {
        expected.values[i] = a.values[i] / 4.0f;
    }

    CHECK(mat4_near(result, expected), "mat4_reduce by 4.0 correct");

    mat4_t reduce_one = mat4_reduce(a, 1.0f);
    CHECK(mat4_near(reduce_one, a), "A / 1 == A");
}

void test_mat4_multiply()
{
    TEST("mat4_multiply");

    mat4_t identity = mat4_identity();
    mat4_t a = {};
    for(int i = 0; i < 16; ++i)
    {
        a.values[i] = (float32)(i + 1);
    }

    mat4_t id_mul = mat4_multiply(identity, a);
    CHECK(mat4_near(id_mul, a), "I * A == A");

    mat4_t mul_id = mat4_multiply(a, identity);
    CHECK(mat4_near(mul_id, a), "A * I == A");

    mat4_t b = {};
    b.columns[0] = {2.0f, 0.0f, 0.0f, 0.0f};
    b.columns[1] = {0.0f, 2.0f, 0.0f, 0.0f};
    b.columns[2] = {0.0f, 0.0f, 2.0f, 0.0f};
    b.columns[3] = {0.0f, 0.0f, 0.0f, 2.0f};

    mat4_t doubled = mat4_multiply(a, b);
    mat4_t expected_double = {};
    for(int i = 0; i < 16; ++i)
    {
        expected_double.values[i] = a.values[i] * 2.0f;
    }
    CHECK(mat4_near(doubled, expected_double), "A * 2I == 2A");

    mat4_t c = {};
    c.columns[0] = {1.0f,  0.0f, 0.0f, 0.0f};
    c.columns[1] = {0.0f,  1.0f, 0.0f, 0.0f};
    c.columns[2] = {0.0f,  0.0f, 1.0f, 0.0f};
    c.columns[3] = {5.0f, 10.0f, 15.0f, 1.0f};

    mat4_t d = {};
    d.columns[0] = {2.0f, 0.0f, 0.0f, 0.0f};
    d.columns[1] = {0.0f, 3.0f, 0.0f, 0.0f};
    d.columns[2] = {0.0f, 0.0f, 4.0f, 0.0f};
    d.columns[3] = {1.0f, 2.0f, 3.0f, 1.0f};

    mat4_t cd_expected = {};
    cd_expected.columns[0] = {2.0f,  0.0f,  0.0f, 0.0f};
    cd_expected.columns[1] = {0.0f,  3.0f,  0.0f, 0.0f};
    cd_expected.columns[2] = {0.0f,  0.0f,  4.0f, 0.0f};
    cd_expected.columns[3] = {6.0f, 12.0f, 18.0f, 1.0f};

    mat4_t cd_result = mat4_multiply(c, d);
    print_mat4("C * D result", cd_result);
    print_mat4("C * D expected", cd_expected);
    CHECK(mat4_near(cd_result, cd_expected), "C * D (translation * scale+translate) correct");

    // Dense multiply
    mat4_t e = {};
    e.columns[0] = {1.0f, 2.0f, 3.0f, 4.0f};
    e.columns[1] = {5.0f, 6.0f, 7.0f, 8.0f};
    e.columns[2] = {9.0f, 10.0f, 11.0f, 12.0f};
    e.columns[3] = {13.0f, 14.0f, 15.0f, 16.0f};

    mat4_t f = {};
    f.columns[0] = {17.0f, 18.0f, 19.0f, 20.0f};
    f.columns[1] = {21.0f, 22.0f, 23.0f, 24.0f};
    f.columns[2] = {25.0f, 26.0f, 27.0f, 28.0f};
    f.columns[3] = {29.0f, 30.0f, 31.0f, 32.0f};

    mat4_t ef_expected = {};
    for(int j = 0; j < 4; ++j)
    {
        for(int r = 0; r < 4; ++r)
        {
            ef_expected.columns[j].elements[r] =
                e.columns[0].elements[r] * f.columns[j].elements[0] +
                e.columns[1].elements[r] * f.columns[j].elements[1] +
                e.columns[2].elements[r] * f.columns[j].elements[2] +
                e.columns[3].elements[r] * f.columns[j].elements[3];
        }
    }

    mat4_t ef_result = mat4_multiply(e, f);
    print_mat4("E * F result", ef_result);
    print_mat4("E * F expected", ef_expected);
    CHECK(mat4_near(ef_result, ef_expected), "dense 4x4 multiply correct");
}

void test_mat4_translate()
{
    TEST("mat4_translate");

    mat4_t identity = mat4_identity();
    vec3_t translation = {5.0f, 10.0f, 15.0f};

    mat4_t result = mat4_translate(identity, translation);

    mat4_t expected = mat4_identity();
    expected.columns[3].x = 5.0f;
    expected.columns[3].y = 10.0f;
    expected.columns[3].z = 15.0f;

    print_mat4("translate result", result);
    CHECK(mat4_near(result, expected), "translate identity by {5,10,15}");

    mat4_t result2 = mat4_translate(result, translation);
    mat4_t expected2 = mat4_identity();
    expected2.columns[3].x = 10.0f;
    expected2.columns[3].y = 20.0f;
    expected2.columns[3].z = 30.0f;
    CHECK(mat4_near(result2, expected2), "double translation accumulates");

    mat4_t scaled = mat4_identity();
    scaled.columns[0].x = 2.0f;
    scaled.columns[1].y = 2.0f;
    scaled.columns[2].z = 2.0f;

    mat4_t scaled_translated = mat4_translate(scaled, {3.0f, 4.0f, 5.0f});
    mat4_t st_expected = scaled;
    st_expected.columns[3] = {6.0f, 8.0f, 10.0f, 1.0f};
    print_mat4("scale then translate result", scaled_translated);
    CHECK(mat4_near(scaled_translated, st_expected), "translate after scale uses scaled basis");

    mat4_t no_op = mat4_translate(scaled, {0.0f, 0.0f, 0.0f});
    CHECK(mat4_near(no_op, scaled), "translate by {0,0,0} is no-op");
}

void test_mat4_scale()
{
    TEST("mat4_scale");

    mat4_t identity = mat4_identity();
    vec3_t scale = {2.0f, 3.0f, 4.0f};

    mat4_t result = mat4_scale(identity, scale);

    mat4_t expected = mat4_identity();
    expected.columns[0].x = 2.0f;
    expected.columns[1].y = 3.0f;
    expected.columns[2].z = 4.0f;

    print_mat4("scale result", result);
    CHECK(mat4_near(result, expected), "scale identity by {2,3,4}");

    mat4_t no_op = mat4_scale(identity, {1.0f, 1.0f, 1.0f});
    CHECK(mat4_near(no_op, identity), "scale by {1,1,1} is no-op");

    mat4_t translated = mat4_identity();
    translated.columns[3] = {5.0f, 10.0f, 15.0f, 1.0f};
    mat4_t ts_result = mat4_scale(translated, {2.0f, 3.0f, 4.0f});

    mat4_t ts_expected = {};
    ts_expected.columns[0] = {2.0f,  0.0f,  0.0f, 0.0f};
    ts_expected.columns[1] = {0.0f,  3.0f,  0.0f, 0.0f};
    ts_expected.columns[2] = {0.0f,  0.0f,  4.0f, 0.0f};
    ts_expected.columns[3] = {5.0f, 10.0f, 15.0f, 1.0f};

    CHECK(mat4_near(ts_result, ts_expected), "scale after translate preserves translation col");

    mat4_t double_scaled = mat4_scale(result, {2.0f, 2.0f, 2.0f});
    mat4_t ds_expected = mat4_identity();
    ds_expected.columns[0].x = 4.0f;
    ds_expected.columns[1].y = 6.0f;
    ds_expected.columns[2].z = 8.0f;
    CHECK(mat4_near(double_scaled, ds_expected), "double scale accumulates");
}

void test_multiply_vs_transform()
{
    TEST("mat4_multiply matches per-column vec4_transform");

    mat4_t a = {};
    a.columns[0] = {1.0f,  5.0f,  9.0f, 13.0f};
    a.columns[1] = {2.0f,  6.0f, 10.0f, 14.0f};
    a.columns[2] = {3.0f,  7.0f, 11.0f, 15.0f};
    a.columns[3] = {4.0f,  8.0f, 12.0f, 16.0f};

    mat4_t b = {};
    b.columns[0] = {17.0f, 21.0f, 25.0f, 29.0f};
    b.columns[1] = {18.0f, 22.0f, 26.0f, 30.0f};
    b.columns[2] = {19.0f, 23.0f, 27.0f, 31.0f};
    b.columns[3] = {20.0f, 24.0f, 28.0f, 32.0f};

    mat4_t mul_result = mat4_multiply(a, b);

    vec4_t col0 = vec4_transform(a, b.columns[0]);
    vec4_t col1 = vec4_transform(a, b.columns[1]);
    vec4_t col2 = vec4_transform(a, b.columns[2]);
    vec4_t col3 = vec4_transform(a, b.columns[3]);

    CHECK(vec4_near(mul_result.columns[0], col0), "result col0 matches vec4_transform");
    CHECK(vec4_near(mul_result.columns[1], col1), "result col1 matches vec4_transform");
    CHECK(vec4_near(mul_result.columns[2], col2), "result col2 matches vec4_transform");
    CHECK(vec4_near(mul_result.columns[3], col3), "result col3 matches vec4_transform");
}

void test_mat4_transpose()
{
    TEST("mat4_transpose");

    mat4_t a = {};
    a.columns[0] = {1.0f,  2.0f,  3.0f,  4.0f};
    a.columns[1] = {5.0f,  6.0f,  7.0f,  8.0f};
    a.columns[2] = {9.0f, 10.0f, 11.0f, 12.0f};
    a.columns[3] = {13.0f, 14.0f, 15.0f, 16.0f};

    mat4_t t = mat4_transpose(a);

    CHECK(f32_near(t.columns[0].x, 1.0f) && f32_near(t.columns[0].y, 5.0f) &&
          f32_near(t.columns[0].z, 9.0f) && f32_near(t.columns[0].w, 13.0f),
          "transpose col0 correct");
    CHECK(f32_near(t.columns[1].x, 2.0f) && f32_near(t.columns[1].y, 6.0f) &&
          f32_near(t.columns[1].z, 10.0f) && f32_near(t.columns[1].w, 14.0f),
          "transpose col1 correct");

    mat4_t tt = mat4_transpose(t);
    CHECK(mat4_near(tt, a), "double transpose == original");
}

// =====================================================================
// BENCHMARKS
// =====================================================================

// NOTE(Sleepster): Force the compiler to treat a value as opaque so it
// can't hoist loop-invariant computations.
static void
do_not_optimize(void *ptr)
{
    __asm__ __volatile__("" : : "g"(ptr) : "memory");
}

void run_benchmarks()
{
    printf("\n========================================\n");
    printf("  BENCHMARKS (%d iterations per function)\n", BENCH_ITERATIONS);
    printf("========================================\n\n");

    // Test data -- dense matrices
    mat4_t ma = {};
    ma.columns[0] = { 1.0f,  5.0f,  9.0f, 13.0f};
    ma.columns[1] = { 2.0f,  6.0f, 10.0f, 14.0f};
    ma.columns[2] = { 3.0f,  7.0f, 11.0f, 15.0f};
    ma.columns[3] = { 4.0f,  8.0f, 12.0f, 16.0f};

    mat4_t mb = {};
    mb.columns[0] = {17.0f, 21.0f, 25.0f, 29.0f};
    mb.columns[1] = {18.0f, 22.0f, 26.0f, 30.0f};
    mb.columns[2] = {19.0f, 23.0f, 27.0f, 31.0f};
    mb.columns[3] = {20.0f, 24.0f, 28.0f, 32.0f};

    vec4_t va = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4_t vb = {5.0f, 6.0f, 7.0f, 8.0f};

    vec3_t translate_v = {5.0f, 10.0f, 15.0f};
    vec3_t scale_v     = {2.0f, 3.0f, 4.0f};

    mat4_t r;
    vec4_t vr;
    float32 fr;

    printf("  --- vec4 functions ---\n");

    BENCH("vec4_dot", BENCH_ITERATIONS, {
        fr = vec4_dot(va, vb);
        do_not_optimize(&fr);
    });

    BENCH("vec4_length", BENCH_ITERATIONS, {
        fr = vec4_length(va);
        do_not_optimize(&fr);
    });

    BENCH("vec4_lerp", BENCH_ITERATIONS, {
        vr = vec4_lerp(va, vb, 0.5f);
        do_not_optimize(&vr);
    });

    BENCH("vec4_normalize", BENCH_ITERATIONS, {
        vr = vec4_normalize(va);
        do_not_optimize(&vr);
    });

    BENCH("vec4_transform", BENCH_ITERATIONS, {
        vr = vec4_transform(ma, va);
        do_not_optimize(&vr);
    });

    printf("\n  --- mat4 arithmetic ---\n");

    BENCH("mat4_add", BENCH_ITERATIONS, {
        r = mat4_add(ma, mb);
        do_not_optimize(&r);
    });

    BENCH("mat4_subtract", BENCH_ITERATIONS, {
        r = mat4_subtract(ma, mb);
        do_not_optimize(&r);
    });

    BENCH("mat4_divide", BENCH_ITERATIONS, {
        r = mat4_divide(ma, mb);
        do_not_optimize(&r);
    });

    BENCH("mat4_reduce", BENCH_ITERATIONS, {
        r = mat4_reduce(ma, 3.14f);
        do_not_optimize(&r);
    });

    printf("\n  --- mat4 multiply/transform ---\n");

    BENCH("mat4_multiply", BENCH_ITERATIONS, {
        r = mat4_multiply(ma, mb);
        do_not_optimize(&r);
    });

    BENCH("mat4_transpose", BENCH_ITERATIONS, {
        r = mat4_transpose(ma);
        do_not_optimize(&r);
    });

    printf("\n  --- mat4 translate/scale ---\n");

    BENCH("mat4_translate", BENCH_ITERATIONS, {
        r = mat4_translate(ma, translate_v);
        do_not_optimize(&r);
    });

    BENCH("mat4_scale", BENCH_ITERATIONS, {
        r = mat4_scale(ma, scale_v);
        do_not_optimize(&r);
    });
}

// =====================================================================
// main
// =====================================================================
int main(int argc, char **argv)
{
    bool8 bench_only = false;
    if(argc > 1 && strcmp(argv[1], "bench") == 0)
    {
        bench_only = true;
    }

    printf("========================================\n");
    printf("  c_math.h Verification Tests\n");
    printf("========================================\n");

#ifdef __AVX2__
    printf("  [AVX2 enabled]\n");
#else
    printf("  [SSE only]\n");
#endif

#ifdef __FMA__
    printf("  [FMA enabled]\n");
#endif

#ifdef __SSE4_1__
    printf("  [SSE4.1 enabled]\n");
#endif

    if(!bench_only)
    {
        test_vec4_dot();
        test_vec4_length();
        test_vec4_lerp();
        test_vec4_transform();

        test_mat4_add();
        test_mat4_subtract();
        test_mat4_divide();
        test_mat4_reduce();

        test_mat4_multiply();
        test_multiply_vs_transform();
        test_mat4_transpose();

        test_mat4_translate();
        test_mat4_scale();

        printf("\n========================================\n");
        printf("  Results: %d passed, %d failed\n", tests_passed, tests_failed);
        printf("========================================\n");
    }

    run_benchmarks();

    return tests_failed > 0 ? 1 : 0;
}
