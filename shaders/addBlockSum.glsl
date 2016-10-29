#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (binding = 5) buffer ssbo_prefixSum
{
    uint prefixSum[];
};

layout (binding = 7) buffer ssbo_blockSum
{
    uint blockSum[];
};

void main(void)
{
    uint id = gl_GlobalInvocationID.x;
    uint wid = gl_WorkGroupID.x;

    if (wid > 0) {
        prefixSum[id * 2] += blockSum[wid - 1];
        prefixSum[id * 2 + 1] += blockSum[wid - 1];
    }
}
