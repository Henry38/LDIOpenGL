#version 430
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
//#include "/buffers.hglsl"
#include "/uniforms.hglsl"
//#include "/auxiliary_functions.hglsl"

layout(binding = 0, offset = 0) uniform atomic_uint aboCounter;

layout(binding=13) buffer ssbo_keys
{
    float vKeys[];
};

layout(binding=14) buffer ssbo_indices
{
    uvec4 vIndices[];
};

layout(binding=15) buffer ssbo_depths
{
    float vDepths[];
};

layout(binding=16) buffer ssbo_barys
{
    vec2 vBarys[];
};

flat in uvec3 gIndice;
in vec3 coordBary;
in vec4 gPosition;

out vec4 color;

void main()
{
    uint tmp = atomicCounterIncrement(aboCounter);
    color = vec4(0.5, 0.5, 0.5, 1);
    opt_frag frag;
    //frag.info_1.x = pack_in_idObj(gl_FrontFacing ? 0 : 1, ID_OBJ);
    frag.info_1.x = uint(gl_FrontFacing);

    frag.info_1.y = gIndice.x;
    frag.info_1.z = gIndice.y;
    frag.info_1.w = gIndice.z;

    frag.info_2.x = (gl_FragCoord.x-0.5)*SCREEN_WIDTH + gl_FragCoord.y-0.5;
    frag.info_2.y = gl_FragCoord.z;
    frag.info_2.z = coordBary.y;
    frag.info_2.w = coordBary.z;

    vKeys[tmp] = frag.info_2.x;
    vIndices[tmp] = frag.info_1;
    vDepths[tmp] = frag.info_2.y;
    vBarys[tmp] = frag.info_2.zw;
    //vOptFrags[tmp] = frag;
}