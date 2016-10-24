#version 430 core

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct pixel_frag {
    uint m_i;
    uint m_j;
    float m_z;
};

layout (std430, binding = 6) buffer ssbo_pixelFrag
{
    pixel_frag pixelFrag[];
};

uniform uint max_pixelFrag;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    if (id < max_pixelFrag) {
        pixelFrag[id].m_i = uint(0);
        pixelFrag[id].m_j = uint(0);
        pixelFrag[id].m_z = float(0.0);
    }
}
