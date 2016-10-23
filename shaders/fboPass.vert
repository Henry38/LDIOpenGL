#version 430 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vColor;

uniform mat4 modelMat, projMat, viewMat;
uniform vec3 light;

out vec3 n;

void main()
{
    n = vNormal;
    gl_Position = projMat * viewMat * modelMat * vec4(vPosition, 1.0);
}
