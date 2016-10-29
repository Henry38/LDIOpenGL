#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

layout (std430, binding = 8) buffer ssbo_indexFrag
{
    uint indexFrag[];
};

uniform uint max_pixel;

void main(void)
{
    uint index = 0;

    for (uint i = 0; i < max_pixel; ++i) {
        if (pixelHashTable[i] != 0) {
            indexFrag[index] = indexFrag[index-1] + pixelHashTable[i];
            index += 1;
        }
    }
}
