#version 430 compatibility

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

layout(std140) uniform projection
{
    mat4 projViewMat;
};

//out vec3 n;

void main()
{
    //n = vNormal;
    gl_Position = projViewMat * vec4(vPosition, 1.0);
}
