/*
 * Contains defines that are common to both shaders and c-code.
 *
 * Also contains centralized struct defs that are shared between glsl and C.
 *
 * NOTE: Can also contain e.g. helper functions that are shared between shaders.
 */
#define WINDOW_TITLE    "compute raytracer"
#define WINDOW_WIDTH    960
#define WINDOW_HEIGHT   540
#define CAMERA_FOV       90
#define PRIMITIVE_COUNT  30 // size of prim_buf
#define LIGHT_COUNT       3 // size of light_buf

/* NOTE: for values >=64 we get error: product of local_sizes exceeds MAX_COMPUTE_WORK_GROUP_INVOCATIONS (2048) */
#define WORK_GROUP_SIZE_X 16 // used in glDispatchCompute and local_size_x in compute shader
#define WORK_GROUP_SIZE_Y 16 // used in glDispatchCompute and local_size_y in compute shader

/* used for lack of enums in glsl */
#define PRIMITIVE_TYPE_NONE      0
#define PRIMITIVE_TYPE_TRIANGLE  1
#define PRIMITIVE_TYPE_SPHERE    2
#define PRIMITIVE_TYPE_COUNT     3

#define MATERIAL_TYPE_NONE       0
#define MATERIAL_TYPE_DIFFUSE    1
#define MATERIAL_TYPE_SPECULAR   2
#define MATERIAL_TYPE_COUNT      3

#define LIGHT_TYPE_NONE          0
#define LIGHT_TYPE_POINT         1
#define LIGHT_TYPE_COUNT         2

/* common structs */
/* NOTE: we need to comply with std430 layout in C code. For now, specifiying padding
 * manually (always aligned to the size of a vec4) seems to work. We could also make use of
 * _Pragma ("pack(push,n)") _Pragma ("pack(pop)") when expanding the macro on the C-side.
 */
T(camera_t,     { vec4 pos;                          vec4 dir;                                                           })

T(material_t,   { uint type; float spec; float _[2]; vec4 color;                                                         })

T(sphere_t,     { vec3 pos; float radius;                                                                                })
T(triangle_t,   { vec3 a; float _1;                  vec3 b; float _2; vec3 c; float _3;                                 })
T(primitive_t,  { uint type; float _unused[3];       sphere_t s;                         triangle_t t;   material_t mat; })

T(pointlight_t, { float intensity;                                                                                       })
T(light_t,      { uint type; float _unused[3];       vec3 pos;  float _1;                vec4 color;     pointlight_t p; })
