#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;

shared uint vShared[gl_WorkGroupSize.x*2+2];

void main()
{
    uint id_local = gl_LocalInvocationID.x;
    uint id_global = gl_GlobalInvocationID.x;
    uint rd_id, wr_id, mask;

    const uint steps = uint(log2(gl_WorkGroupSize.x)) + 1;
    uint d = 0;

    if(id_local == 0)
        vShared[0] = 0;
    //Input to shared
    vShared[2*id_local+1] = (2*id_global < NB_PIXELS ? uint(hashTable[2*id_global]) : 0);
    vShared[2*id_local+2] = (2*id_global+1 < NB_PIXELS ? uint(hashTable[2*id_global+1]) : 0);
    barrier();
    memoryBarrierShared();
    //summation
    for(d=0; d<steps; d++)
    {
        mask = (1<<d)-1;
        rd_id = ((id_local>>d) << (d+1)) + mask;
        wr_id = rd_id + 1 + (id_local&mask);

        vShared[wr_id] += vShared[rd_id];
        barrier();
        memoryBarrierShared();
    }
    //shared to output
    vPrefixSums[2*id_global] = vShared[2*id_local];
    vPrefixSums[2*id_global+1] = vShared[2*id_local+1];

    //if(id_local == (gl_WorkGroupSize.x-1) && id_global < NB_PIXELS)
    if(id_local == (gl_WorkGroupSize.x-1))
      vSums[gl_WorkGroupID.x] = vShared[2*id_local+1] + uint(hashTable[2*id_global+1]);
}
