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

layout (std430, binding = 7) buffer ssbo_pixelFrag
{
    pixel_frag pixelFrag[];
};

uniform uint screen_width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int key = int(((screenPixel.y-0.5) * screen_width) + (screenPixel.x-0.5));

    uint pos = atomicAdd(prefixSum[key], 1);
    pixelFrag[pos].m_i = uint(screenPixel.x);
    pixelFrag[pos].m_j = uint(screenPixel.y);
    pixelFrag[pos].m_z = gl_FragCoord.z;

    color = vec4(0,0,0,1);
}
