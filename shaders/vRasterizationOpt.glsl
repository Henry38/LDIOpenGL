#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/uniforms.hglsl"

layout(location = 0) in vec3 vPosition;
layout(location = 2) in uint vIndice;

flat out uint vvIndice;

void main()
{
    vvIndice = vIndice;
    gl_Position = projMat*viewMat*modelMat*vec4(vPosition, 1.0);
}
