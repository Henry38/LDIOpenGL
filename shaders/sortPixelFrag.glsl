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

bool compareTo(uint i, uint j)
{
    pixel_frag frag1 = pixelFrag[i];
    pixel_frag frag2 = pixelFrag[j];

    if (frag1.m_j < frag2.m_j)
        return true;
    if (frag1.m_j > frag2.m_j)
        return false;

    if (frag1.m_i < frag2.m_i)
        return true;
    if (frag1.m_i > frag2.m_i)
        return false;

    return (frag1.m_z < frag2.m_z);
}

void swap(uint i, uint j)
{
    pixel_frag tmp = pixelFrag[i];
    pixelFrag[i] = pixelFrag[j];
    pixelFrag[j] = tmp;
}

void main(void)
{
    uint wid = gl_WorkGroupID.x;

    uint begin = (wid > 0 ? indexFrag[wid - 1] : 0);
    uint end = indexFrag[wid];

    for (uint i = begin+1; i < end; ++i) {
        uint k = i;
        bool stop = false;
        while (!stop && k > begin) {
            stop = compareTo(k-1, k);
            if (!stop) {
                swap(k, k-1);
                k = k - 1;
            }
        }
    }
}
