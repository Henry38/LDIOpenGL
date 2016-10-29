#version 430 core

layout(binding = 1, offset = 0) uniform atomic_uint ac_countFrag;

out vec4 color;

void main()
{
    atomicCounterIncrement(ac_countFrag);

    color = vec4(0,0,0,1);
}
