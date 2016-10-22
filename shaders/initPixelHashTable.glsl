#version 430 compatibility

layout(local_size_x=256, local_size_y=1, local_size_z=1) in;

layout (std430, binding = 3) buffer ssbo_pixelHashTable
{
    uint pixelHashTable[];
};

uniform uint max;

void main()
{
    if(gl_GlobalInvocationID.x < max) {
        pixelHashTable[gl_GlobalInvocationID.x] = 0;
    }
}
