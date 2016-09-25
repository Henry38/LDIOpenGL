#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"

layout(local_size_x=256, local_size_y=1, local_size_z=1) in;

void main()
{
    if(gl_GlobalInvocationID.x < NB_PIXELS)
      vZeroFloat[gl_GlobalInvocationID.x] = 0.0f;
}
