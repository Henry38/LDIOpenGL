#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"

layout(local_size_x=16, local_size_y=1, local_size_z=1) in;
layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;

layout(binding=0) buffer ssbo_voxel_frag_positions
{
    vec4 vPositions[];
};

layout(binding=1) buffer ssbo_octree
{
    uint vOctree[];
};

uniform uint NB_VOXEL_FRAGS;
uniform uint OCTREE_LEVEL;
uniform float VOXEL_DIM;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if(id == 0)
        vOctree[0] = 0;
    barrier();

    if(id >= NB_VOXEL_FRAGS)
        return;
    vec4 box_min = vec4(-VOXEL_DIM, -VOXEL_DIM, -VOXEL_DIM, 0);
    vec4 box_max = vec4(VOXEL_DIM, VOXEL_DIM, VOXEL_DIM, 0);
    vec4 pos = vPositions[id];
    uint node, subnode;
    float voxelDim = VOXEL_DIM;

    bool flag = true;
    uint child_id = 0;
    node = vOctree[child_id];

    for(uint i=0; i<OCTREE_LEVEL; i++)
    {
        voxelDim /= 2;
        subnode = clamp(int(1+pos.x-box_min.x-voxelDim), 0, 1);
        subnode += 4*clamp(int(1+pos.y-box_min.y-voxelDim), 0, 1);
        subnode += 2*clamp(int(1+pos.z-box_min.z-voxelDim), 0, 1);
        child_id += int(subnode);

        box_min += voxelDim*clamp(int(1+pos-box_min-voxelDim), 0, 1);
    }
    if(flag)
    {
        node = 0x80000000;
        atomicExchange(vOctree[child_id], node);
    }
}
