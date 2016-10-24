#version 430 core

struct pixel_frag {
    uint m_i;
    uint m_j;
    float m_z;
};

layout (std430, binding = 5) buffer ssbo_prefixSum
{
    uint prefixSum[];
};

layout (std430, binding = 6) buffer ssbo_pixelFrag
{
    pixel_frag pixelFrag[];
};

uniform uint screen_width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int n = int(((screenPixel.y-0.5) * screen_width) + (screenPixel.x-0.5));

    pixel_frag frag;
    frag.m_i = uint(screenPixel.x);
    frag.m_j = uint(screenPixel.y);
    frag.m_z = gl_FragCoord.z;

    uint pos = prefixSum[n];
    pixelFrag[pos] = frag;
    atomicAdd(prefixSum[n], 1);

    color = vec4(0,0,0,1);
}
