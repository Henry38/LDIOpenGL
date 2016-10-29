#version 430 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(std140, binding = 0) uniform projection
{
    mat4 projViewMat;
};

void main()
{
    gl_Position = projViewMat * vec4(vPosition, 1.0);
}
