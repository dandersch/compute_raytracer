/*
 * Contains defines that are common to both shaders and c-code.
 * (Can also contain defines that are exclusive to either)
 *
 * Due to restrictions with the C preprocessor, we need to duplicate them (once
 * as a normal #define and once as a stringified token). But at least they are
 * all centralized in this file and we don't have to pass them in as uniforms.
 *
 * NOTE: we could go even further with this and have centralized struct definitions...
 */
  #define WINDOW_TITLE "compute raytracer"
  #define WINDOW_WIDTH  1080
S(#define WINDOW_WIDTH  1080)
  #define WINDOW_HEIGHT 640
S(#define WINDOW_HEIGHT 640)
  #define TRIANGLE_COUNT 3
S(#define TRIANGLE_COUNT 3)
