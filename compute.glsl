SHADER_VERSION_STRING
#include "common.h"
STR(
writeonly uniform image2D output_texture;


struct camera_t { vec4 pos; vec4 dir; };
uniform camera_t camera;

layout(std430, binding = 0) buffer prim_buf { primitive_t prims[]; };

struct ray_t { vec3 origin; vec3 dir; };

/* constants */
const float EPSILON         = 0.001f;
const float FLOAT_MAX       = 3.402823466e+38;
const uint  WIDTH           = WINDOW_WIDTH;    // from common.h
const uint  HEIGHT          = WINDOW_HEIGHT;   // from common.h
const uint  primitive_count = PRIMITIVE_COUNT; // from common.h

float ray_sphere_intersection(ray_t r, sphere_t s)
{
    // NOTE algorithms behave differently when inside sphere
    #if 1
    vec3  difference   = r.origin - s.pos;
    float a            = 1.0f;
    float b            = 2.0f * dot(r.dir, difference);
    float c            = dot(difference, difference) - s.radius * s.radius;
    float discriminant = b * b - 4 * a * c;

    // see if ray intersects at all
    if (discriminant < 0) { return FLOAT_MAX; }
    float root = sqrt(discriminant);

    // solution
    float q  = -0.5f * (b < 0 ? (b - root) : (b + root));
    float t0 = q / a;
    float t1 = c / q;
    float t  = min(t0, t1);
    if (t < EPSILON) { t = max(t0, t1); }

    if (t < EPSILON) { return FLOAT_MAX; }

    return t;

    #else

    vec3 sphere_to_ray = r.origin - s.pos;
    float s_roc        = dot(r.dir, sphere_to_ray); // b
    float s_oc         = dot(sphere_to_ray, sphere_to_ray);
    float d            = s_roc * s_roc - s_oc + s.radius * s.radius;

    if (d < 0) {
      return FLOAT_MAX;
    } else {
      float t1 = sqrt(d);
      float t2 = -s_roc - t1;
      t1 = -s_roc + t1;

      /* ray is in sphere */
      if ((t1 < 0 && t2 > 0) || (t1 > 0 && t2 < 0)) { return FLOAT_MAX; }

      if ((t2 > t1 ? t1 : t2) < 0) { return FLOAT_MAX;           }
      else                         { return (t2 > t1 ? t1 : t2); }
    }
    #endif
}

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
    switch (prims[index].type)
    {
        case PRIMITIVE_TYPE_TRIANGLE: { color = prims[index].t.color; } break;
        case PRIMITIVE_TYPE_SPHERE:   { color = prims[index].s.color; } break;
    }

    return color;
}

layout (local_size_x = 1 /* NOTE: try higher */, local_size_y = 1 /* NOTE: try higher */, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    const vec4 background_color = vec4(0.3,0.2,0,1);
    vec4 color = background_color; // final color of pixel on texture

    /* init ray */
    ray_t ray;

    // normalized device coordinates from (x,y) screen coords
    vec2 ndc = vec2((x + 0.5) / WIDTH, (y + 0.5) / HEIGHT);
    ray.origin = camera.pos.xyz;

    #if 0
    {  /* orthographic projection */
        ray.dir = normalize(camera.dir.xyz); // ray direction in camera space
        ray.origin.x += ndc.x;
        ray.origin.y += ndc.y;
        //ray.origin += ray.dir * 2.0 * ndc.x - camera.dir.xyz;
    }
    #else
    { /* perspective projection */
        vec3 cam_dir = normalize(camera.dir.xyz);
        vec3 right   = normalize(cross(cam_dir, vec3(0, 1, 0)));
        vec3 up      = normalize(cross(right, cam_dir));

        float aspect_ratio = float(WIDTH) / float(HEIGHT);
        float fov = radians(CAMERA_FOV); // from common.h
        float tan_half_fov = tan(fov / 2.0);

        ray.dir = normalize(cam_dir + right * (2.0 * ndc.x - 1.0) * tan_half_fov * aspect_ratio + up * (1.0 - 2.0 * ndc.y) * tan_half_fov);
    }
    #endif

    /* check for intersections */
    uint reflection_depth = 2;
    {
        for (uint n = 0; n < reflection_depth; ++n)
        {
            int tri_idx = -1;
            float t     = FLOAT_MAX;
            float temp  = FLOAT_MAX;

            /* compute intersection of ray and primitives */
            for (int i = 0; i < primitive_count; i++)
            {
                switch (prims[i].type)
                {
                    case PRIMITIVE_TYPE_TRIANGLE: { temp = ray_triangle_intersection(ray, prims[i].t); } break;
                    case PRIMITIVE_TYPE_SPHERE:   { temp = ray_sphere_intersection(ray, prims[i].s);   } break;
                }

                if (temp < t && temp >= -EPSILON) {
                    t       = temp;
                    tri_idx = i;
                }
            }

            if (tri_idx != -1) /* ray hit triangle */
            {
                /* compute color */
                color = shade(ray, t, tri_idx); // TODO consider specularity

                // TODO if not specular, exit early, else compute a reflection ray
            } else {
                break; /* early return */
            }
        }
    }

    imageStore(output_texture, ivec2(x, y), color);
}
)
