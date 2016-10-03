#version 430 compatibility

layout (std430, binding = 2) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

uniform uint width;

out vec4 color;

void main()
{
    vec2 screenPixel = gl_FragCoord.xy;
    int n = int((screenPixel.y * width) + screenPixel.x);
    pixelHashTable[n] = pixelHashTable[n] + 1;

    color = vec4(0,0,0,1);
}
