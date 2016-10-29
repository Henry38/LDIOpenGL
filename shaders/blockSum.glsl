#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (binding = 6) buffer ssbo_blockSum
{
    uint blockSum[];
};

uniform uint max_block;

void main(void)
{
    for (uint i = 1; i < max_block; ++i) {
        blockSum[i] += blockSum[i-1];
    }
}
