#version 430 compatibility

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 vTexCoord;
//layout(location = 2) in vec3 vColor;

//uniform mat4 modelMat, projMat, viewMat;
//uniform vec3 light;

out vec2 texCoord;

void main()
{
//    n = vNormal;
//    gl_Position = projMat*viewMat*modelMat*vec4(vPosition, 1.0);
    texCoord = vTexCoord;
    gl_Position = vec4(vPosition, 1.0);
}
