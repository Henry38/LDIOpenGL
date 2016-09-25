#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/uniforms.hglsl"

layout(vertices=3) out;

in vec3 vPosition[];
in vec3 vNormal[];
in vec2 vTexCoords[];
flat in uint vIndice[];
out vec3 tcPosition[];
out vec3 tcNormal[];
out vec2 tcTexCoords[];
flat out uint tcIndice[];

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcNormal[ID] = vNormal[ID];
    tcTexCoords[ID] = vTexCoords[ID];
    tcIndice[ID] = vIndice[ID];

    if(ID==0)
    {
        gl_TessLevelInner[0] = tessInnerLvl;
        gl_TessLevelOuter[0] = tessOuterLvl;
        gl_TessLevelOuter[1] = tessOuterLvl;
        gl_TessLevelOuter[2] = tessOuterLvl;
    }
}
