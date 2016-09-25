#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

#pragma optionNV (unroll all)

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;
//layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;

void main()
{
    float atomicVol = 1;
    float atomicBaryA_top=0, atomicBaryB_top=0, atomicBaryC_top=0;
    float atomicBaryA_down=0, atomicBaryB_down=0, atomicBaryC_down=0;

    uint id = gl_GlobalInvocationID.x;
    uint start_offset=0, end_offset=0;
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
            if(acc == 2)
            {
                float frag_coordBary_1;
                float topFrag_coordBary_1;
                if(frag.info_2.y != topFrag.info_2.y)
                {
                    //atomicCounterIncrement(vAtomicCounter);
                    atomicVol = AREA*BOX_DEPTH*(frag.info_2.y-topFrag.info_2.y);

                    topFrag_coordBary_1 = 1.0f-topFrag.info_2.z-topFrag.info_2.w;
                    frag_coordBary_1 = 1.0f-frag.info_2.z-frag.info_2.w;
                    atomicBaryA_top = AREA*topFrag_coordBary_1;
                    atomicBaryB_top = AREA*topFrag.info_2.z;
                    atomicBaryC_top = AREA*topFrag.info_2.w;
                    atomicBaryA_down = -AREA*frag_coordBary_1;
                    atomicBaryB_down = -AREA*frag.info_2.z;
                    atomicBaryC_down = -AREA*frag.info_2.w;

                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + topFrag.info_1.y + 1], atomicBaryA_top);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + topFrag.info_1.z + 1], atomicBaryB_top);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + topFrag.info_1.w + 1], atomicBaryC_top);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + frag.info_1.y + 1], atomicBaryA_down);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + frag.info_1.z + 1], atomicBaryB_down);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR + frag.info_1.w + 1], atomicBaryC_down);
                    atomicAdd(vGradients[NB_VERTICES*RENDER_DIR], atomicVol);
                }
            }
            acc -= 1;
        }
    }
}
