#version 430 compatibility

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

//uniform mat4 projMat, viewMat, modelMat;
//uniform vec3 light;

out vec3 n;
layout(std140) uniform shader_data
{
    mat4 projViewMat;
};

void main()
{
    n = vNormal;
    //gl_Position = projMat * viewMat * modelMat * vec4(vPosition, 1.0);
    gl_Position = projViewMat * vec4(vPosition, 1.0);
}
