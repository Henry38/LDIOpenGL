#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/uniforms.hglsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in uint indice;
layout(location = 3) in vec2 texCoords;

out vec3 vNormal;
out vec3 vPosition;
flat out uint vIndice;
out vec2 vTexCoords;

void main()
{
    vTexCoords = texCoords;
    vNormal = normal;
    vPosition = vec3(modelMat*vec4(position, 1.0));
    vIndice = indice;
    gl_Position = projMat*viewMat*vec4(vPosition, 1.0);
}
