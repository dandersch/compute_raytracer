/*
 * Contains defines that are common to both shaders and c-code.
 *
 * Also contains centralized struct defs that are shared between glsl and C.
 *
 * NOTE: Can also contain e.g. helper functions that are shared between shaders.
 */
#define WINDOW_TITLE "compute raytracer"
#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 640
#define CAMERA_FOV     90
#define PRIMITIVE_COUNT 15 // size of buffer

/* used for lack of enums in glsl */
#define PRIMITIVE_TYPE_NONE      0
#define PRIMITIVE_TYPE_TRIANGLE  1
#define PRIMITIVE_TYPE_SPHERE    2
#define PRIMITIVE_TYPE_COUNT     3

#define MATERIAL_TYPE_NONE      0
#define MATERIAL_TYPE_DIFFUSE   1
#define MATERIAL_TYPE_SPECULAR  2
#define MATERIAL_TYPE_COUNT     3

/* common structs */
/* NOTE: we need to comply with std430 layout in C code. For now, specifiying padding
 * manually (always aligned to the size of a vec4) seems to work. We could also make use of
 * _Pragma ("pack(push,n)") _Pragma ("pack(pop)") when expanding the macro on the C-side.
 */
T(camera_t,    { vec4 pos;                          vec4 dir;                                                           })
T(material_t,  { uint type; float spec; float _[2]; vec4 color;                                                         })
T(sphere_t,    { vec3 pos; float radius;                                                                                })
T(triangle_t,  { vec3 a; float _1;                  vec3 b; float _2; vec3 c; float _3;                                 })
T(primitive_t, { uint type; float _unused[3];       sphere_t s;                         triangle_t t;   material_t mat; })
