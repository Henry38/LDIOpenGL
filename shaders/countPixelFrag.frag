#version 430 core

layout(binding = 2, offset = 0) uniform atomic_uint counter;

out vec4 color;

void main()
{
    atomicCounterIncrement(counter);

    color = vec4(0,0,0,1);
}
