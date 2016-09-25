#version 430

in vec3 n;

uniform vec3 light;

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 normal;

void main()
{
    color = max(0.0, -dot(n, light)) * vec3(0.8, 0.8, 0.8);
    normal = n;
}
