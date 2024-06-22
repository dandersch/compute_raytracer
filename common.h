/*
 * Contains defines that are common to both shaders and c-code.
 * (Can also contain defines that are exclusive to either)
 *
 * Also contains centralized struct defs that are shared between glsl and C.
 *
 * Due to restrictions with the C preprocessor, we need to duplicate them (once
 * as a normal #define and once as a stringified token). But at least they are
 * all centralized in this file and we don't have to pass them in as uniforms.
 *
 * NOTE: Can also contain e.g. helper functions that are shared between shaders.
 */
  #define WINDOW_TITLE "compute raytracer"
  #define WINDOW_WIDTH  640
S(#define WINDOW_WIDTH  640)
  #define WINDOW_HEIGHT 640
S(#define WINDOW_HEIGHT 640)
  #define CAMERA_FOV     90
S(#define CAMERA_FOV     90)
  #define PRIMITIVE_COUNT 3  // size of buffer
S(#define PRIMITIVE_COUNT 3)
  /* used for lack of enums in glsl */
  #define PRIMITIVE_TYPE_NONE      0
S(#define PRIMITIVE_TYPE_NONE      0)
  #define PRIMITIVE_TYPE_TRIANGLE  1
S(#define PRIMITIVE_TYPE_TRIANGLE  1)
  #define PRIMITIVE_TYPE_SPHERE    2
S(#define PRIMITIVE_TYPE_SPHERE    2)
  #define PRIMITIVE_TYPE_COUNT     3
S(#define PRIMITIVE_TYPE_COUNT     3)

/* common structs */
/* NOTE: we need to comply with std430 layout. For now, specifiying padding
 * manually seems to work. We could also make use of _Pragma ("pack(push,n)")
 * _Pragma ("pack(pop)") when expanding the macro on the C-side.
 */
T(sphere_t,    { vec3 pos; float radius;      vec4 color;                                       })
T(triangle_t,  { vec3 a; float _1;            vec3 b; float _2; vec3 c; float _3; vec4 color;   })
T(primitive_t, { uint type; float _unused[3]; sphere_t s;                         triangle_t t; }) // TODO correct padding?
