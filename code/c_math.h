#if !defined(C_MATH_H)
/* ========================================================================
   $File: c_math.h $
   $Date: Mon, 30 Jun 25: 02:32PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
/*
  Sort the functions by their types, like so:
     - float32 functions
     - float64 functions
  
     - Vector2 Functions
     - Vector3 Functions
     - Vector4 Functions

     - iVector2 Functions
     - iVector3 Functions
     - iVector4 Functions

     - Matrix2x2 Functions
     - Matrix3x3 Functions
     - Matrix4x4 Functions

     - Quaternion Functions

     - Rectangle Functions
     - Cube Functions
  TODO:

  SSE2 SIMD (mm128) ON ALL FUNCTIONS POSSIBLE
  
  - MATRICES
      - [x] VECTOR TRANSFORMS
      - [x] TRANSLATE
      - [x] ROTATE
      - [x] SCALE
      - [x] DETERMINATES
      - [x] INVERSION
      - [x] INVERSE DETERMINATES
      - [x] TRANSPOSITION
      - [x] ARITHMATIC FUNCTIONS (sub, add, mul, div)
      - [x] REDUCE (uniform div by a scaler)

      - [x] ORTHOGRAPHIC MATRICES
      - [ ] PERSPECTIVE MATRICES
      - [ ] LOOK-AT MATRICES

  - VECTORS
      - [X] LENGTH FUNCTIONS
      - [X] CROSS PRODUCT
      - [X] DOT PRODUCT
      - [X] NORMALIZE
      - [x] SCALING
      - [X] LENGTH SQUARED
      - [x] LERP
      - [ ] SLERP
      
  - QUATERNIONS
      - EVERYTHING

  - CREATE A WAY OF TOGGLING BETWEEN SSE AND NON-SSE FUNCTIONS
      (check handmade_math.h)
*/
  
#define C_MATH_H

#include <math.h>
#include <xmmintrin.h>

#include <c_types.h>

#define PI32 3.14159265359f
#define PI64 3.14159265358979323846

#define TAU32 ((PI32) * 2)
#define TAU64 ((PI64) * 2)

#define DegToRad(x) (x * (float32)(PI32 / 180.0f))
#define RadToDeg(x) (x * (float32)(180.0f / PI32))

#define Abs(A)  ((A) > 0 ? (A) : -(A))
#define Sign(A) ((A) > 0 ? 1 : -1) 

#define Clamp(X, A, B) (((X) < (A)) ? (A) : ((B) < (X)) ? (B) : (X))
#define ClampTop(A, B) (Min(A, B))
#define ClampBot(A, B) (Max(A, B))

#define Min(A, B) (((A) < (B)) ? (A) : (B))
#define Max(A, B) (((A) > (B)) ? (A) : (B)) 

#define Square(x) ((x) * (x))

/*===========================================
  ============= FLOAT32 FUNCTIONS ===========
  ===========================================*/

internal inline float32
f32_lerp(float32 A, float32 B, float32 T)
{
    float32 result;
    result = A + (B - A) * T;

    return(result);
}

internal inline float32
f32_unlerp(float32 A, float32 B, float32 X)
{
    float32 result = 0.0f;
    if(A != B)
    {
        result = (X - A) / (B - A);
    }

    return(result);
}

internal inline bool8
f32_equals(float32 A, float32 B, float32 tolerance)
{
    return(fabs(A - B) <= tolerance);
}

internal inline void
f32_approach(float32 *value, float32 target, float32 rate, float32 delta_t)
{
    *value += (float32)((target - *value) * (1.0 - pow(2.0f, -rate * delta_t)));
    if(f32_equals(*value, target, 0.01f))
    {
        *value = target;
    }
}

internal inline float32
f32_ease_out_quad(float32 x)
{
    return(1 - (1 - x) * (1 - x));
}

internal inline float32
f32_sin_breathe_normalized(float32 time, float32 modifier, float32 min, float32 max)
{
    float32 sinevalue = (sinf(modifier * 2 * PI32 * time) + 1.0f) / 2.0f;
    return(min + (max - min) * sinevalue);
}

internal inline float32
f32_sin_breathe(float32 time, float32 modifier)
{
    return(sinf(time * modifier));
}

#if 0
#ifdef __cplusplus
#include <typeinfo>

// -std=c++20 
internal inline auto 
lerp(auto A, auto B)
{
    return(A + B);
}

#endif
#endif

/*===========================================
  ============= FLOAT64 FUNCTIONS ===========
  ===========================================*/

/*===========================================
  =============== FLOAT VECTORS =============
  ===========================================*/
typedef struct vec2
{
    union
    {
        float32 elements[2];
        struct
        {
            float32 x;
            float32 y;
        };

        struct
        {
            float32 xy[2];
        };
    };
}vec2_t;

typedef struct vec3
{
    union
    {
        float32 elements[3];
        struct
        {
            float32 x;
            float32 y;
            float32 z;
        };

        struct
        {
            vec2_t  xy;
            float32 z_;
        };
    };
}vec3_t;

typedef struct vec4
{
    union
    {
        float32 elements[4];
        struct
        {
            float32 x;
            float32 y;
            float32 z;
            float32 w;
        };

        struct
        {
            vec2_t xy;
            vec2_t zw;
        };

        struct
        {
            vec3_t xyz;
            float32 _w;
        };

        __m128 SSE;
    };
}vec4_t;

/*===========================================
  ============== FLOAT MATRICES =============
  ===========================================*/

/*

NOTE(Sleepster):
ALL OF THE MATRICES ARE COLUMN MAJOR
  
*/

typedef struct mat4
{
    union
    {
        float32 elements[4][4];
        float32 values[16];
        vec4_t  columns[4];

        __m128 SSE[4];

        struct
        {
            float32 _00;
            float32 _01;
            float32 _02;
            float32 _03;
            float32 _10;
            float32 _11;
            float32 _12;
            float32 _13;
            float32 _20;
            float32 _21;
            float32 _22;
            float32 _23;
            float32 _30;
            float32 _31;
            float32 _32;
            float32 _33;
        };

        struct
        {
            vec4_t column0;
            vec4_t column1;
            vec4_t column2;
            vec4_t column3;
        };
    };
}mat4_t;

typedef struct mat3
{
    union
    {
        float32 elements[3][3];
        float32 values[9];
        vec3_t  columns[3];
        struct
        {
            float32 _00;
            float32 _01;
            float32 _02;
            float32 _10;
            float32 _11;
            float32 _12;
            float32 _20;
            float32 _21;
            float32 _22;
        };
    };
}mat3_t;

typedef struct mat2
{
    union
    {
        float32 elements[2][2];
        float32 values[4];
        vec2_t  columns[2];
        struct
        {
            float32 _00;
            float32 _01;
            float32 _10;
            float32 _11;
        };
    }; 
}mat2_t;

/*===========================================
  ============= INTEGER VECTORS =============
  ===========================================*/
typedef struct ivec4
{
    union
    {
        s32 elements[4];
        struct
        {
            s32 x;
            s32 y;
            s32 z;
            s32 w;
        };

        struct
        {
            s32 xy[2];
            s32 wz[2];
        };

        struct
        {
            s32 xyz[3];
            s32 z_;
        };

        __m128i SSE;
    };
}ivec4_t;

typedef struct ivec3
{
    union
    {
        s32 elements[3];
        struct
        {
            s32 x;
            s32 y;
            s32 z;
        };

        struct
        {
            s32 xy[2];
            s32 z_1;
        };
    };
}ivec3_t;

typedef struct ivec2
{
    union
    {
        s32 elements[2];
        struct
        {
            s32 x;
            s32 y;
        };

        struct
        {
            s32 width;
            s32 height;
        };
    };
}ivec2_t;

#define NULL_VECTOR2 (vec2())
#define NULL_VECTOR3 (vec3())
#define NULL_VECTOR4 (vec4())

#define NULL_IVECTOR2 (ivec2())
#define NULL_IVECTOR3 (ivec3())
#define NULL_IVECTOR4 (ivec4())

#define NULL_MATRIX4 (mat4_identity())
#define NULL_MATRIX3 (mat3_identity())
#define NULL_MATRIX2 (mat2_identity())

/*===========================================
  ================= VECTOR 2 ================
  ===========================================*/

internal inline vec2_t
vec2(float32 A, float32 B)
{
    vec2_t result;
    result.x = A;
    result.y = B;
    
    return(result);
}

internal inline vec2_t
vec2_zero()
{
    vec2_t result = {};
    return(result);
}

internal inline vec2_t
vec2_create(float32 A)
{
    vec2_t result = {};
    result.x = A;
    result.y = A;

    return(result);
}

internal inline vec3_t
vec2_expand_vec3(vec2_t A, float32 B)
{
    vec3_t result;
    result.x = A.x;
    result.y = A.y;
    result.z = B;

    return(result);
}

internal inline vec2_t
vec2_cast(ivec2_t A)
{
    vec2_t result;
    result.x = (float32)A.x;
    result.y = (float32)A.y;

   return(result);
}

internal inline vec4_t
vec2_expand_vec4(vec2_t A, float32 B, float32 C)
{
    vec4_t result;
    result.x = A.x;
    result.y = A.y;
    result.z = B;
    result.w = C;

    return(result);
}

internal inline vec2_t
vec2_scale(vec2_t A, float32 B)
{
    vec2_t result;
    result.x = A.x * B;
    result.y = A.y * B;

    return(result);
}

internal inline vec2_t
vec2_multiply(vec2_t A, vec2_t B)
{
    vec2_t result;
    result.x = A.x * B.x;
    result.y = A.y * B.y;

    return(result);
}

internal inline vec2_t
vec2_add(vec2_t A, vec2_t B)
{
    vec2_t result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return(result);
}

internal inline vec2_t
vec2_subtract(vec2_t A, vec2_t B)
{
    vec2_t result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return(result);
}

internal inline vec2_t
vec2_reduce(vec2_t A, float32 B)
{
    vec2_t result;
    result.x = A.x / B;
    result.y = A.y / B;

    return(result);
}

// NOTE(Sleepster): "Should be magnitude" bla bla bla don't care 
internal inline float32
vec2_length(vec2_t A)
{
    float32 result = 0.0f;
    result = sqrtf(Square(A.x) + Square(A.y));

    return(result);
}

internal inline float32
vec2_length_squared(vec2_t A)
{
    float32 result = 0.0f;
    result = Square(A.x) + Square(A.y);

    return(result);
}

internal inline vec2_t
vec2_normalize(vec2_t A)
{
    vec2_t result = {};

    float32 magnitude = vec2_length(A);
    if(magnitude > 0.0f)
    {
        result.x = A.x / magnitude;
        result.y = A.y / magnitude;
    }

    return(result);
}

internal inline float32
vec2_dot(vec2_t A, vec2_t B)
{
    float32 result;
    result = (A.x * B.x) + (A.y * B.y);

    return(result);
}

internal inline float32
vec2_cross(vec2_t A, vec2_t B)
{
    float32 result;
    result = (A.x * B.x) - (A.y * B.y);
    
    return(result);
}

internal inline vec2_t
vec2_transform(mat2_t A, vec2_t B)
{
    vec2_t result;
    result.x = (A._00 * B.x) + (A._01 * B.y);
    result.y = (A._10 * B.x) + (A._11 * B.y);
    
    return(result);
}

internal inline void
vec2_approach(vec2_t *value, vec2_t target, vec2_t rate, float32 delta_t)
{
    f32_approach(&value->x, target.x, rate.x, delta_t);
    f32_approach(&value->y, target.y, rate.y, delta_t);
}

#ifndef SL_MATH_USE_DEGREES
internal inline vec2_t
vec2_rotate(vec2_t A, float32 rotation)
{
    vec2_t result;
    
    float32 cos_angle = cosf(-rotation);
    float32 sin_angle = sinf(-rotation);

    result.x = (A.x * cos_angle) - (A.y * sin_angle);
    result.y = (A.x * sin_angle) + (A.y * cos_angle);

    return(result);
}

#else

internal inline vec2_t
vec2_rotate(vec2_t A, float32 rotation)
{
    vec2_t result;
    
    float32 cos_angle = cosf(-deg_to_rad(rotation));
    float32 sin_angle = sinf(-deg_to_rad(rotation));

    result.x = (A.x * cos_angle) + (A.y * sin_angle);
    result.y = (A.y * sin_angle) + (A.y * cos_angle);

    return(result);
}
#endif

internal inline vec2_t
vec2_lerp(vec2_t A, vec2_t B, real32 time)
{
    vec2_t result;
    result.x = f32_lerp(A.x, B.x, time);
    result.y = f32_lerp(A.y, B.y, time);

    return(result);
}

internal inline vec2_t
vec2_unlerp(vec2_t A, vec2_t B, vec2_t X)
{
    vec2_t result;
    result.x = f32_unlerp(A.x, B.x, X.x);
    result.y = f32_unlerp(A.y, B.y, X.y);

    return(result);
}

internal inline vec2_t 
vec2_negate(vec2_t A)
{
    vec2_t result;

    result.x = -A.x;
    result.y = -A.y;

    return(result);
}


#ifdef __cplusplus

inline vec2_t 
operator+(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return(result);
}

inline vec2_t 
operator-(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return(result);
}

inline vec2_t
operator*(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;

    return(result);
}

inline vec2_t
operator/(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x / B.x;
    result.y = A.y / B.y;

    return(result);
}

inline vec2_t
operator+=(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;

    return(result);
}

inline vec2_t
operator-=(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;

    return(result);
}

inline vec2_t 
operator*=(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;

    return(result);
}

inline vec2_t 
operator/=(vec2_t A, vec2_t B)
{
    vec2_t result;

    result.x = A.x / B.x;
    result.y = A.y / B.y;

    return(result);
}

#endif


/*===========================================
  ================= VECTOR 3 ================
  ===========================================*/

internal inline vec3_t
vec3(float32 A, float32 B, float32 C)
{
    vec3_t result;
    result.x = A;
    result.y = B;
    result.z = C;

    return(result);
}

internal inline vec3_t
vec3_zero()
{
    vec3_t result = {};
    return(result);
}

internal inline vec3_t
vec3_create(float32 A)
{
    vec3_t result;
    result.x = A;
    result.y = A;
    result.z = A;

    return(result);
}

internal inline vec3_t
vec3_cast(ivec3_t A)
{
    vec3_t result;
    result.x = (float32)A.x;
    result.y = (float32)A.y;
    result.z = (float32)A.z;

    return(result);
}

internal inline vec3_t
vec3_multiply(vec3_t A, vec3_t B)
{
    vec3_t result;
    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;

    return(result);
}

internal inline vec3_t
vec3_subtract(vec3_t A, vec3_t B)
{
    vec3_t result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return(result);
}

internal inline vec3_t
vec3_add(vec3_t A, vec3_t B)
{
    vec3_t result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return(result);
}

internal inline vec3_t
vec3_scale(vec3_t A, float32 B)
{
    vec3_t result;
    result.x = A.x * B;
    result.y = A.y * B;
    result.z = A.z * B;

    return(result);
}

internal inline float32
vec3_length(vec3_t A)
{
    float32 result;
    result = sqrt(Square(A.x) + Square(A.y) + Square(A.z));

    return(result);
}

internal inline vec3_t
vec3_normalize(vec3_t A)
{
    vec3_t result;
    
    float32 magnitude = vec3_length(A);
    result.x = A.x / magnitude;
    result.y = A.y / magnitude;
    result.z = A.z / magnitude;

    return(result);
}

internal inline float32
vec3_dot(vec3_t A, vec3_t B)
{
    float32 result;
    result = (A.x * B.x) + (A.y * B.y) + (A.z * B.z);

    return(result);
}

internal inline vec3_t
vec3_cross(vec3_t A, vec3_t B)
{
    vec3_t result;
    result.x = (A.y * B.z) - (A.z * B.y);
    result.y = (A.z * B.x) - (A.x * B.z);
    result.z = (A.x * B.y) - (A.y * B.x);

    return(result);
}

internal inline vec3_t
vec3_transform(vec3_t A, mat3_t B)
{
    vec3_t result;
    result.x = (B._00 * A.x) + (B._01 * A.y) + (B._02 * A.z);
    result.y = (B._10 * A.x) + (B._11 * A.y) + (B._12 * A.z);
    result.z = (B._20 * A.x) + (B._21 * A.y) + (B._22 * A.z);

    return(result);
}

internal inline vec4_t
vec3_expand_vec4(vec3_t A, float32 B)
{
    vec4_t result;
    result.x = A.x;
    result.y = A.y;
    result.z = A.z;
    result.w = B;

    return(result);
}


#ifndef SL_MATH_USE_DEGREES
internal inline vec3_t
vec3_rotate(vec3_t A, vec3_t axis, float32 rotation)
{
    vec3_t result;

    float32 cos_angle  = cosf(-rotation);
    float32 sin_angle  = sinf(-rotation);

    float32 axis_dot   = vec3_dot(axis, A);
    vec3_t  axis_cross = vec3_cross(axis, A);

    float32 k          = axis_dot * (1.0f - cos_angle);

    result.x = (A.x * cos_angle) + (axis_cross.x * sin_angle) + (axis.x * k);
    result.y = (A.y * cos_angle) + (axis_cross.y * sin_angle) + (axis.y * k);
    result.z = (A.z * cos_angle) + (axis_cross.z * sin_angle) + (axis.z * k);

    return(result);
}

#else

internal inline vec3_t
vec3_rotate(vec3_t A, vec3_t axis, float32 rotation)
{
    vec3_t result;

    float32 cos_angle  = cosf(deg_to_rad(-rotation));
    float32 sin_angle  = sinf(deg_to_rad(-rotation));

    float32 axis_dot   = vec3_dot(axis, A);
    vec3_t  axis_cross = vec3_cross(axis, A);

    float32 k          = axis_dot * (1.0f - cos_angle);

    result.x = (A.x * cos_angle) + (axis_cross.x * sin_angle) + (axis.x * k);
    result.y = (A.y * cos_angle) + (axis_cross.y * sin_angle) + (axis.y * k);
    result.z = (A.z * cos_angle) + (axis_cross.z * sin_angle) + (axis.z * k);

    return(result);
}
#endif

internal inline vec3_t
vec3_lerp(vec3_t A, vec3_t B, real32 time)
{
    vec3_t result;
    result.x = f32_lerp(A.x, B.x, time);
    result.y = f32_lerp(A.y, B.y, time);
    result.z = f32_lerp(A.z, B.z, time);

    return(result);
}

internal inline vec3_t
vec3_unlerp(vec3_t A, vec3_t B, vec3_t X)
{
    vec3_t result;
    result.x = f32_unlerp(A.x, B.x, X.x);
    result.y = f32_unlerp(A.y, B.y, X.y);
    result.z = f32_unlerp(A.z, B.z, X.z);

    return(result);
}


#ifdef __cplusplus

inline vec3_t 
operator+(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return(result);
}

inline vec3_t 
operator-(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return(result);
}

inline vec3_t
operator*(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;

    return(result);
}

inline vec3_t
operator/(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x / B.x;
    result.y = A.y / B.y;
    result.z = A.z / B.z;

    return(result);
}

inline vec3_t
operator+=(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x + B.x;
    result.y = A.y + B.y;
    result.z = A.z + B.z;

    return(result);
}

inline vec3_t
operator-=(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x - B.x;
    result.y = A.y - B.y;
    result.z = A.z - B.z;

    return(result);
}

inline vec3_t 
operator*=(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;

    return(result);
}

inline vec3_t 
operator/=(vec3_t A, vec3_t B)
{
    vec3_t result;

    result.x = A.x / B.x;
    result.y = A.y / B.y;
    result.z = A.z / B.z;

    return(result);
}

#endif


/*===========================================
  ================= VECTOR 4 ================
  ===========================================*/
internal inline vec4_t
vec4(float32 A, float32 B, float32 C, float32 D)
{
    vec4_t result;
    result.SSE = _mm_setr_ps(A, B, C, D);

    return(result);
}

internal inline vec4_t
vec4_zero()
{
    vec4_t result = {};
    return(result);
}

internal inline vec4_t
vec4_create(float32 A)
{
    vec4_t result;
    // TODO(Sleepster): Perhaps make this a 32 bit -> 128bit SIMD function???
    // (128bit load)
    result.SSE = _mm_set_ps1(A);

    return(result);
}

internal inline vec4_t
vec4_scale(vec4_t A, float32 B)
{
    vec4_t result;

    __m128 B_vector = _mm_set_ps1(B);
    result.SSE      = _mm_mul_ps(A.SSE, B_vector);

    return(result);
}

internal inline vec4_t
vec4_multiply(vec4_t A, vec4_t B)
{
    vec4_t result;
    result.SSE = _mm_mul_ps(A.SSE, B.SSE);

    return(result);
}

internal inline float32
vec4_length(vec4_t A)
{
    float32 result;
    result = sqrt(Square(A.x) + Square(A.y) + Square(A.z) + Square(A.w));

    return(result);
}

internal inline vec4_t
vec4_normalize(vec4_t A)
{
    vec4_t result;
    float32 magnitude     = vec4_length(A);
    __m128  magnitude_reg = _mm_set_ps1(magnitude);

    result.SSE = _mm_div_ps(A.SSE, magnitude_reg);
    return(result);
}

internal inline float32
vec4_dot(vec4_t A, vec4_t B)
{
    float32 result;
    result = (A.x * B.x) + (A.y * B.y) + (A.z * B.z) + (A.w * B.w);

    return(result);
}

internal inline vec4_t
vec4_transform(mat4_t A, vec4_t B)
{
    vec4_t result = {};

    result.SSE = _mm_mul_ps(_mm_shuffle_ps(B.SSE, B.SSE, 0x00), A.columns[0].SSE);
    result.SSE = _mm_add_ps(result.SSE, _mm_mul_ps(_mm_shuffle_ps(B.SSE, B.SSE, 0x55), A.columns[1].SSE));
    result.SSE = _mm_add_ps(result.SSE, _mm_mul_ps(_mm_shuffle_ps(B.SSE, B.SSE, 0xaa), A.columns[2].SSE));
    result.SSE = _mm_add_ps(result.SSE, _mm_mul_ps(_mm_shuffle_ps(B.SSE, B.SSE, 0xff), A.columns[3].SSE));

    /* result.x = (A._00 * B.x) + (A._01 * B.x) + (A._02 * B.x) + (A._03 * B.x); */
    /* result.y = (A._10 * B.y) + (A._11 * B.y) + (A._12 * B.y) + (A._13 * B.y); */
    /* result.z = (A._20 * B.z) + (A._21 * B.z) + (A._22 * B.z) + (A._23 * B.z); */
    /* result.w = (A._30 * B.w) + (A._31 * B.w) + (A._32 * B.w) + (A._33 * B.w); */
    
    return(result);
}

internal inline vec4_t
vec4_lerp(vec4_t A, vec4_t B, real32 time)
{
    vec4_t result;
    result.x = f32_lerp(A.x, B.x, time);
    result.y = f32_lerp(A.y, B.y, time);
    result.z = f32_lerp(A.z, B.z, time);
    result.w = f32_lerp(A.w, B.w, time);

    return(result);
}

internal inline vec4_t
vec4_unlerp(vec4_t A, vec4_t B, vec4_t X)
{
    vec4_t result;
    result.x = f32_unlerp(A.x, B.x, X.x);
    result.y = f32_unlerp(A.y, B.y, X.y);
    result.z = f32_unlerp(A.z, B.z, X.z);
    result.w = f32_unlerp(A.w, B.w, X.w);

    return(result);
}



/*===========================================
  ================ IVECTOR 2 ================
  ===========================================*/
internal inline ivec2_t
ivec2()
{
    ivec2_t result = {};
    return(result);
}

internal ivec2_t
ivec2_create(s32 A)
{
    ivec2_t result;
    result.x = A;
    result.y = A;

    return(result);
}

internal ivec2_t
ivec2_create_int32(s32 A, s32 B)
{
    ivec2_t result;
    result.x = A;
    result.y = B;

    return(result);
}

internal inline ivec3_t
ivec2_expand_ivec3(ivec2_t A, s32 B)
{
    ivec3_t result;
    result.x = A.x;
    result.y = A.y;
    result.z = B;

    return(result);
}

internal inline ivec4_t
ivec2_expand_ivec4(ivec2_t A, s32 B, s32 C)
{
    ivec4_t result;
    result.x = A.x;
    result.y = A.y;
    result.z = B;
    result.w = C;

    return(result);
}

internal inline ivec2_t
ivec2_cast(vec2_t A)
{
    ivec2_t result;
    result.x = (s32)A.x;
    result.y = (s32)A.y;

    return(result);
}

internal inline ivec2_t
ivec2_multiply(ivec2_t A, ivec2_t B)
{
    ivec2_t result;
    result.x = A.x * B.x;
    result.y = A.y * B.y;

    return(result);
}

/*===========================================
  ================ IVECTOR 3 ================
  ===========================================*/
internal inline ivec3_t
ivec3()
{
    ivec3_t result = {};
    return(result);
}

internal ivec3_t
ivec3_create(s32 A)
{
    ivec3_t result;
    result.x = A;
    result.y = A;
    result.z = A;

    return(result);
}

internal ivec3_t
ivec3_create_int32(s32 A, s32 B, s32 C)
{
    ivec3_t result;
    result.x = A;
    result.y = B;
    result.z = C;

    return(result);
}

internal inline ivec3_t
ivec3_cast(vec3_t A)
{
    ivec3_t result;
    result.x = (s32) A.x;
    result.y = (s32) A.y;
    result.z = (s32) A.z;

    return(result);
}

internal inline ivec3_t
ivec3_multiply(ivec3_t A, ivec3_t B)
{
    ivec3_t result;
    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;

    return(result);
}


/*===========================================
  ================ IVECTOR 4 ================
  ===========================================*/
internal inline ivec4_t
ivec4()
{
    ivec4_t result = {};
    return(result);
}

internal inline ivec4_t
ivec4_create(s32 A)
{
    ivec4_t result;
    result.SSE = _mm_set1_epi32(A);

    return(result);
}

internal inline ivec4_t
ivec4_create_int32(s32 A, s32 B, s32 C, s32 D)
{
    ivec4_t result;
    result.SSE = _mm_set_epi32(A, B, C, D);

    return(result);
}

internal inline vec4_t
vec4_cast(ivec4_t A)
{
    vec4_t result;
    result.x = (float32)A.x;
    result.y = (float32)A.y;
    result.z = (float32)A.z;
    result.w = (float32)A.w;

    return(result);
}

internal inline ivec4_t
ivec4_cast(vec4_t A)
{
    ivec4_t result;
    result.x = (s32)A.x;
    result.y = (s32)A.y;
    result.z = (s32)A.z;
    result.w = (s32)A.w;

    return(result);
}

internal inline ivec4_t
ivec4_multiply(ivec4_t A, ivec4_t B)
{
    ivec4_t result;
    
    result.x = A.x * B.x;
    result.y = A.y * B.y;
    result.z = A.z * B.z;
    result.w = A.w * B.w;

    return(result);
}


/*===========================================
  ================ MATRIX 2 =================
  ===========================================*/

internal inline mat2_t
mat2_identity(void)
{
    mat2_t result = {};
    result._00 = 1.0f;
    result._11 = 1.0f;

    return(result);
}

internal inline mat2_t
mat2_set_identity(float32 value)
{
    mat2_t result = {};
    result._00 = value;
    result._11 = value;

    return(result);
}

internal inline mat2_t
mat2_transpose(mat2_t A)
{
    mat2_t result = {};
    result._01 = A._10;
    result._10 = A._01;

    return(result);
}

internal inline mat2_t
mat2_add(mat2_t A, mat2_t B)
{
    mat2_t result;

    result._00 = A._00 + B._00;
    result._10 = A._10 + B._10;
    result._01 = A._01 + B._01;
    result._11 = A._11 + B._11;

    return(result);
}

internal inline mat2_t
mat2_subtract(mat2_t A, mat2_t B)
{
    mat2_t result;

    result._00 = A._00 - B._00;
    result._10 = A._10 - B._10;
    result._01 = A._01 - B._01;
    result._11 = A._11 - B._11;

    return(result);
}

internal inline mat2_t
mat2_multiply(mat2_t A, mat2_t B)
{
    mat2_t result;

    result._00 = A._00 * B._00;
    result._10 = A._10 * B._10;
    result._01 = A._01 * B._01;
    result._11 = A._11 * B._11;

    return(result);
}

internal inline mat2_t
mat2_divide(mat2_t A, mat2_t B)
{
    mat2_t result;

    result._00 = A._00 / B._00;
    result._10 = A._10 / B._10;
    result._01 = A._01 / B._01;
    result._11 = A._11 / B._11;

    return(result);
}

internal inline mat2_t
mat2_reduce(mat2_t A, vec2_t B)
{
    mat2_t result;
    result._00 = A._00 / B.x;
    result._01 = A._10 / B.x;

    result._10 = A._00 / B.y;
    result._11 = A._01 / B.y;

    return(result);
}

internal inline mat2_t
mat2_scale(mat2_t A, vec2_t B)
{
    mat2_t result;
    result._00 = A._00 * B.x;
    result._01 = A._10 * B.x;

    result._10 = A._00 * B.y;
    result._11 = A._01 * B.y;

    return(result);
}

#ifndef SL_MATH_USE_DEGREES
internal inline mat2_t
mat2_rotate(mat2_t A, float32 B)
{
    mat2_t result;
    float32 cos_angle = cosf(-B);
    float32 sin_angle = sinf(-B);

    mat2_t rotation_matrix;
    rotation_matrix._00 =  cos_angle;
    rotation_matrix._01 = -sin_angle;
    rotation_matrix._10 =  sin_angle;
    rotation_matrix._11 =  cos_angle;
    
    result = mat2_multiply(A, rotation_matrix);

    return(result);
}

#else

internal inline mat2_t
mat2_rotate(mat2_t A, float32 B)
{
    mat2_t result;
    float32 cos_angle = cosf(deg_to_rad(-B));
    float32 sin_angle = sinf(deg_to_rad(-B));

    mat2_t rotation_matrix;
    rotation_matrix._00 =  cos_angle;
    rotation_matrix._01 = -sin_angle;
    rotation_matrix._10 =  sin_angle;
    rotation_matrix._11 =  cos_angle;
    
    result = mat2_mul_mat2(A, rotation_matrix);

    return(result);
}
#endif

internal inline float32
mat2_determinant(mat2_t A)
{
    float32 result;
    result = (A.elements[0][0] * A.elements[0][1]) - (A.elements[0][1] * A.elements[1][0]);

    return(result);
}

internal inline mat2_t 
mat2_inverse(mat2_t A)
{
    mat2_t result;
    float32 inverse_determinate = 1.0f / mat2_determinant(A);

    result.elements[0][0] = inverse_determinate * +A.elements[1][1];
    result.elements[1][1] = inverse_determinate * +A.elements[0][0];
    result.elements[0][1] = inverse_determinate * -A.elements[0][1];
    result.elements[1][0] = inverse_determinate * -A.elements[1][0];

    return(result);
}


/*===========================================
  ================ MATRIX 3 =================
  ===========================================*/

internal inline mat3_t
mat3_identity(void)
{
    mat3_t result = {};
    result._00 = 1.0f;
    result._11 = 1.0f;
    result._22 = 1.0f;

    return(result);
}

internal inline mat3_t
mat3_set_identity(float32 value)
{
    mat3_t result = {};
    result._00 = value;
    result._11 = value;
    result._22 = value;

    return(result);
}

internal inline mat3_t
mat3_add(mat3_t A, mat3_t B)
{
    mat3_t result;
    result._00 = A._00 + B._00;
    result._01 = A._01 + B._01;
    result._02 = A._02 + B._02;

    result._10 = A._10 + B._10;
    result._11 = A._11 + B._11;
    result._12 = A._12 + B._12;

    result._20 = A._20 + B._20;
    result._21 = A._21 + B._21;
    result._22 = A._22 + B._22;

    return(result);
}

internal inline mat3_t
mat3_subtract(mat3_t A, mat3_t B)
{
    mat3_t result;
    result._00 = A._00 - B._00;
    result._01 = A._01 - B._01;
    result._02 = A._02 - B._02;

    result._10 = A._10 - B._10;
    result._11 = A._11 - B._11;
    result._12 = A._12 - B._12;

    result._20 = A._20 - B._20;
    result._21 = A._21 - B._21;
    result._22 = A._22 - B._22;

    return(result);
}

internal mat3_t
mat3_multiply(mat3_t A, mat3_t B)
{
    mat3_t result;
    result._00 = A._00 * B._00;
    result._01 = A._01 * B._01;
    result._02 = A._02 * B._02;

    result._10 = A._10 * B._10;
    result._11 = A._11 * B._11;
    result._12 = A._12 * B._12;

    result._20 = A._20 * B._20;
    result._21 = A._21 * B._21;
    result._22 = A._22 * B._22;

    return(result);
}

internal mat3_t
mat3_divide(mat3_t A, mat3_t B)
{
    mat3_t result;
    result._00 = A._00 / B._00;
    result._01 = A._01 / B._01;
    result._02 = A._02 / B._02;

    result._10 = A._10 / B._10;
    result._11 = A._11 / B._11;
    result._12 = A._12 / B._12;

    result._20 = A._20 / B._20;
    result._21 = A._21 / B._21;
    result._22 = A._22 / B._22;

    return(result);
}

internal mat3_t
mat3_reduce(mat3_t A, float32 B)
{
    mat3_t result;
    result._00 = A._00 / B;
    result._01 = A._01 / B;
    result._02 = A._02 / B;

    result._10 = A._10 / B;
    result._11 = A._11 / B;
    result._12 = A._12 / B;

    result._20 = A._20 / B;
    result._21 = A._21 / B;
    result._22 = A._22 / B;

    return(result);
}

internal mat3_t
mat3_make_translation(vec3_t translation)
{
    mat3_t result = mat3_identity();
    result.elements[0][0] = translation.x;
    result.elements[1][1] = translation.y;
    result.elements[2][2] = translation.z;

    return(result);
}

internal inline mat3_t
mat3_translate(mat3_t A, vec3_t translation)
{
    return(mat3_multiply(A, mat3_make_translation(translation)));
}

internal inline mat3_t
mat3_make_scale(vec3_t scale)
{
    mat3_t result = {};
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;

    return(result);
}

internal inline mat3_t
mat3_scale(mat3_t A, vec3_t scale)
{
    return(mat3_multiply(A, mat3_make_scale(scale)));
}

internal mat3_t
mat3_make_rotation(vec3_t axis, float32 rotation)
{
    mat3_t result = mat3_identity();

    axis = vec3_normalize(axis);

    float32 sin_theta = sinf(-rotation);
    float32 cos_theta = sinf(-rotation);
    float32 cos_value = 1.0f - cos_theta;

    result.elements[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
    result.elements[0][1] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
    result.elements[0][2] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);

    result.elements[1][0] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
    result.elements[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
    result.elements[1][2] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);

    result.elements[2][0] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
    result.elements[2][1] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
    result.elements[2][2] = (axis.z * axis.z * cos_value) + cos_theta;

    return(result);
}

internal inline mat3_t
mat3_rotate(mat3_t A, vec3_t axis, float32 rotation)
{
    return(mat3_multiply(A, mat3_make_rotation(axis, rotation)));
}

internal inline mat3_t
mat3_transpose(mat3_t A)
{
    mat3_t result = A;
    result.elements[0][1] = A.elements[1][0];
    result.elements[0][2] = A.elements[2][0];
    result.elements[1][0] = A.elements[0][1];
    result.elements[1][2] = A.elements[2][1];
    result.elements[2][1] = A.elements[1][2];
    result.elements[2][0] = A.elements[0][2];

    return(result);
}

internal inline mat3_t
mat3_invert(mat3_t A)
{
    mat3_t result;
    result.columns[0] = vec3_cross(A.columns[1], A.columns[2]);
    result.columns[1] = vec3_cross(A.columns[2], A.columns[0]);
    result.columns[2] = vec3_cross(A.columns[0], A.columns[1]);

    float inverse_determinant = 1.0f / vec3_dot(result.columns[2], A.columns[2]);
    result.columns[0] = vec3_scale(result.columns[0], inverse_determinant);
    result.columns[1] = vec3_scale(result.columns[1], inverse_determinant);
    result.columns[2] = vec3_scale(result.columns[2], inverse_determinant);

    return(mat3_transpose(result));
}

internal inline float32 
mat3_determinant(mat3_t A)
{
    float32 result;

    mat3_t cross;
    cross.columns[0] = vec3_cross(A.columns[1], A.columns[2]);
    cross.columns[1] = vec3_cross(A.columns[2], A.columns[0]);
    cross.columns[2] = vec3_cross(A.columns[0], A.columns[1]);

    result = vec3_dot(cross.columns[2], A.columns[2]);

    return(result);
}

internal inline float32 
mat3_inverse_determinant(mat3_t A)
{
    return(1.0f / mat3_determinant(A));
}


/*===========================================
  ================ MATRIX 4 =================
  ===========================================*/

internal inline mat4_t
mat4_identity(void)
{
    mat4_t result = {};
    result._00 = 1.0f;
    result._11 = 1.0f;
    result._22 = 1.0f;
    result._33 = 1.0f;

    return(result);
}

internal inline mat4_t
mat4_set_identity(float32 value)
{
    mat4_t result = {};
    result._00 = value;
    result._11 = value;
    result._22 = value;
    result._33 = value;

    return(result);
}

internal inline mat4_t
mat4_add(mat4_t A, mat4_t B)
{
    mat4_t result;
    result.columns[0].SSE = _mm_add_ps(A.columns[0].SSE, B.columns[0].SSE);
    result.columns[1].SSE = _mm_add_ps(A.columns[1].SSE, B.columns[1].SSE);
    result.columns[2].SSE = _mm_add_ps(A.columns[2].SSE, B.columns[2].SSE);
    result.columns[3].SSE = _mm_add_ps(A.columns[3].SSE, B.columns[3].SSE);

    return(result);
}

internal inline mat4_t
mat4_subtract(mat4_t A, mat4_t B)
{
    mat4_t result;
    result.columns[0].SSE = _mm_sub_ps(A.columns[0].SSE, B.columns[0].SSE);
    result.columns[1].SSE = _mm_sub_ps(A.columns[1].SSE, B.columns[1].SSE);
    result.columns[2].SSE = _mm_sub_ps(A.columns[2].SSE, B.columns[2].SSE);
    result.columns[3].SSE = _mm_sub_ps(A.columns[3].SSE, B.columns[3].SSE);

    return(result);
}

internal inline mat4_t
mat4_multiply(mat4_t A, mat4_t B)
{
    mat4_t result;

    // TODO(Sleepster): Optimize this... 
    result.columns[0] = vec4_transform(A, B.columns[0]);
    result.columns[1] = vec4_transform(A, B.columns[1]);
    result.columns[2] = vec4_transform(A, B.columns[2]);
    result.columns[3] = vec4_transform(A, B.columns[3]);

    return(result);
}

internal inline mat4_t
mat4_divide(mat4_t A, mat4_t B)
{
    mat4_t result;
    
    result.columns[0].SSE = _mm_div_ps(A.columns[0].SSE, B.columns[0].SSE);
    result.columns[1].SSE = _mm_div_ps(A.columns[1].SSE, B.columns[1].SSE);
    result.columns[2].SSE = _mm_div_ps(A.columns[2].SSE, B.columns[2].SSE);
    result.columns[3].SSE = _mm_div_ps(A.columns[3].SSE, B.columns[3].SSE);

    return(result);
}

internal inline mat4_t
mat4_reduce(mat4_t A, float32 B)
{
    mat4_t result;
    __m128 scaler_vector = _mm_set_ps1(B);

    result.columns[0].SSE = _mm_div_ps(A.columns[0].SSE, scaler_vector);
    result.columns[1].SSE = _mm_div_ps(A.columns[1].SSE, scaler_vector);
    result.columns[2].SSE = _mm_div_ps(A.columns[2].SSE, scaler_vector);
    result.columns[3].SSE = _mm_div_ps(A.columns[3].SSE, scaler_vector);

    return(result);
}

internal inline mat4_t
mat4_make_translation(vec3_t translation)
{
    mat4_t result = mat4_identity();
    result.elements[3][0] = translation.x;
    result.elements[3][1] = translation.y;
    result.elements[3][2] = translation.z;
    
    return(result);
}

internal inline mat4_t
mat4_translate(mat4_t A, vec3_t B)
{
    return(mat4_multiply(A, mat4_make_translation(B)));
}

internal inline mat4_t
mat4_make_scale(vec3_t scale)
{
    mat4_t result = mat4_identity();
    result.elements[0][0] = scale.x;
    result.elements[1][1] = scale.y;
    result.elements[2][2] = scale.z;

    return(result);
}

internal inline mat4_t
mat4_scale(mat4_t A, vec3_t B)
{
    return(mat4_multiply(A, mat4_make_scale(B)));
}

internal mat4_t
mat4_make_rotation(vec3_t axis, float32 rotation)
{
    mat4_t result = mat4_identity();

    axis = vec3_normalize(axis);

    float32 sin_theta = sinf(-rotation);
    float32 cos_theta = sinf(-rotation);
    float32 cos_value = 1.0f - cos_theta;

    result.elements[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
    result.elements[0][1] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
    result.elements[0][2] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);

    result.elements[1][0] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
    result.elements[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
    result.elements[1][2] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);

    result.elements[2][0] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
    result.elements[2][1] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
    result.elements[2][2] = (axis.z * axis.z * cos_value) + cos_theta;

    return(result);
}

internal inline mat4_t
mat4_rotate(mat4_t A, vec3_t axis, float32 rotation)
{
    return(mat4_multiply(A, mat4_make_rotation(axis, rotation)));
}

internal inline mat4_t
mat4_transpose(mat4_t A)
{
    mat4_t result = A;
    _MM_TRANSPOSE4_PS(result.columns[0].SSE, result.columns[1].SSE, result.columns[2].SSE, result.columns[3].SSE);

    return(result);
}

internal inline mat4_t
mat4_invert(mat4_t A)
{
    mat4_t result;

    vec3_t column_cross01 = vec3_cross(A.columns[0].xyz, A.columns[1].xyz);
    vec3_t column_cross23 = vec3_cross(A.columns[2].xyz, A.columns[3].xyz);

    vec3_t quat_scale10 = vec3_subtract(vec3_scale(A.columns[0].xyz, A.columns[1].w), vec3_scale(A.columns[1].xyz, A.columns[0].w)); 
    vec3_t quat_scale23 = vec3_subtract(vec3_scale(A.columns[2].xyz, A.columns[3].w), vec3_scale(A.columns[3].xyz, A.columns[2].w));

    float32 inverse_determinant = 1.0f / (vec3_dot(column_cross01, quat_scale23) + vec3_dot(column_cross23, quat_scale10));

    column_cross01 = vec3_scale(column_cross01, inverse_determinant);
    column_cross23 = vec3_scale(column_cross23, inverse_determinant);
    quat_scale10   = vec3_scale(quat_scale10, inverse_determinant);
    quat_scale23   = vec3_scale(quat_scale23, inverse_determinant);

    result.columns[0] = vec3_expand_vec4(vec3_add(vec3_cross(A.columns[1].xyz, quat_scale23), vec3_scale(column_cross23, A.columns[1].w)), -vec3_dot(A.columns[1].xyz, column_cross23));
    result.columns[1] = vec3_expand_vec4(vec3_subtract(vec3_cross(quat_scale23, A.columns[0].xyz), vec3_scale(column_cross23, A.columns[0].w)), +vec3_dot(A.columns[0].xyz, column_cross23));
    result.columns[2] = vec3_expand_vec4(vec3_add(vec3_cross(A.columns[3].xyz, quat_scale10), vec3_scale(column_cross01, A.columns[3].w)), -vec3_dot(A.columns[3].xyz, column_cross01));
    result.columns[3] = vec3_expand_vec4(vec3_subtract(vec3_cross(quat_scale10, A.columns[2].xyz), vec3_scale(column_cross01, A.columns[2].w)), +vec3_dot(A.columns[2].xyz, column_cross01));
    
    return(mat4_transpose(result));
}

internal inline float32
mat4_determinant(mat4_t A)
{
    float32 result = 0.0f;

    vec3_t column_cross01 = vec3_cross(A.columns[0].xyz, A.columns[1].xyz);
    vec3_t column_cross23 = vec3_cross(A.columns[2].xyz, A.columns[3].xyz);

    vec3_t quat_scale10 = vec3_subtract(vec3_scale(A.columns[0].xyz, A.columns[1].w), vec3_scale(A.columns[1].xyz, A.columns[0].w));
    vec3_t quat_scale23 = vec3_subtract(vec3_scale(A.columns[2].xyz, A.columns[3].w), vec3_scale(A.columns[3].xyz, A.columns[2].w));

    result = vec3_dot(column_cross01, quat_scale23) + vec3_dot(column_cross23, quat_scale10);
    
    return(result);
}

internal inline float32
mat4_inverse_determinant(mat4_t A)
{
    return(1.0f / mat4_determinant(A));
}

/*===========================================
  =========== GRAPHICS TRANSFORMS ===========
  ===========================================*/

// NOTE(Sleepster): Near and far plane are -1 - 1 OpenGL standard.
internal inline mat4_t
mat4_RHGL_ortho(float32 left,
                float32 right,
                float32 bottom,
                float32 top,
                float32 near,
                float32 far)
{
    mat4_t result = mat4_identity();

    result.elements[0][0] = 2.0f / (right - left);
    result.elements[1][1] = 2.0f / (top - bottom);
    result.elements[2][2] = 2.0f / (near - far);
    result.elements[3][3] = 1.0f;

    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (near + far) / (near - far);

    return(result);
}

// NOTE(Sleepster): Near and far plane are 0 - 1 DirectX standard.
internal inline mat4_t
mat4_RHDX_ortho(float32 left,
                float32 right,
                float32 bottom,
                float32 top,
                float32 near,
                float32 far)
{
    mat4_t result = mat4_identity();

    result.elements[0][0] = 2.0f / (right - left);
    result.elements[1][1] = 2.0f / (top - bottom);
    result.elements[2][2] = 2.0f / (near - far);
    result.elements[3][3] = 1.0f;

    result.elements[3][0] = (left + right) / (left - right);
    result.elements[3][1] = (bottom + top) / (bottom - top);
    result.elements[3][2] = (near + far) / (near - far);

    return(result);
}


internal inline mat4_t
mat4_LHGL_ortho(float32 left,
                float32 right,
                float32 bottom,
                float32 top,
                float32 near,
                float32 far)
{
    mat4_t result = mat4_RHGL_ortho(left, right, bottom, top, near, far);
    result.elements[2][2] = -result.elements[2][2];

    return(result);
}

internal inline mat4_t
mat4_LHDX_ortho(float32 left,
                float32 right,
                float32 bottom,
                float32 top,
                float32 near,
                float32 far)
{
    mat4_t result = mat4_RHDX_ortho(left, right, bottom, top, near, far);
    result.elements[2][2] = -result.elements[2][2];

    return(result);
}

internal inline mat4_t
mat4_inverse_ortho(mat4_t orthographic_projection)
{
    mat4_t result = mat4_identity();
    result.elements[0][0] = 1.0f / orthographic_projection.elements[0][0];
    result.elements[1][1] = 1.0f / orthographic_projection.elements[1][1];
    result.elements[2][2] = 1.0f / orthographic_projection.elements[2][2];
    result.elements[3][3] = 1.0f;

    result.elements[3][0] = -orthographic_projection.elements[3][0] * result.elements[0][0];
    result.elements[3][1] = -orthographic_projection.elements[3][1] * result.elements[1][1];
    result.elements[3][2] = -orthographic_projection.elements[3][2] * result.elements[2][2];

    return(result);
}

/*==============================================
  ================= RECTANGLES =================
  ==============================================*/

typedef struct rectangle2
{
    vec2_t min;
    vec2_t max;
    vec2_t center;
    vec2_t half_size;
}rectangle2_t;

internal rectangle2_t
rect2_create(vec2_t position, vec2_t size)
{
    rectangle2_t result;
    result.min       = position;
    result.max       = vec2_add(position, size);
    result.half_size = vec2_scale(size, 0.5f);
    result.center    = vec2_add(position, result.half_size);

    return(result);
}

internal void 
rect2_shift_by(rectangle2_t *rect, vec2_t shift)
{
    rect->min    = vec2_add(rect->min, shift);
    rect->max    = vec2_add(rect->max, shift);
    rect->center = vec2_add(rect->center, shift);
}

internal vec2_t
rect2_get_size(rectangle2_t rect)
{
    vec2_t result = vec2_scale(rect.half_size, 2.0f);

    return(result);
}

internal inline vec2_t
rect2_get_position(rectangle2_t rect)
{
    return(rect.min);
}

internal bool8
rect2_vec2_SAT(rectangle2_t rect, vec2_t point)
{
    return (point.x >= rect.min.x && point.x <= rect.max.x && 
            point.y >= rect.min.y && point.y <= rect.max.y);
}

internal bool8
rect2_AABB_SAT(rectangle2_t A, rectangle2_t B)
{
    return (A.min.x <= B.max.x && A.max.x >= B.min.x &&
            A.min.y <= B.max.y && A.max.y >= B.min.y);
}

internal rectangle2_t 
rect2_minkowski_sum(rectangle2_t A, rectangle2_t B)
{
    rectangle2_t result;

    result.min       = vec2_add(A.min, B.min);
    result.max       = vec2_add(A.max, B.max);
    result.half_size = vec2_scale(vec2_subtract(result.max, result.min), 0.5f);
    result.center    = vec2_add(result.min, result.half_size);

    return(result);
}

internal rectangle2_t
rect2_minkowski_difference(rectangle2_t A, rectangle2_t B)
{
    rectangle2_t result;

    result.min       = vec2_subtract(A.min, B.max);
    result.max       = vec2_subtract(A.max, B.min);
    result.half_size = vec2_scale(vec2_subtract(result.max, result.min), 0.5f);
    result.center    = vec2_add(result.min, result.half_size);

    return(result);
}

typedef struct raytest 
{
    bool32  hit;
    float32 time;
    vec2_t  position;
    vec2_t  normal;
}raytest_t;

internal raytest_t 
rect2_ray_test(vec2_t position, vec2_t magnitude, rectangle2_t bounding_box) 
{
    raytest_t result = {};

    float32 last_entry = -INFINITY;
    float32 first_exit =  INFINITY;

    for(u32 axis = 0;
        axis < 2;
        ++axis)
    {
        if(magnitude.elements[axis] != 0.0f)
        {
            float32 time0 = (bounding_box.min.elements[axis] - position.elements[axis]) / magnitude.elements[axis];
            float32 time1 = (bounding_box.max.elements[axis] - position.elements[axis]) / magnitude.elements[axis];

            last_entry = Max(last_entry, Min(time0, time1));
            first_exit = Min(first_exit, Max(time0, time1));
        }
        else if(position.elements[axis] <= bounding_box.min.elements[axis] || 
                position.elements[axis] >= bounding_box.max.elements[axis])
        {
            return(result);
        }
    }

    if(first_exit > last_entry && first_exit > 0.0f && last_entry < 1.0f)
    {
        result.hit      = true;
        result.time     = last_entry;
        result.position = vec2(position.x + (magnitude.x * last_entry),
                               position.y + (magnitude.y * last_entry));

        float32 dcenter_x = result.position.x - bounding_box.center.x;
        float32 dcenter_y = result.position.y - bounding_box.center.y;

        float32 overlap_x = bounding_box.half_size.x - Abs(dcenter_x);
        float32 overlap_y = bounding_box.half_size.y - Abs(dcenter_y);
        if(overlap_x < overlap_y)
        {
            result.normal.x = (dcenter_x > 0) - (dcenter_x < 0);
        }
        else
        {
            result.normal.y = (dcenter_y > 0) - (dcenter_y < 0);
        }
    }

    return(result);
}

internal raytest_t 
rect2_sweep_test(rectangle2_t moving_rect, vec2_t velocity, rectangle2_t static_rect)
{
    raytest_t result;
    rectangle2_t expanded_box;
    expanded_box.min       = vec2_subtract(static_rect.min, moving_rect.half_size);
    expanded_box.max       = vec2_add(static_rect.max, moving_rect.half_size);
    expanded_box.center    = vec2_scale(vec2_add(expanded_box.min, expanded_box.max), 0.5f);
    expanded_box.half_size = vec2_scale(vec2_subtract(expanded_box.max, expanded_box.min), 0.5f);

    result = rect2_ray_test(moving_rect.center, velocity, expanded_box);

    return(result);
}

internal vec2_t
rect2_get_vector_depth(rectangle2_t rect)
{
    vec2_t result;

    float32 min_distance = Abs(rect.min.x);
    result.x = rect.min.x;
    result.y = 0.0f;

    if (Abs(rect.max.x) < min_distance)
    {
        min_distance = Abs(rect.max.x);
        result.x = rect.max.x;
        result.y = 0.0f;
    }

    if (Abs(rect.min.y) < min_distance)
    {
        min_distance = Abs(rect.min.y);
        result.x = 0.0f;
        result.y = rect.min.y;
    }

    if (Abs(rect.max.y) < min_distance)
    {
        result.x = 0.0f;
        result.y = rect.max.y;
    }

    return result;
}
#endif
