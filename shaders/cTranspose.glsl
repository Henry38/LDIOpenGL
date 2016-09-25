#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"

#define BLOCK_DIM 16

layout(binding=15) buffer ssbo_in_mat
{
    uint vInMat[];
};

layout(binding=16) buffer ssbo_out_mat
{
    uint vOutMat[];
};

layout(local_size_x=BLOCK_DIM, local_size_y=BLOCK_DIM, local_size_z=1) in;

uniform uint MAT_HEIGHT, MAT_WIDTH;

shared uint vShared[gl_WorkGroupSize.x][gl_WorkGroupSize.y+1];

void main()
{
    //OPTIMIZED
    uint x_index = gl_LocalInvocationID.x;
    uint y_index = gl_LocalInvocationID.y;

    if((x_index<MAT_WIDTH) && (y_index<MAT_HEIGHT))
    {
        uint in_index = x_index + MAT_WIDTH*y_index;
        vShared[gl_LocalInvocationID.y][gl_LocalInvocationID.x] = vInMat[in_index];
    }
    barrier();
    memoryBarrierShared();

    if((x_index<MAT_HEIGHT) && (y_index<MAT_WIDTH))
    {
        uint out_index = x_index + MAT_HEIGHT*y_index;
        vOutMat[out_index] = vShared[gl_LocalInvocationID.x][gl_LocalInvocationID.y];
    }

    //COPY
    //uint x_index = gl_LocalInvocationID.x;
    //uint y_index = gl_LocalInvocationID.y;

    //if((x_index<MAT_WIDTH) && (y_index<MAT_HEIGHT))
    //{
    //    uint in_index = x_index + MAT_WIDTH*y_index;
    //    uint out_index = y_index + MAT_HEIGHT*x_index;
    //    vOutMat[out_index] = vInMat[in_index];
    //}

}
