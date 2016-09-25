#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;

layout (binding = 0) buffer ssbo_samScan_input
{
    uint64_t ginput[];
};

layout (binding = 1) buffer ssbo_samScan_output
{
    uint goutput[];
};

layout (binding = 2) buffer ssbo_samScan_gcarry
{
    uint gcarry[];
};

layout (binding = 3) buffer ssbo_samScan_gwait
{
    uint gwait[];
};

uniform uint NB_ELEMENTS;

shared uint globcarry;
shared uint tempcarry;
shared uint sbuf[32];
shared uint syncthreads_count;

void main()
{
    const uint tid = gl_LocalInvocationID.x;
    const uint warp = tid>>5;
    const uint lane = tid&31;
    //if(tid == gl_WorkGroupID.x)
    if(tid == 33) //33 because we won't stop the first warp 2 times
        gwait[gl_WorkGroupID.x] = 1;
    if(tid == 0)
    {
        tempcarry = 0;
        //globcarry = 0;
        //syncthreads_count = 0;
    }
    if(tid < 32)
        sbuf[tid] = 0;

    int pos = 0;
    uint chunk = gl_WorkGroupID.x;
    const uint offs = tid + chunk*1024; //gl_GlobalInvocationID.x
    //const uint firstid = 0;
    //const uint lastid = 0;

    uint val = (offs < NB_ELEMENTS ? uint(ginput[offs]) : 0);
    //intra-warp scan
    for(int d=1; d<32; d *=2)
    {
        uint tmp = shuffleUpNV(val, d, 32);
        if(lane >= d)
            val += tmp;
    }
    if(lane == gl_WarpSizeNV-1)
        sbuf[warp] = val;
    barrier();
    memoryBarrierShared();

    //first warp performs the inter-warp/intra-block scan
    if(warp == 0)
    {
        uint v = sbuf[lane];
        for(int d=1; d<32; d *=2)
        {
            uint tmp = shuffleUpNV(v, d, 32);
            if(lane >= d)
                v += tmp;
        }
        sbuf[lane] = v;
    }

    barrier();
    memoryBarrierShared();

    if(warp > 0)
    {
        const uint tix = warp-1;
        val += sbuf[tix];
    }

    if(tid == 1023)
    {
        if(chunk == 0)
        {
            gcarry[chunk] = val;
            gwait[chunk] = 0;
        }
        else
        {
            while(gwait[chunk-1] != 0) {}
            tempcarry = gcarry[chunk-1];
            gcarry[chunk] = tempcarry + val;
            gwait[chunk] = 0;
        }
    }
    barrier();
    memoryBarrierShared();

    val += tempcarry;

    if(offs == 0)
        goutput[0] = 0;
    if(offs+1 <= NB_ELEMENTS)
        goutput[offs+1] = val;
}
