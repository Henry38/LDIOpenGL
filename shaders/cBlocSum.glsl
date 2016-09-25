#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;

shared uint vShared[gl_WorkGroupSize.x*2+1];

void main()
{
    uint id = gl_LocalInvocationID.x;
    uint rd_id, wr_id, mask;

    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint d = 0;

    if(id == 0)
        vShared[0] = 0;
    //Input to shared
    vShared[2*id+1] = (2*id < NB_SUMS ? uint(vSums[2*id]) : 0);
    vShared[2*id+2] = (2*id+1 < NB_SUMS ? uint(vSums[2*id+1]) : 0);
    barrier();
    memoryBarrierShared();
    //summation
    for(d=0; d<steps; d++)
    {
        mask = (1<<d)-1;
        rd_id = ((id>>d) << (d+1)) + mask;
        wr_id = rd_id + 1 + (id&mask);

        vShared[wr_id] += vShared[rd_id];
        barrier();
        memoryBarrierShared();
    }
    vSums[2*id] = vShared[2*id];
    vSums[2*id+1] = vShared[2*id+1];
}
