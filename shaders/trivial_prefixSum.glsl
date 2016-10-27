#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint input_data[];
};

layout (std430, binding = 5) buffer ssbo_prefixSum
{
    uint output_data[];
};

uniform uint max_pixels;

void main(void)
{
    output_data[0] = 0;
    for (uint i = 1; i < max_pixels; ++i) {
        output_data[i] = output_data[i-1] + input_data[i-1];
    }
}
