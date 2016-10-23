#version 430 core

//in vec3 n;

//uniform vec3 light;

layout(location = 0) out vec3 color;

void main()
{
    color = vec3(0, 0, 0);
    //color = max(0.0, -dot(n, light)) * vec3(0.8, 0.8, 0.8);
}
