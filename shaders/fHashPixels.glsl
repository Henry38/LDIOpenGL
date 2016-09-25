#version 430
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

out vec4 color;

void main()
{
    color = vec4(0.5, 0.5, 0.5, 1);
    uvec2 screen = uvec2(gl_FragCoord.xy);
    uint64_t key = Haddr(screen);//the key is the linearized 2D coordinate of the fragment
    insert(key);
}
