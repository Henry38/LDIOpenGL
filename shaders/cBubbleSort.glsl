#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

#pragma optionNV (unroll all)

layout(local_size_x=256, local_size_y=1, local_size_z=1) in;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    uint start_offset=0, end_offset=0;
    start_offset = vPrefixSums[id];
    if(id >= NB_PIXELS)
        return;
    end_offset = vPrefixSums[id+1];
    if((end_offset-start_offset)<2)
        return;
    //bubbleSort
    for(uint i=start_offset; i<end_offset; i++)
    {
        for(uint j=start_offset; j<(end_offset - (i-start_offset)-1); j++)
        {
            if(vOptFrags[j].info_2.y >= vOptFrags[j+1].info_2.y)
                swap(j, j+1);
        }
    }
}
