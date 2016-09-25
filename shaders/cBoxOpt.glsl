#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

#pragma optionNV (unroll all)

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;
//layout(binding = 0, offset = 0) uniform atomic_uint vAtomic

const int magnitude = 100000;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    //initialisation of the box
    if(id<3)
       box[id] = 100*magnitude;
    else if(id<6)
       box[id] = -100*magnitude;
    barrier();

    uint start_offset, end_offset;
    start_offset = vPrefixSums[id];
    if(id >= NB_PIXELS)
        return;
    end_offset = vPrefixSums[id+1];
    if(start_offset == end_offset)
        return;
    //collision computation
    uint acc = 0;
    opt_frag frag, topFrag;
    for(uint i=start_offset; i<end_offset; i++)
    {
        frag = vOptFrags[i];
        uint normal_in = unpack_in(frag);
        if(normal_in == 0)
        {
            acc++;
            if(acc == 2)
            {
                topFrag = frag;
            }
        }
        if(normal_in == 1)
        {
            if(acc == 2 && (topFrag.info_2.y != frag.info_2.y))
            {
                //atomicCounterIncrement(vAtomic);
                vec3 topPos = getPos(uint(topFrag.info_2.x), topFrag.info_2.y);
                vec3 pos = getPos(uint(frag.info_2.x), frag.info_2.y);
                atomicMin(box[0], int(topPos.x*magnitude));
                atomicMin(box[1], int(topPos.y*magnitude));
                atomicMin(box[2], int(topPos.z*magnitude));
                atomicMax(box[3], int(topPos.x*magnitude));
                atomicMax(box[4], int(topPos.y*magnitude));
                atomicMax(box[5], int(topPos.z*magnitude));

                atomicMin(box[0], int(pos.x*magnitude));
                atomicMin(box[1], int(pos.y*magnitude));
                atomicMin(box[2], int(pos.z*magnitude));
                atomicMax(box[3], int(pos.x*magnitude));
                atomicMax(box[4], int(pos.y*magnitude));
                atomicMax(box[5], int(pos.z*magnitude));
            }
            acc -= 1;
        }
    }
}
