#version 430
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

flat in uvec3 gIndice;
in vec3 coordBary;

out vec4 color;

//layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;
void main()
{
    color = vec4(0.5, 0.5, 0.5, 1);
    uvec2 screen = uvec2(gl_FragCoord.xy);
    uint64_t key = Haddr(screen);//the key is the linearized 2D coordinate of the fragment
    uint frag_index = getIndex(key);
    opt_frag frag;
    frag.info_1.x = pack_in_idObj(gl_FrontFacing ? 0 : 1, ID_OBJ);

    frag.info_1.y = gIndice.x;
    frag.info_1.z = gIndice.y;
    frag.info_1.w = gIndice.z;

    frag.info_2.x = (gl_FragCoord.x-0.5)*SCREEN_WIDTH + gl_FragCoord.y-0.5;
    frag.info_2.y = gl_FragCoord.z;
    frag.info_2.z = coordBary.y;
    frag.info_2.w = coordBary.z;

    vOptFrags[frag_index] = frag;
}
