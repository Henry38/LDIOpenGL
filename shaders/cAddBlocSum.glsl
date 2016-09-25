#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;

void main()
{
    uint id_global = gl_GlobalInvocationID.x;
    vPrefixSums[id_global] += vSums[gl_WorkGroupID.x/2];
}
