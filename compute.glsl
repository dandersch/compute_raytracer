SHADER_STRINGIFY(

writeonly uniform image2D output_texture;

struct triangle_t { vec3 a; vec3 b; vec3 c; vec4 color; };

struct camera_t
{
    vec4 pos;
    vec4 dir;
};

//uniform camera_t camera; // TODO
uniform uint width;
uniform uint height;
//uniform uint triangle_count; // TODO
layout(std430, binding = 0) buffer triangle_buf { triangle_t triangles[]; };

/* r = o + d * t */
struct ray_t {
  vec3 origin;
  vec3 dir;
};

/* constants */
const float EPSILON   = 0.001f;
const float FLOAT_MAX = 3.402823466e+38;

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

//layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    float col1 = x/640.0;
    float col2 = y/360.0;

    vec4 color = vec4(0.f); // final color of pixel on texture

    /* init ray */
    ray_t ray;
    camera_t camera; // TODO as uniform
    camera.pos = vec4( 0, 0, 0,1);
    camera.dir = vec4( 0, 0,-1,1);
    uint triangle_count = 1; // TODO as uniform
    {
        // Calculate normalized device coordinates (NDC) from pixel coordinates
        vec2 ndc = vec2((x + 0.5) / 640, (y + 0.5) / 360); // TODO hardcoded width and height

        ray.origin = camera.pos.xyz;

        vec3 ray_dir_in_camera_space = normalize(camera.dir.xyz);

        // ray origin in world space based on pixel coordinates
        ray.origin += ray_dir_in_camera_space * 2.0 * ndc.x - camera.dir.xyz;
        ray.dir = ray_dir_in_camera_space;
    }

    /* check for intersections */
    uint reflection_depth = 1;
    {
        for (uint n = 0; n < reflection_depth + 1; ++n)
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

    //imageStore(output_texture, ivec2(x, y), color);
    imageStore(output_texture, ivec2(x, y), color);
    //imageStore(output_texture, ivec2(x, y), triangles[0].color);
}
)
