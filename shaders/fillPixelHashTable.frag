#version 430 compatibility

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

layout(binding = 4, offset = 0) uniform atomic_uint counter;

uniform uint width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int n = int((screenPixel.y * width) + screenPixel.x);

    atomicCounterIncrement(counter);
    atomicAdd(pixelHashTable[n], 1);

    color = vec4(0,0,0,1);
}
