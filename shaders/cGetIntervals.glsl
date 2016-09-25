#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;
layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;

//layout(binding=13) buffer ssbo_lengths
//{
//    float vLengths[];
//};

layout(binding=14) buffer ssbo_intervals
{
    interval vIntervals[];
};

void main()
{
    uint id = gl_GlobalInvocationID.x;
    uint start_offset, end_offset;
    start_offset = vPrefixSums[id];
    if(id >= NB_PIXELS)
        return;
    end_offset = vPrefixSums[id+1];
    if(start_offset == end_offset)
        return;
    //collision computation
    uint acc = 0;
    opt_frag topFrag, downFrag;
    for(uint i=start_offset; i<end_offset; i++)
    {
        topFrag = vOptFrags[i];
        uint normal_in = unpack_in(topFrag);
        if(normal_in == 0)
        {
            acc++;
            if(acc == 2)
            {
                downFrag = topFrag;
            }
        }
        if(normal_in == 1)
        {
            if(acc == 2 && (topFrag.info_2.y != downFrag.info_2.y))
            {
                uint indice = atomicCounterIncrement(vAtomicCounter);
                //Intervals Length scatter
                float tmpLength = BOX_DEPTH*(topFrag.info_2.y-downFrag.info_2.y);
                //vLengths[indice] = tmpLength;
                //Interval scatter
                interval tmpInterval;
                tmpInterval.z0 = downFrag;
                tmpInterval.z1 = topFrag;
                vIntervals[indice] = tmpInterval;
            }
            acc -= 1;
        }
    }
}
