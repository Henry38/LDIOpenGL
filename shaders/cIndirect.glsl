#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;

layout(binding=0) buffer ssbo_indirect
{
    uvec4 vIndirect;
};


void main()
{
    vIndirect = uvec4(atomicCounter(vAtomicCounter)/1024+1, 1, 1, 0);
}
