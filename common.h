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
  #define PRIMITIVE_COUNT 10 // size of buffer
  /* used for lack of enums in glsl */
  #define PRIMITIVE_TYPE_NONE      0
  #define PRIMITIVE_TYPE_TRIANGLE  1
  #define PRIMITIVE_TYPE_SPHERE    2
  #define PRIMITIVE_TYPE_COUNT     3

/* common structs */
/* NOTE: we need to comply with std430 layout in C code. For now, specifiying padding
 * manually (always aligned to the size of a vec4) seems to work. We could also make use of
 * _Pragma ("pack(push,n)") _Pragma ("pack(pop)") when expanding the macro on the C-side.
 */
T(material_t,  { uint type; float _unused[3]; vec4 color;})
T(sphere_t,    { vec3 pos; float radius;      vec4 color;                                       })
T(triangle_t,  { vec3 a; float _1;            vec3 b; float _2; vec3 c; float _3; vec4 color;   })
T(primitive_t, { uint type; float _unused[3]; sphere_t s;                         triangle_t t; }) // TODO correct padding?
