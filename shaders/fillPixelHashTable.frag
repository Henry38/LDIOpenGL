#version 430 core

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

//layout(binding = 1, offset = 0) uniform atomic_uint counter;

uniform uint screen_width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int n = int(((screenPixel.y-0.5) * screen_width) + (screenPixel.x-0.5));

    //atomicCounterIncrement(counter);
    atomicAdd(pixelHashTable[n], 1);

    color = vec4(0,0,0,1);
}
