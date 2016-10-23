#version 430 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

uniform uint max_pixels;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if (id < max_pixels) {
        pixelHashTable[id] = 0;
    }
}
