SHADER_STRINGIFY(
writeonly uniform image2D output_texture;
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    imageStore(output_texture, ivec2(x, y), vec4(1,0,1,1));
}
)
