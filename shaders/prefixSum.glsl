#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout (binding = 3) coherent readonly buffer ssbo_pixelHashTable
{
    uint input_data[];
};

layout (binding = 5) coherent writeonly buffer ssbo_prefixSum
{
    uint output_data[];
};

layout (binding = 7) coherent writeonly buffer ssbo_blockSum
{
    uint blockSum[];
};

shared uint shared_data[gl_WorkGroupSize.x * 2];

uniform uint max_pixels;

void main(void)
{
    uint id = gl_LocalInvocationID.x;
    uint rd_id;
    uint wr_id;
    uint mask;

    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint step = 0;

    shared_data[id * 2] = input_data[id * 2];
    shared_data[id * 2 + 1] = input_data[id * 2 + 1];

    barrier();

    for (step = 0; step < steps; step++)
    {
        mask = (1 << step) - 1;
        rd_id = ((id >> step) << (step + 1)) + mask;
        wr_id = rd_id + 1 + (id & mask);

        shared_data[wr_id] += shared_data[rd_id];

        barrier();
    }

    output_data[id * 2] = shared_data[id * 2];
    output_data[id * 2 + 1] = shared_data[id * 2 + 1];

    if (id == gl_WorkGroupSize.x-1) {
        blockSum[gl_WorkGroupID.x-1] = shared_data[id * 2 + 1];
    }
}
