#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
//#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=512, local_size_y=1, local_size_z=1) in;

layout (binding = 19) buffer ssbo_oData
{
    uint oData[];
};

layout (binding = 20) buffer ssbo_counters
{
    uint counters[];
};

layout (binding = 21) buffer ssbo_blockOffsets
{
    uint blockOffsets[];
};

uniform uint TOTAL_BLOCKS, START_BIT, END_BIT, CTA_SIZE;

shared uint sRadix1[gl_WorkGroupSize.x*2+2];
shared uint sStartPointers[16];

void main()
{
    uint blockID = gl_WorkGroupID.x;
    uint id_local = gl_LocalInvocationID.x;

    uvec2 radix2;
    uint i = gl_GlobalInvocationID.x<<1;
    radix2.x = oData[i];
    radix2.y = oData[i+1];

    sRadix1[id_local<<1] = (radix2.x >> START_BIT) & 0xF;
    sRadix1[1+(id_local<<1)] = (radix2.y >> START_BIT) & 0xF;

    //find block offsets
    if(id_local < 16)
    {
        counters[id_local*TOTAL_BLOCKS+blockID] = 0;
        blockOffsets[blockID*16+id_local] = 0;
        sStartPointers[id_local] = 0;
    }
    barrier();
    memoryBarrierShared();

    if(id_local > 0 && (sRadix1[id_local] != sRadix1[id_local-1]))
        sStartPointers[sRadix1[id_local]] = id_local;
    if(sRadix1[id_local+2*CTA_SIZE] != sRadix1[id_local+2*CTA_SIZE-1])
        sStartPointers[sRadix1[id_local+2*CTA_SIZE]] = id_local+2*CTA_SIZE;
    barrier();
    memoryBarrierShared();

    if(id_local < 16)
        blockOffsets[blockID*16+id_local] = sStartPointers[id_local];

    barrier();
    memoryBarrierShared();

    //compute block sizes
    if(id_local > 0 && (sRadix1[id_local] != sRadix1[id_local-1]))
        sStartPointers[sRadix1[id_local-1]] =
                id_local - sStartPointers[sRadix1[id_local-1]];
    if(sRadix1[id_local+2*CTA_SIZE] != sRadix1[id_local+2*CTA_SIZE-1])
        sStartPointers[sRadix1[id_local+2*CTA_SIZE-1]] =
                id_local+2*CTA_SIZE - sStartPointers[sRadix1[id_local+2*CTA_SIZE-1]];
    if(id_local == 2*CTA_SIZE-1)
        sStartPointers[sRadix1[4*CTA_SIZE-1]] =
                4*CTA_SIZE - sStartPointers[sRadix1[4*CTA_SIZE-1]];
    barrier();
    memoryBarrierShared();

    if(id_local < 16)
        counters[id_local*TOTAL_BLOCKS+blockID] = sStartPointers[id_local];
        //counters[id_local+16*blockID] = sStartPointers[id_local];
    //avoids column-major order but still gives us the counters for each block in an coalesced manner
}
