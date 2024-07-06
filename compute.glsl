SHADER_VERSION_STRING
#include "common.h"
S(

writeonly uniform image2D output_texture;

/* uniforms */
uniform camera_t camera;

/* shader storage buffer objects */
layout(std430, binding = 0) buffer prim_buf  { primitive_t prims[]; };
layout(std430, binding = 1) buffer light_buf { light_t lights[];    };

/* internal structs */
struct ray_t { vec3  origin; vec3 dir;      };
struct hit_t { float t;      vec3 normal;   }; /* returned by intersections */

/* constants */
const float EPSILON         = 0.001f;
const float FLOAT_MAX       = 3.402823466e+38;
const uint  WIDTH           = WINDOW_WIDTH;    // from common.h
const uint  HEIGHT          = WINDOW_HEIGHT;   // from common.h
const uint  primitive_count = PRIMITIVE_COUNT; // from common.h
const uint  light_count     = LIGHT_COUNT;     // from common.h

hit_t ray_intersection(ray_t r, primitive_t prim)
{
    hit_t hit;

    sphere_t s = prim.s;
    triangle_t t = prim.t;

    /* compute t */
    float t_sphere = FLOAT_MAX;
    {
        vec3  difference   = r.origin - s.pos;
        float a            = 1.0f;
        float b            = 2.0f * dot(r.dir, difference);
        float c            = dot(difference, difference) - s.radius * s.radius;
        float discriminant = b * b - 4 * a * c;

        /* see if ray intersects at all */
        if (discriminant < 0) { t_sphere = FLOAT_MAX; }
        float root = sqrt(discriminant);

        /* solve for t */
        float q  = -0.5f * (b < 0 ? (b - root) : (b + root));
        float t0 = q / a;
        float t1 = c / q;
        float t  = min(t0, t1);
        if (t < EPSILON) { t = max(t0, t1); } /* too close to camera */

        if (t < EPSILON) { t = FLOAT_MAX; }   /* still too close to camera */

        t_sphere = t;

        /* set the normal at the hitpoint */
        vec3 intersection = r.origin + t_sphere * r.dir;
        hit.normal = normalize(intersection - s.pos);
    }

    float t_triangle = FLOAT_MAX;
    {
        vec3 a_to_b  = t.b - t.a;
        vec3 a_to_c  = t.c - t.a;

        mat3 mat = mat3(a_to_b, a_to_c, -1.0f * r.dir);

        float det = determinant(mat);

        if (det == 0.0f) { t_triangle = FLOAT_MAX; }

        vec3 ray_to_a = r.origin - t.a;

        vec3 dst = inverse(mat) * ray_to_a;

        if (dst.x >= -EPSILON && dst.x <= (1 + EPSILON)) {
            if (dst.y >= -EPSILON && dst.y <= (1 + EPSILON)) {
                if ((dst.x + dst.y) <= (1 + EPSILON)) {
                    t_triangle = dst.z;
                }
            }
        }
    }

    hit.t = min(t_triangle, t_sphere);

    return hit;
}

hit_t ray_sphere_intersection(ray_t r, sphere_t s)
{
    hit_t hit;

    /* compute t */
    {
        vec3  difference   = r.origin - s.pos;
        float a            = 1.0f;
        float b            = 2.0f * dot(r.dir, difference);
        float c            = dot(difference, difference) - s.radius * s.radius;
        float discriminant = b * b - 4 * a * c;

        /* see if ray intersects at all */
        if (discriminant < 0) { hit.t = FLOAT_MAX; return hit; }
        float root = sqrt(discriminant);

        /* solve for t */
        float q  = -0.5f * (b < 0 ? (b - root) : (b + root));
        float t0 = q / a;
        float t1 = c / q;
        float t  = min(t0, t1);
        if (t < EPSILON) { t = max(t0, t1); } /* too close to camera */

        if (t < EPSILON) { t = FLOAT_MAX; }   /* still too close to camera */

        hit.t = t;
    }

    /* set the normal at the hitpoint */
    vec3 intersection = r.origin + hit.t * r.dir;
    hit.normal = normalize(intersection - s.pos);

    return hit;
}

hit_t ray_triangle_intersection(ray_t r, triangle_t t)
{
    hit_t hit;

    vec3 a_to_b  = t.b - t.a;
    vec3 a_to_c  = t.c - t.a;

    mat3 mat = mat3(a_to_b, a_to_c, -1.0f * r.dir);

    float det = determinant(mat);

    if (det == 0.0f) { hit.t = FLOAT_MAX; return hit; } /* early out */

    vec3 ray_to_a = r.origin - t.a;

    vec3 dst = inverse(mat) * ray_to_a;

    if (dst.x >= -EPSILON && dst.x <= (1 + EPSILON)) {
        if (dst.y >= -EPSILON && dst.y <= (1 + EPSILON)) {
            if ((dst.x + dst.y) <= (1 + EPSILON)) {
                hit.t = dst.z;
                return hit;
            }
        }
    }

    hit.t = FLOAT_MAX;

    /* calculate normal */

    return hit;
}

vec4 shade(ray_t r, hit_t hit, int index)
{
    vec4 color     = vec4(0,0,0,1);

    material_t mat = prims[index].mat;

    vec3 intersection = r.origin + hit.t * r.dir;

    /* check if intersection is in shadow */
    for (int i = 0; i < light_count; i++)
    {
        vec3 to_light = normalize(lights[i].pos - intersection);

        ray_t ray_to_light = {intersection, to_light};
        bool is_in_shadow  = false;

        for (int prim_idx = 0; prim_idx < primitive_count; prim_idx++)
        {
            hit_t temp = {FLOAT_MAX, vec3(0)};
            temp = ray_intersection(ray_to_light, prims[prim_idx]);

            if (temp.t < FLOAT_MAX && temp.t >= EPSILON && temp.t < length(intersection - lights[i].pos))
            {
                is_in_shadow = true;
            }
        }

        if (!is_in_shadow)
        {
            float dist        = length(lights[i].pos - intersection);
            float attenuation = lights[i].p.intensity/dist;

            color += attenuation * mat.color;
            color += attenuation * lights[i].color;
        }
    }

    const float ambient_light_intensity = 0.1;
    color += ambient_light_intensity * mat.color;

    return color;
}

layout (local_size_x = WORK_GROUP_SIZE_X, local_size_y = WORK_GROUP_SIZE_Y, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    //const vec4 background_color = vec4(0.2,0.6,0.7,1);
    const vec4 background_color = vec4(0,0,0,0); // transparent
    vec4 color = vec4(0); // final color of pixel on texture

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
    uint reflection_depth = 3;
    {
        for (uint n = 0; n < reflection_depth; n++)
        {
            int tri_idx = -1; // TODO rename
            int a,b;
            hit_t hit   = { FLOAT_MAX, vec3(0,0,0) };
            hit_t temp  = { FLOAT_MAX, vec3(0,0,0) };
            /* compute intersection of ray and primitives */
            for (int i = 0; i < primitive_count; i++)
            {
                temp = ray_intersection(ray, prims[i]);
                if (temp.t < hit.t && temp.t >= EPSILON) {
                    hit     = temp;
                    tri_idx = i;
                }
            }

            //float no_intersection = step(-1, tri_idx);
            if (tri_idx != -1) /* ray hit triangle */
            {
                material_t mat = prims[tri_idx].mat;

                /* compute color */
                vec4 temp_color = shade(ray, hit, tri_idx);

                /* reflect if material is specular */
                if (mat.type == MATERIAL_TYPE_SPECULAR)
                {
                    float spec  = mat.spec;
                    color      += spec * temp_color;
                    /* compute reflection ray */
                    {
                        vec3 intersection = ray.origin + hit.t * ray.dir;
                        vec3 reflection   = normalize(ray.dir - 2 * dot(ray.dir, hit.normal) * hit.normal);

                        ray.origin = intersection + reflection;
                        ray.dir =  reflection;
                    }
                }
                else // no reflection needed
                {
                    color += temp_color;
                }
            } else { /* hit nothing but the background */
                color = background_color;
                break; /* early return */
            }
        }
    }

    imageStore(output_texture, ivec2(x, y), color);
}
)
