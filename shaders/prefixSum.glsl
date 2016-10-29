#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (std430, binding = 3) coherent readonly buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

layout (std430, binding = 5) coherent writeonly buffer ssbo_prefixSum
{
    uint prefixSum[];
};

layout (binding = 6) coherent writeonly buffer ssbo_blockSum
{
    uint blockSum[];
};

shared uint shared_data[gl_WorkGroupSize.x * 2];

void main(void)
{
    uint id = gl_WorkGroupID.x * 1024 + gl_LocalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;

    uint rd_id;
    uint wr_id;
    uint mask;

    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint step = 0;

    shared_data[lid * 2] = pixelHashTable[id * 2];
    shared_data[lid * 2 + 1] = pixelHashTable[id * 2 + 1];

    barrier();

    for (step = 0; step < steps; step++)
    {
        mask = (1 << step) - 1;
        rd_id = ((lid >> step) << (step + 1)) + mask;
        wr_id = rd_id + 1 + (lid & mask);

        shared_data[wr_id] += shared_data[rd_id];

        barrier();
    }

    prefixSum[id * 2 + 1] = shared_data[lid * 2];
    prefixSum[id * 2 + 2] = shared_data[lid * 2 + 1];

    if (lid == gl_WorkGroupSize.x-1) {
        blockSum[gl_WorkGroupID.x] = shared_data[lid * 2 + 1];
    }
}
