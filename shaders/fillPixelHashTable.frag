#version 430 core

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

layout(binding = 2, offset = 0) uniform atomic_uint ac_countPixel;

uniform uint screen_width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int key = int(((screenPixel.y-0.5) * screen_width) + (screenPixel.x-0.5));

    uint n = atomicAdd(pixelHashTable[key], 1);
    if (n == 0) {
        atomicCounterIncrement(ac_countPixel);
    }

    color = vec4(0,0,0,1);
}
