#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct pixel_frag {
    uint m_i;
    uint m_j;
    float m_z;
};

layout (std430, binding = 7) buffer ssbo_pixelFrag
{
    pixel_frag pixelFrag[];
};

layout (std430, binding = 8) buffer ssbo_indexFrag
{
    uint indexFrag[];
};

uniform uint nb_pixel;

void main(void)
{
    uint wid = gl_WorkGroupID.x;

    uint prev = (wid > 0 ? indexFrag[wid - 1] : 0);
    uint next = indexFrag[wid];
//    uint prev = 0;
//    uint next = indexFrag[wid];
//    if (wid > 0) {
//        prev = indexFrag[wid - 1];
//    }

    for (uint i = prev; i < next; ++i) {
        // sort pixelFrag
    }
}
