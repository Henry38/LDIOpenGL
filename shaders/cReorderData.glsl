#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
//#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=512, local_size_y=1, local_size_z=1) in;

layout (binding = 0) buffer ssbo_oData
{
    uint keys[];
};

layout (binding = 1) buffer ssbo_scanned_counters
{
    uint offsets[];
};

layout (binding = 2) buffer ssbo_blockOffsets
{
    uint blockOffsets[];
};

layout (binding = 3) buffer ssbo_outKeys
{
    uint outKeys[];
};


uniform uint NB_ELEMENTS, TOTAL_BLOCKS, START_BIT, END_BIT, CTA_SIZE;

shared uint sKeys[gl_WorkGroupSize.x*2];
shared uint sOffsets[16];
shared uint sBlockOffsets[16];

void main()
{
    uint blockID = gl_WorkGroupID.x;
    //uint i = blockID*gl_WorkGroupSize.x + gl_LocalInvocationID.x;
    uint i = gl_GlobalInvocationID.x;
    uint id_local = gl_LocalInvocationID.x;

    sKeys[2*id_local] = keys[2*i];
    sKeys[2*id_local+1] = keys[2*i+1];

    if(id_local < 16)
    {
        sOffsets[id_local] = offsets[id_local*TOTAL_BLOCKS+blockID];
        sBlockOffsets[id_local] = blockOffsets[blockID*16+id_local];
    }
    barrier();
    memoryBarrierShared();

    uint radix = (sKeys[id_local]>>START_BIT)&0xf;
    uint globalOffset = sOffsets[radix] + id_local - sBlockOffsets[radix];
    if(globalOffset < NB_ELEMENTS)
        outKeys[globalOffset] = sKeys[id_local];

    radix = (sKeys[id_local+2*CTA_SIZE]>>START_BIT)&0xf;
    globalOffset = sOffsets[radix] + id_local+2*CTA_SIZE - sBlockOffsets[radix];
    if(globalOffset < NB_ELEMENTS)
        outKeys[globalOffset] = sKeys[id_local+2*CTA_SIZE];
}
