SHADER_VERSION_STRING
#include "common.h"
S(
writeonly uniform image2D output_texture;


struct camera_t { vec4 pos; vec4 dir; };
//uniform camera_t camera; // TODO
struct triangle_t { vec3 a; vec3 b; vec3 c; vec4 color; };
layout(std430, binding = 0) buffer triangle_buf { triangle_t triangles[]; };

struct ray_t { vec3 origin; vec3 dir; };

/* constants */
const float EPSILON        = 0.001f;
const float FLOAT_MAX      = 3.402823466e+38;
const uint  WIDTH          = WINDOW_WIDTH;   // from common.h
const uint  HEIGHT         = WINDOW_HEIGHT;  // from common.h
const uint  triangle_count = TRIANGLE_COUNT; // from common.h

float ray_triangle_intersection(ray_t r, triangle_t t)
{
    vec3 a_to_b  = t.b - t.a;
    vec3 a_to_c  = t.c - t.a;

    mat3 mat = mat3(a_to_b, a_to_c, -1.0f * r.dir);

    float det = determinant(mat);

    if (det == 0.0f) { return FLOAT_MAX; } /* early out */

    vec3 ray_to_a = r.origin - t.a;

    vec3 dst = inverse(mat) * ray_to_a;

    if (dst.x >= -EPSILON && dst.x <= (1 + EPSILON)) {
        if (dst.y >= -EPSILON && dst.y <= (1 + EPSILON)) {
            if ((dst.x + dst.y) <= (1 + EPSILON)) {
                return dst.z;
            }
        }
    }
    return FLOAT_MAX;
}

vec4 shade(ray_t r, float t, int index)
{
    vec4 color = vec4(0);

    vec3 intersection = r.origin + t * r.dir;

    // TODO implement different shaders
    color = triangles[index].color;

    return color;
}

layout (local_size_x = 1 /* NOTE: try higher */, local_size_y = 1 /* NOTE: try higher */, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    vec4 color = vec4(0); // final color of pixel on texture

    /* init ray */
    ray_t ray;
    camera_t camera;
    camera.pos = vec4( 0, 0, 0,1);
    camera.dir = vec4( 0, 0,-1,1);
    {
        // normalized device coordinates from (x,y) screen coords
        vec2 ndc = vec2((x + 0.5) / WIDTH, (y + 0.5) / HEIGHT);

        ray.origin = camera.pos.xyz;

        /* orthographic projection */
        ray.dir = normalize(camera.dir.xyz); // ray direction in camera space
        //ray.origin += ray.dir * 2.0 * ndc.x - camera.dir.xyz;
        ray.origin.x += ndc.x;
        ray.origin.y += ndc.y;
    }

    /* check for intersections */
    uint reflection_depth = 1;
    {
        for (uint n = 0; n < reflection_depth; ++n)
        {
            int tri_idx = -1;
            float t     = FLOAT_MAX;
            float temp  = FLOAT_MAX;

            /* compute intersection of ray and triangles */
            for (int i = 0; i < triangle_count; i++) {
                temp = ray_triangle_intersection(ray, triangles[i]);
                if (temp < t && temp >= -EPSILON) {
                    t       = temp;
                    tri_idx = i;
                }
            }

            if (tri_idx != -1) /* ray hit triangle */
            {
                /* compute color */
                color = shade(ray, t, tri_idx); // TODO consider specularity
            } else {
                break; /* early return */
            }
        }
    }

    imageStore(output_texture, ivec2(x, y), color);
}
)
