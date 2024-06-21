SHADER_STRINGIFY(

writeonly uniform image2D output_texture;

struct triangle_t { vec3 a; vec3 b; vec3 c; vec4 color; };

};

layout(std430, binding = 0) buffer triangle_buf { triangle_t triangles[]; };

/* r = o + d * t */
struct ray_t {
  vec3 origin;
  vec3 dir;
};

//layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    float col1 = x/640.0;
    float col2 = y/360.0;
    float col3 = test_array[0].a;

    imageStore(output_texture, ivec2(x, y), vec4(col1, col2, col3, 1));
}
)
