#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct pixel_frag {
    uint m_i;
    uint m_j;
    float m_z;
};

layout (std430, binding = 6) buffer ssbo_pixelFrag
{
    uint pixelFrag[];
};

uniform uint max_pixelFrag;

void main(void)
{
}
