#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef USE_SSE
#include <nmmintrin.h>
#include <immintrin.h>

typedef __m128 m128;
typedef __m128i m128i;

#define PI 3.14

#define set_m128i(a0, a1, a2, a3) _mm_set_epi32(a3, a2, a1, a0)
//#define set_m128i(a0, a1, a2, a3) _mm_set_epi32(a0, a1, a2, a3)
#define mul_m128i(a, b) _mm_mullo_epi32(a, b)
#define add_m128i(a, b) _mm_add_epi32(a, b)
#define or_m128i(a, b) _mm_or_si128(a, b)
#define and_m128i(a, b) _mm_and_si128(a, b)
#define broadcast_m128i(a) _mm_set1_epi32(a)
#define blend_m128i(a, b, conditional) _mm_blendv_epi8(a, b, conditional)
#define move_m128i(a, b, conditional) _mm_maskmoveu_si128(a, b, conditional)
#define compare_less_than_m128i(a, b) _mm_cmplt_epi32(a, b)
#define compare_greater_than_m128i(a, b) _mm_cmpgt_epi32(a, b)
#define compare_equal_m128i(a, b) _mm_cmpeq_epi32(a, b)
#define convert_to_m128i(a) _mm_cvtps_epi32(a)
#define sub_m128i(a, b) _mm_sub_epi32(a, b)
#define cast_to_m128i(a) _mm_castps_si128(a)

#define mul_m128(a, b) _mm_mul_ps(a, b)
#define add_m128(a, b) _mm_add_ps(a, b)
#define broadcast_m128(a) _mm_set1_ps(a)
#define compare_greater_than_m128(a, b) _mm_cmpgt_ps(a, b)
#define compare_less_than_m128(a, b) _mm_cmplt_ps(a, b)
#define convert_to_m128(a) _mm_cvtepi32_ps(a)
#define div_m128(a, b) _mm_div_ps(a, b)
#define load_m128(memory) _mm_load_ps(memory)
#define compare_less_than_or_equal_m128(a, b) _mm_cmple_ps(a, b)
#define and_m128(a, b) _mm_and_ps(a, b)
#define cast_to_m128(a) _mm_castsi128_ps(a)


m128i dot_product_m128i(m128i a0, m128i a1, m128i a2, m128i b0, m128i b1, m128i b2) {
    return add_m128i(add_m128i(mul_m128i(a0, b0), mul_m128i(a1, b1)), mul_m128i(a2, b2));
}

m128 dot_product_m128(m128 a0, m128 a1, m128 a2, m128 b0, m128 b1, m128 b2) {
    return add_m128(add_m128(mul_m128(a0, b0), mul_m128(a1, b1)), mul_m128(a2, b2));
}


#endif

#if 0
//TODO: finish creating a non-sse path
typedef struct m128 {
    float e[4];
} m128;

typedef struct m128i {
    int e[4];
} m128i;

#define mul_m128i(a, b) multiply_4x_int(a, b)
#define add_m128i(a, b) add_4x_int(a, b)
#define or_m128i(a, b) or_4x_int(a, b)
#define and_m128i(a, b) and_4x_int(a, b)
#define broadcast_m128i(a) broadcast_4x_int(a)
#define blend_m128i(a, b, conditional) blend_4x_int(a)
#define move_m128i(a, b, conditional) move_4x_int(a)
#define compare_less_than_m128i(a, b) compare_less_than_4x_int(a, b)
#define compare_greater_than_m128i(a, b) compare_greater_than_4x_int(a, b)
#define compare_equal_m128i(a, b) compare_equal_4x_int(a, b)
#define convert_to_m128i(a) convert_to_4x_int(a)
#define sub_m128i(a, b) sub_4x_int(a, b)
#define cast_to_m128i(a) cast_to_4x_int(a)

#define add_m128(a, b) add_4x_float(a, b)
#define mul_m128(a, b) multiply_4x_float(a, b)
#define broadcast_m128(a) 
#define compare_greater_than_m128(a, b) 
#define compare_less_than_m128(a, b)
#define convert_to_m128(a) 
#define div_m128(a, b) 
#define load_m128(memory) 
#define compare_less_than_or_equal_m128(a, b) 
#define and_m128(a, b) 
#define cast_to_m128(a) 

m128i multiply_4x_int(m128i a, m128i b) {
    m128i c = {0};
    for (int i = 0; i < 4; ++i) {
        c.e[i] = a.e[i] * b.e[i];
    }
    
    return c;
}

m128i add_4x_int(m128i a, m128i b) {
    m128i c = {0};
    for (int i = 0; i < 4; ++i) {
        c.e[i] = a.e[i] + b.e[i];
    }
    
    return c;
}

m128 add_4x_float(m128 a, m128 b) {
    m128 c = {0};
    for (int i = 0; i < 4; ++i) {
        c.e[i] = a.e[i] + b.e[i];
    }
    
    return c;
}

m128 multiply_4x_float(m128 a, m128 b) {
    m128 c = {0};
    for (int i = 0; i < 4; ++i) {
        c.e[i] = a.e[i] * b.e[i];
    }
    
    return c;
}

#endif


//TODO: fma + avx2 instructions

typedef union v2i {
    struct { s32 x, y;  };
    struct { s32 u, v;  };
    s32 e[2];
} v2i;

typedef union v2 {
    struct { float x, y; };
    struct { float u, v; };
    struct { float c, r; };
    float e[2];
} v2;

typedef union v3 {
    struct { float x, y, z; };
    struct { float r, g, b; };
    float e[3];
} v3;


typedef union  v4 {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    float e[4];
} v4;


typedef struct m4 {
    union {
        float e[4][4];
        mat4 mat;
    };
} m4;

v2 get_v2(float x, float y) {
    return (v2){.x =x, .y =y};
}



v4 get_v4(float x, float y, float z, float w) {
    return (v4){.x =x, .y =y, .z = z, .w = w};
}

v3 get_v3_from_v4(v4 vec) {
    v3 result = {
        .x = vec.x,
        .y = vec.y,
        .z = vec.z
    };
    
    return result;
}

v4 mul_m4_by_v4(m4 m, v4 vec) {
    v4 result = {0};
#if 0
    for (int i = 0; i < 4; ++i) {
        result.e[i] += m.e[i][0] * vec.x;
        result.e[i] += m.e[i][1] * vec.y;
        result.e[i] += m.e[i][2] * vec.z;
        result.e[i] += m.e[i][3] * vec.w;
    }
#else
    glm_mat4_mulv(m.e, vec.e, result.e);
#endif
    return result;
}

v3 div_v3(v3 a, v3 b) {
    v3 result = { a.x / b.x, a.y / b.y, a.z / b.z };
    
    return result;
}

m4 mul_m4_by_m4(m4 a, m4 b) {
    m4 result = {0};
    
#if 0
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                result.e[i][j] += a.e[i][k] * b.e[k][j];
            }
        }
    }
#else
    glm_mat4_mul(a.e, b.e, result.e);
#endif
    return result;
}

v3 cross_v3(v3 vec1, v3 vec2) {
    v3 result = {
        .x = vec1.y * vec2.z - vec1.z * vec2.y,
        .y = vec1.z * vec2.x - vec1.x * vec2.z,
        .z = vec1.x * vec2.y - vec1.y * vec2.x
    };
    
    return result;
}

float get_v3_length(v3 vec) {
    float result = (float)sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    return result;
}

v3 normalize_v3(v3 vec) {
    float length = get_v3_length(vec);
    v3 result = {
        .x = vec.x/length,
        .y = vec.y/length,
        .z = vec.z/length
    };
    
    return result;
}

m4 math_get_identity_matrix() {
    m4 result =  {0};
    result.e[0][0] = result.e[1][1] = result.e[2][2] = result.e[3][3] = 1.0f;
    
    return result;
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}



v2 lerp_v2(v2 a, v2 b, float t) {
    float x = lerp(a.x, b.x, t);
    float y = lerp(a.y, b.y, t);
    v2 result = get_v2(x, y);
    
    return result;
}

v3 get_v3(float x, float y, float z) {
    return (v3){
        .x = x,
        .y = y,
        .z = z
    };
}

v3 lerp_v3(v3 a, v3 b, float t) {
    float x = lerp(a.x, b.x, t);
    float y = lerp(a.y, b.y, t);
    float z = lerp(a.z, b.z, t);
    v3 result = get_v3(x, y, z);
    
    return result;
}

v3 add_v3(v3 a, v3 b) {
    return get_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}


v2 add_v2(v2 a, v2 b) {
    return get_v2(a.x + b.x, a.y + b.y);
}


v3 sub_v3(v3 a, v3 b) {
    return get_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

v2 sub_v2(v2 a, v2 b) {
    return get_v2(a.x - b.x, a.y - b.y);
}

v2 get_v2_from_v4(v4 v) {
    return get_v2(v.x, v.y);
}

v3 get_v3_from_array(float *array) {
    v3 v = {0};
    v.e[0] = array[0];
    v.e[1] = array[1];
    v.e[2] = array[2];
    
    return v;
}

v2 max_v2(v2 a, v2 b) {
    if (a.x > b.x) {
        return a;
    }
    if (a.y > b.y) {
        return a;
    }
    
    return b;
}

v2 min_v2(v2 a, v2 b) {
    if (a.x < b.x) {
        return a;
    }
    if (a.y < b.y) {
        return a;
    }
    
    return b;
}

float dot_v2(v2 a, v2 b) {
    float result = a.x * b.x + a.y * b.y;
    
    return result;
}

v2 get_v2_from_v2i(v2i v) {
    return get_v2((float)v.x, (float)v.y);
}

v2i get_v2i(s32 x, s32 y) {
    return (v2i){.x =x, .y =y};
}

v2i get_v2i_from_v2(v2 v) {
    return get_v2i((s32)v.x, (s32)v.y);
}

float dot_v3(v3 a, v3 b) {
    float result = a.x * b.x +  a.y * b.y +  a.z * b.z;
    
    return result;
}

v3 hadamard_v3(v3 a, v3 b) {
    return get_v3(a.x * b.x, a.y * b.y, a.z * b.z);
}

s32 min_s32(s32 a, s32 b) {
    return a < b ? a : b;
}

s32 max_s32(s32 a, s32 b) {
    return a > b ? a : b;
}


u32 min_u32(u32 a, u32 b) {
    return a < b ? a : b;
}

u32 max_u32(u32 a, u32 b) {
    return a > b ? a : b;
}

f32 max_f32(f32 a, f32 b) {
    return a > b ? a : b;
}

v4 get_v4_from_v3(v3 v, f32 last_element) {
    return get_v4(v.e[0], v.e[1], v.e[2], last_element);
}

typedef struct Bounding_box {
    union {
        struct {
            float min_x, min_y, min_z, max_x, max_y, max_z;
        };
        struct {
            float x0, y0, z0, x1, y1, z1;
        };
        struct {
            v3 v0, v1;
        };
    };
} Bounding_box;

typedef Bounding_box Bbox;

Bounding_box get_2d_bounding_box_from_4_coords(int min_x, int min_y, int max_x, int max_y) {
    Bounding_box bounding_box = {
        .min_x = min_x,
        .min_y = min_y, 
        .max_x = max_x,
        .max_y = max_y
    };
    
    return bounding_box;
}

Bounding_box get_2d_bounding_box_from_3_v2(v2 a, v2 b, v2 c) {
    Bounding_box bounding_box;
    bounding_box.min_x = min_s32(min_s32(a.x, b.x), c.x);
    bounding_box.min_y = min_s32(min_s32(a.y, b.y), c.y);
    bounding_box.max_x = max_s32(max_s32(a.x, b.x), c.x);
    bounding_box.max_y = max_s32(max_s32(a.y, b.y), c.y);
    
    return bounding_box;
}

bool check_aabb_collision(Bounding_box a, Bounding_box b) {
    bool result = 
        a.max_x >= b.min_x &&
        a.min_x <= b.max_x &&
        a.max_y >= b.min_y &&
        a.min_y <= b.max_y &&
        a.max_z >= b.min_z &&
        a.min_z <= b.max_z;
    
    return result;
}

typedef struct Rect {
    union {
        struct {
            float x_min, y_min, x_max, y_max;
        };
        struct {
            float x0, y0, x1, y1;
        };
        struct {
            float u0, v0, u1, v1;
        };
        struct {
            v2 vec0, vec1;
        };
        struct {
            float left, right, bottom, top;
        };
    };
} Rect;

Rect create_rect(float x_min, float y_min, float x_max, float y_max) {
    Rect rect = { .x_min = x_min, .y_min = y_min, .x_max = x_max, .y_max = y_max };
    return rect;
}

float get_rect_area(Rect rect) {
    float result = (rect.x1 - rect.x0) * (rect.y1 - rect.y0);
    
    assert(result >= 0);
    
    return result;
}

bool is_point_inside_rect(Rect rect, v2 point) {
    if (point.x >= rect.x0 &&
        point.x <= rect.x1 &&
        point.y >= rect.y0 &&
        point.y <= rect.y1)
        return true;
    return false;
}

Rect get_2d_bounding_box_from_mesh(float *vertices, int num) {
    Rect rect = {0};
    for (int i = 0; i < num; ++i) {
        rect.x_min = fmin(rect.x_min, vertices[i]);
        rect.y_min = fmin(rect.y_min, vertices[i]);
        rect.x_max = fmin(rect.x_max, vertices[i]);
        rect.y_max = fmin(rect.y_max, vertices[i]);
    }
}

Bounding_box get_bounding_box_from_rect(Rect rect) {
    Bounding_box bounding_box = {
        .min_x = rect.x_min,
        .min_y = rect.y_min,
        .max_x = rect.x_max,
        .max_y = rect.y_max,
    };
    
    return bounding_box;
}

Rect get_rect_from_bbox(Bbox bbox) {
    Rect rect = { bbox.x0, bbox.y0, bbox.x1, bbox.y1 };
    
    return rect;
}

bool is_point_inside_bounding_box(Bounding_box bounding_box, v2 point) {
    if (point.x >= bounding_box.min_x &&
        point.x <= bounding_box.max_x &&
        point.y >= bounding_box.min_y && 
        point.y <= bounding_box.max_y)
        return true;
    return false;
}

v3 get_v3_from_v2(v2 v) {
    return get_v3(v.x, v.y, 0);
}

v2 get_v2_from_v3(v3 v) {
    return get_v2(v.x, v.y);
}

v3 convert_to_vulkan_coordinates(v3 v) {
    return get_v3(v.x, -v.y, v.z);
}

v3 scatter_v3(float val) {
    return get_v3(val, val, val);
}

v3 mul_v3_by_scalar(v3 v, float scalar) {
    v3 result = get_v3(v.x * scalar, v.y * scalar, v.z * scalar);
    
    return result;
}

v3 mul_v3_hadmard(v3 v0, v3 v1) {
    v3 result = get_v3(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z);
    
    return result;
}

v3 exp_v3(v3 v, float val) {
    v3 result = get_v3(pow(v.x, val), pow(v.y, val), pow(v.z, val));
    
    return result;
}

float get_rect_width(Rect rect) {
    return rect.x_max - rect.x_min;
}

float get_rect_height(Rect rect) {
    return rect.y_max - rect.y_min;
}

Rect get_maximal_rect(Rect rect1, Rect rect2) {
    Rect rect = {
        .x_min = fmin(rect1.x_min, rect2.x_min),
        .x_max = fmax(rect1.x_max, rect2.x_max),
        .y_min = fmin(rect1.y_min, rect2.y_min),
        .y_max = fmax(rect1.y_max, rect2.y_max)
    };
    
    return rect;
}

Rect move_rect(Rect rect, v2 v) {
    Rect result = {
        .x_min = rect.x_min + v.x,
        .y_min = rect.y_min + v.y,
        .x_max = rect.x_max + v.x,
        .y_max = rect.y_max + v.y,
    };
    
    return result;
}

v4 divide_v4_by_scalar(v4 v, float scalar) {
    for (int i = 0; i < 4; ++i)
        v.e[i] /= scalar;
    
    return v;
}


v3 divide_v3_by_scalar(v3 v, float scalar) {
    for (int i = 0; i < 3; ++i)
        v.e[i] /= scalar;
    
    return v;
}

void convert_from_ndc_to_screen_coords(float *x, float *y, int screen_width, int screen_height) {
    if (x) {
        float temp_x = *x;
        temp_x += 1;
        temp_x /= 2;
        temp_x *= screen_width;
        *x = temp_x;
    }
    if (y) {
        float temp_y = *y;
        temp_y += 1;
        temp_y /= 2;
        temp_y *= screen_height;
        *y = temp_y;
    }
}

v2 convert_from_screen_coords_to_ndc(v2 vec, int screen_width, int screen_height) {
    v2 result = get_v2(vec.x / screen_width * 2 - 1, vec.y / screen_height * 2 - 1);
    
    return result;
}

void convert_from_screen_coords_to_ndc_non_vulkan(float *x, float *y, int screen_width, int screen_height) {
    if (x) {
        float temp_x = *x;
#if 0
        temp_x /= screen_width;
        temp_x *= 2;
        temp_x -= 1;
        *x = temp_x;
#endif
        *x = (2.0f * temp_x / screen_width) - 1.0f;
    }
    if (y) {
        float temp_y = *y;
#if 1
        temp_y /= screen_height;
        temp_y *= 2;
        temp_y *= -1;
        temp_y += 1;
        *y = temp_y;
#else
        *y = 1.0f - (2.0f * temp_y / screen_height);
#endif
    }
}

typedef struct Transform_data {
    m4 model, view, projection;
} Transform_data;

v3 convert_from_model_coords_to_ndc(v3 v, Transform_data *transform_data) {
    mat4 vp, mvp;
    mat4 model, view, projection;
#if 1
    glm_mat4_copy(transform_data->model.e, model);
    glm_mat4_copy(transform_data->view.e, view);
    glm_perspective(glm_rad(45.0f), (float)g_screen_width/(float)g_screen_height, 0.1f, 100.0f, projection);
    glm_mat4_copy(transform_data->projection.e, projection);
#else
    glm_mat4_identity(model.e);
    glm_mat4_identity(view);
    glm_mat4_copy(transform_data->projection, projection);
#endif
    glm_mat4_mul(projection, view, vp);
    glm_mat4_mul(vp, model, mvp);
    //vec4 vec = { v.x, v.y, v.z, 1.0f };
    vec4 viewport = { 0, 0, g_screen_width, g_screen_height };
    vec4 vec = { v.x, v.y, -1.0f, 1.0f };
    vec4 pos4;
    vec4 dest;
    vec4 vone = GLM_VEC4_ONE_INIT;
    glm_mat4_mulv(mvp, vec, pos4);
    glm_vec4_scale(pos4, 1.0f / pos4[3], pos4);
    glm_vec4_add(pos4, vone, pos4);
    glm_vec4_scale(pos4, 0.5f, pos4);
    
    dest[0] = pos4[0] * viewport[2] + viewport[0];
    dest[1] = pos4[1] * viewport[3] + viewport[1];
    dest[2] = pos4[2];
    v3 result = {
        .x = dest[0],
        .y = dest[1],
        .z = dest[2],
    };
    //v3 result = get_v3(dest[0], dest[1], dest[2]);
    //result = divide_v3_by_scalar(result, dest[3]);
    
    return result;
}

void convert_from_ndc_to_screen_coords_non_vulkan(float *x, float *y, int screen_width, int screen_height) {
    if (x) {
        float temp_x = *x;
        temp_x += 1;
        temp_x /= 2;
        temp_x *= screen_width;
        *x = temp_x;
    }
    if (y) {
        float temp_y = *y;
        temp_y += 1;
        temp_y /= 2;
        temp_y *= screen_height;
        *y = temp_y;
    }
}

v3 convert_from_model_coords_to_screen(v3 v, Transform_data *transform_data) {
    convert_from_model_coords_to_ndc(v, transform_data);
    //convert_from_ndc_to_screen_coords_non_vulkan();
}

float math_convert_to_radians(float angle) {
    float result = angle * 2 * PI / 360;
    
    return result;
}

void math_copy_m4_to_mat4(m4 m, mat4 mat) {
    glm_mat4_copy(m.mat, mat);
}

m4 math_get_perspective_matrix(float fov_in_radians, float aspect_ratio, float near, float far) {
    m4 m;
    glm_perspective(fov_in_radians, aspect_ratio, near, far, m.mat);
    
    return m;
}

m4 inverse_matrix(m4 m) {
    m4 result;
    glm_mat4_inv(m.e, result.e);
    
    return result;
}

v4 mul_v4_by_scalar(v4 v, float scalar) {
    v4 result = get_v4(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
    
    return result;
}

v3 unproject_glm(Transform_data transform_data, v3 pos) {
    v3 result;
    
    m4 vp = mul_m4_by_m4(transform_data.projection, transform_data.view);
    m4 mvp = mul_m4_by_m4(vp, transform_data.model);
    glm_unproject(pos.e, vp.e, (vec4){0, 0, g_screen_width, g_screen_height}, result.e);
    
    return result;
}

v3 unproject(Transform_data transform_data, v3 pos) {
    v3 result;
    
    v2 ndc_pos = convert_from_screen_coords_to_ndc(get_v2(pos.x, pos.y), g_screen_width, g_screen_height);
    v4 temp_pos = get_v4(ndc_pos.x, ndc_pos.y, pos.z, 1);
    m4 m = transform_data.model, v = transform_data.view, p = transform_data.projection;
    m = inverse_matrix(m);
    v = inverse_matrix(v);
    p = inverse_matrix(p);
    temp_pos = mul_m4_by_v4(p, temp_pos);
    temp_pos = mul_m4_by_v4(v, temp_pos);
    //temp_pos = mul_m4_by_v4(m, temp_pos);
    //temp_pos = mul_v4_by_scalar(temp_pos, 1.0f / temp_pos.w);
    
    result = get_v3_from_v4(temp_pos);
    
    return result;
}

v3 project(Transform_data transform_data, v3 pos) {
#if 0
    mat4 vp, mvp;
    glm_mat4_mul(transform_data.projection.e, transform_data.view.e, vp);
    glm_mat4_mul(vp, transform_data.model.e, mvp);
    vec3 dest;
    glm_project(pos.e, mvp, (vec4){0, 0, g_screen_width, g_screen_height}, dest);
    v3 result = get_v3_from_array(dest);
#else
    m4 vp = mul_m4_by_m4(transform_data.projection, transform_data.view);
    m4 mvp = mul_m4_by_m4(vp, transform_data.model);
    v4 temp_result;
    glm_project(pos.e, mvp.e, (vec4){0, 0, g_screen_width, g_screen_height}, temp_result.e);
    v3 result = get_v3_from_v4(temp_result);
#endif
    return result;
}

Bounding_box get_bounding_box_world_coords(Bounding_box bbox, m4 transformation_matrix) {
    v4 v0 = get_v4(bbox.x0, bbox.y0, 0, 1);
    v4 v1 = get_v4(bbox.x1, bbox.y1, 0, 1);
    
    v4 dest0 = mul_m4_by_v4(transformation_matrix, v0);
    v4 dest1 = mul_m4_by_v4(transformation_matrix, v1);
    
    Bounding_box result = {
        .x0 = dest0.x,
        .y0 = dest0.y,
        .x1 = dest1.x,
        .y1 = dest1.y
    };
    
    return result;
}

Bounding_box get_bounding_box(float x0, float y0, float x1, float y1) {
    Bounding_box bbox = {0};
    bbox.min_x = x0;
    bbox.min_y = y0;
    bbox.max_x = x1;
    bbox.max_y = y1;
    
    return bbox;
}

v3 get_bbox_center(Bbox bbox) {
    float x = (bbox.x1 - bbox.x0) / 2;
    float y = (bbox.y1 - bbox.y0) / 2;
    float z = (bbox.z1 - bbox.z0) / 2;
    v3 result = get_v3(x, y, z);
    
    return result;
}

m4 math_lookat(v3 pos, v3 front, v3 up) {
    m4 result;
    glm_lookat(pos.e, front.e, up.e, result.e);
    
    return result;
}

m4 math_translate(m4 m, v3 amount) {
    m4 result;
    result = m;
    glm_translate(result.e, amount.e);
    
    return result;
}

m4 math_get_translation_matrix(v3 amount) {
    m4 result = math_translate(math_get_identity_matrix(), amount);
    
    return result;
}

m4 math_scale(m4 m, v3 scale) {
    m4 result = m;
    glm_scale(result.e, scale.e);
    
    return result;
}

m4 math_get_scale_matrix(v3 scale) {
    m4 result = math_scale(math_get_identity_matrix(), scale);
    
    return result;
}

v2 convert_opengl_screenspace_to_vulkan_screenspace(v2 pos, int height) {
    v2 result = get_v2(pos.x, height - pos.y);
    
    return result;
}

v2 convert_vulkan_screenspace_to_opengl_screenspace(v2 pos, int height) {
    v2 result = get_v2(pos.x, height - pos.y);
    
    return result;
}

v3 convert_screenspace_to_worldspace(v3 pos, m4 view, m4 projection) {
    v3 result;
    m4 vp = mul_m4_by_m4(projection, view);
    glm_unproject(pos.e, vp.e, (vec4){0, 0, g_screen_width, g_screen_height}, result.e);
    
    return result;
}

v3 raycast(v2 pos, m4 view, m4 projection) {
    v3 pos_near = convert_screenspace_to_worldspace(get_v3(pos.x, pos.y, 0), view, projection);
    v3 pos_far = convert_screenspace_to_worldspace(get_v3(pos.x, pos.y, 1), view, projection);
    
    v3 result = sub_v3(pos_far, pos_near);
    
    return result;
}

typedef struct Bottom_right_corner_rect_vertices {
    v3 vertices[6];
} Bottom_right_corner_rect_vertices;

Bottom_right_corner_rect_vertices get_bottom_right_corrner_rect_vertices(Rect rect, float z) {
    Bottom_right_corner_rect_vertices result;
    result.vertices[0] = get_v3(rect.x0, rect.y0, z);
    result.vertices[1] = get_v3(rect.x1, rect.y0, z);
    result.vertices[2] = get_v3(rect.x1, rect.y1, z);
    result.vertices[3]= get_v3(rect.x1, rect.y1, z);
    result.vertices[4] = get_v3(rect.x0, rect.y1, z);
    result.vertices[5] = get_v3(rect.x0, rect.y0, z);
    
    return result;
}

bool check_collision_between_vector_and_plane(v3 vector, Rect plane, float z, v3 camera_pos, float *distance) {
    Bottom_right_corner_rect_vertices rect_vertices = get_bottom_right_corrner_rect_vertices(plane, z);
    float first_distance = 0, second_distance = 0;
    bool first_result = glm_ray_triangle(camera_pos.e, vector.e, rect_vertices.vertices[0].e, rect_vertices.vertices[1].e, rect_vertices.vertices[2].e, &first_distance);
    bool second_result = glm_ray_triangle(camera_pos.e, vector.e, rect_vertices.vertices[3].e, rect_vertices.vertices[4].e, rect_vertices.vertices[5].e, &second_distance);
    
    if (first_result)
        *distance = first_distance;
    else if (second_result)
        *distance = second_distance;
    else
        *distance = 0;
    
    bool result = first_result || second_result;
    
    return result;
}

Rect get_rect(float x0, float y0, float x1, float y1) {
    Rect result = {
        .x0 = x0,
        .y0 = y0,
        .x1 = x1,
        .y1 = y1
    };
    
    return result;
}

m4 math_get_orthographic_matrix(Rect viewport, float near, float far) {
    m4 result;
    
    glm_ortho(viewport.left, viewport.right, viewport.bottom, viewport.top, near, far, result.e);
    
    return result;
}

Rect get_rect_by_pos(v2 pos, float width, float height) {
    Rect result = {
        .x0 = pos.x - width / 2,
        .x1 = pos.x + width / 2,
        .y0 = pos.y - height / 2,
        .y1 = pos.y + height / 2
    };
    
    return result;
}

bool is_v3_equal(v3 a, v3 b) {
    if (fabs(a.x - b.x) < 0.0001 && fabs(a.y - b.y) < 0.0001 && fabs(a.z - b.z) < 0.0001)
        return true;
    
    return false;
}
