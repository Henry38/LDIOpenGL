#version 430 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vColor;

uniform mat4 modelMat, projMat_persp, projMat_ortho, viewMat;
uniform vec3 light;

out vec3 n;

void main()
{
    n = vNormal;

    vec4 pos = viewMat * modelMat * vec4(vPosition, 1.0);

    // x-persp, y-ortho
    float x = (projMat_persp[0][0] * pos.x + projMat_persp[2][0] * pos.z) / -pos.z;
    float y = (projMat_ortho[1][1] * pos.y + projMat_ortho[3][1]);
    float z = (projMat_ortho[2][2] * pos.z + projMat_ortho[3][2]);
    float w = 1;

//    // x-ortho, y-persp
//    float x = (projMat_ortho[0][0] * pos.x + projMat_ortho[3][0]);
//    float y = (projMat_persp[1][1] * pos.y + projMat_persp[2][1] * pos.z) / -pos.z;
//    float z = (projMat_ortho[2][2] * pos.z + projMat_ortho[3][2]);
//    float w = 1;

    gl_Position = vec4(x,y,z,w);

    //gl_Position = projMat_persp * viewMat * modelMat * vec4(vPosition, 1);
}
