#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

#pragma optionNV (unroll all)

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;
layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;
layout(binding = 1, offset = 0) uniform atomic_uint vAtomicVoxelCounter;

layout(binding=13) buffer ssbo_positions
{
    vec4 vPositions[];
};

layout(binding=14) buffer ssbo_intervals
{
    interval vIntervals[];
};

layout(binding=15) buffer ssbo_voxel_offsets
{
    uint vVoxelOffsets[];
};


const float cell_step = 0.1f;
const uint nb_frags = atomicCounter(vAtomicCounter);

void main()
{
    uint offset_counter = 0;
    uint id = gl_GlobalInvocationID.x;
    if(id >= nb_frags)
        return;
    uint global_offset = vVoxelOffsets[id];
    //collision computation
    interval tmpInterval = vIntervals[id];
    opt_frag downFrag = tmpInterval.z0;
    opt_frag topFrag = tmpInterval.z1;
    //get the cell number of our current fragment, xyz(coords) <=> klm(cell_nb)
    //this computation depends on the render direction
    float z_minus=0, z_plus=0;
    int nb_steps = 0;
    vec3 topPos = getPos(uint(topFrag.info_2.x), topFrag.info_2.y);
    vec3 downPos = getPos(uint(downFrag.info_2.x), downFrag.info_2.y);
    vec3 tmpPos = downPos; //for the loop

    //uint tmp = atomicCounterIncrement(vAtomicVoxelCounter);
    vPositions[global_offset+offset_counter] = vec4(topPos, 0);
    offset_counter ++;
    //tmp = atomicCounterIncrement(vAtomicVoxelCounter);
    vPositions[global_offset+offset_counter] = vec4(downPos, 0);
    offset_counter ++;

    if(RENDER_DIR == 2) //rendering along the z-axis
    {
        z_plus = topPos.z;
        z_minus = downPos.z;
    } else if(RENDER_DIR == 1) //rendering along the y-axis
    {
        z_plus = topPos.y;
        z_minus = downPos.y;
    } else { //rendering along the x-axis
        z_plus = topPos.x;
        z_minus = downPos.x;
    }
    //we start the process at W_minus of the cell m_down and we finish at
    //W_plus of the cell m_top
    float W_minus = float(uint(z_minus*10.0f))/10.0f;
    float W_plus = W_minus + cell_step;
    nb_steps = int(max((float(uint(z_plus*10.0f))/10.0f-W_minus)*10.0f, 1));

    W_minus = W_plus;
    W_plus += cell_step;

    uint j=0;
    while(j<30)
    {
        if(j>=nb_steps-1)
            break;
        tmpPos[RENDER_DIR] = W_minus;

        //tmp = atomicCounterIncrement(vAtomicVoxelCounter);
        vPositions[global_offset+offset_counter] = vec4(tmpPos, 0);
        offset_counter ++;

        //next iteration preparation
        W_minus = W_plus;
        W_plus += cell_step;
        j++;
    }
}
