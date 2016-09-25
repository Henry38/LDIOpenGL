#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
//#include "/buffers.hglsl"
#include "/uniforms.hglsl"

#define MAX_LEVEL 4
#define CTA_SIZE 256
#define UINT_MAX 0xffffffff

layout(local_size_x=256, local_size_y=1, local_size_z=1) in;

layout (binding = 18) buffer ssbo_iData
{
    uint iData[];
};

layout (binding = 19) buffer ssbo_oData
{
    uint oData[];
};

uniform uint START_BIT, END_BIT, NB_ELEMENTS;
shared uint sData[gl_WorkGroupSize.x*4];

uint scanwarp_4(uint val)
{
    uint idx = 2*gl_LocalInvocationID.x - (gl_LocalInvocationID.x &(gl_WarpSizeNV-1));
    sData[idx] = 0;
    idx += gl_WarpSizeNV;
    uint t = sData[idx] = val;

    if(0 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-1];
    if(1 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-2];
    if(2 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-4];
    if(3 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-8];
    if(4 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-16];

    return sData[idx]-val;
}

uint scanwarp_2(uint val)
{
    uint idx = 2*gl_LocalInvocationID.x - (gl_LocalInvocationID.x &(gl_WarpSizeNV-1));
    sData[idx] = 0;
    idx += gl_WarpSizeNV;
    uint t = sData[idx] = val;

    if(0 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-1];
    if(1 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-2];
    if(2 <= MAX_LEVEL)
        sData[idx] = t = t + sData[idx-4];

    return sData[idx]-val;
}

uvec4 scan4(uvec4 idata)
{
    uint idx = gl_LocalInvocationID.x;
    uvec4 val4 = idata;

    uvec3 sum;
    sum.x = val4.x;
    sum.y = val4.y + sum.x;
    sum.z = val4.z + sum.y;

    uint val = val4.w + sum.z;

    val = scanwarp_4(val);

    barrier();
    memoryBarrierShared();

    if((idx & (gl_WarpSizeNV-1)) == gl_WarpSizeNV-1)
        sData[idx>>5] = val + val4.w + sum.z;

    barrier();
    memoryBarrierShared();

    if(idx < gl_WarpSizeNV)
        sData[idx] = scanwarp_2(sData[idx]);

    barrier();
    memoryBarrierShared();

    val += sData[idx>>5];

    val4.x = val;
    val4.y = val + sum.x;
    val4.z = val + sum.y;
    val4.w = val + sum.z;

    return val4;
}

shared uint numtrue;

uvec4 rank4(uvec4 preds)
{
    uvec4 address = scan4(preds);

    if(gl_LocalInvocationID.x == CTA_SIZE-1)
        numtrue = address.w + preds.w;
    barrier();
    memoryBarrierShared();

    uvec4 rank;
    uint idx = gl_LocalInvocationID.x<<2;
    rank.x = bool(preds.x) ? address.x : numtrue + idx - address.x;
    rank.y = bool(preds.y) ? address.y : numtrue + idx+1 - address.y;
    rank.z = bool(preds.z) ? address.z : numtrue + idx+2 - address.z;
    rank.w = bool(preds.w) ? address.w : numtrue + idx+3 - address.w;

    return rank;
}

uvec4 getKey(uint global_thread_id)
{
    uvec4 key;
    uint i = global_thread_id<<2;
    key.x = (i<NB_ELEMENTS ? iData[i] : UINT_MAX);
    key.y = (i+1<NB_ELEMENTS ? iData[i+1] : UINT_MAX);
    key.z = (i+2<NB_ELEMENTS ? iData[i+2] : UINT_MAX);
    key.w = (i+3<NB_ELEMENTS ? iData[i+3] : UINT_MAX);
    //key.x = iData[i];
    //key.y = iData[i+1];
    //key.z = iData[i+2];
    //key.w = iData[i+3];
    return key;
}

void insertVec4(uvec4 val4, uint global_thread_id)
{
    uint i = global_thread_id<<2;
    oData[i] = val4.x;
    oData[i+1] = val4.y;
    oData[i+2] = val4.z;
    oData[i+3] = val4.w;
}

shared uint sData2[gl_WorkGroupSize.x*4];

void sharedToOutput()
{
    uint idx = gl_LocalInvocationID.x<<2;
    uint idx_out = gl_GlobalInvocationID.x<<2;
    oData[idx_out] = sData2[idx];
    oData[idx_out+1] = sData2[idx+1];
    oData[idx_out+2] = sData2[idx+2];
    oData[idx_out+3] = sData2[idx+3];
    //oData[idx_out] = gl_GlobalInvocationID.x;
    //oData[idx_out+1] = gl_GlobalInvocationID.x;
    //oData[idx_out+2] = gl_GlobalInvocationID.x;
    //oData[idx_out+3] = gl_GlobalInvocationID.x;
}

void radixSortBlock(uvec4 key)
{
    for(uint shift=START_BIT; shift<END_BIT; shift++)
    {
        uvec4 lsb;
        lsb.x = uint(!bool((key.x>>shift) & 0x1));
        lsb.y = uint(!bool((key.y>>shift) & 0x1));
        lsb.z = uint(!bool((key.z>>shift) & 0x1));
        lsb.w = uint(!bool((key.w>>shift) & 0x1));

        uvec4 r = rank4(lsb);

        sData2[r.x] = key.x;
        sData2[r.y] = key.y;
        sData2[r.z] = key.z;
        sData2[r.w] = key.w;

        barrier();
        memoryBarrierShared();

        uint idx = gl_LocalInvocationID.x<<2;
        key.x = sData2[idx+0];
        key.y = sData2[idx+1];
        key.z = sData2[idx+2];
        key.w = sData2[idx+3];

        barrier();
        memoryBarrierShared();
    }
}

void main()
{
    uint idx_local = gl_LocalInvocationID.x;
    uint idx_global = gl_GlobalInvocationID.x;
    uvec4 key = getKey(idx_global);
    radixSortBlock(key);
    sharedToOutput();
}
