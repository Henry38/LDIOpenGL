#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=256, local_size_y=1, local_size_z=1) in;
layout(binding = 0, offset = 0) uniform atomic_uint vAtomic; 

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if(id > NB_VERTICES)
        return;
    //fetch the 3 values
    float val_1 = vGradients[id];
    float val_2 = vGradients[NB_VERTICES+1+id];
    float val_3 = vGradients[2*(NB_VERTICES+1)+id];
    float abs_sum = abs(val_1)+abs(val_2)+abs(val_3);
    if(abs_sum > 0.0001)
    {
        uint sparseIndice = atomicCounterIncrement(vAtomic);
        vec4 tmp;
        tmp.x = id;
        tmp.y = val_1;
        tmp.z = val_2;
        tmp.w = val_3;
        vSparseGradients[sparseIndice] = tmp;
    }
}
