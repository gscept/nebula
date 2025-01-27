//------------------------------------------------------------------------------
//  givolumecull.fx
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"

write rgba16f image2D DebugOutput;
//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader 
void csCull()
{
    uint index1D = gl_GlobalInvocationID.x;

    if (index1D > NumGIVolumeClusters)
        return;

    ClusterAABB aabb = AABBs[index1D];

    uint flags = 0;

    // update PBR decals
    uint numVolumes = 0;
    for (uint i = 0; i < NumGIVolumes; i++)
    {
        const GIVolume volume = GIVolumes[i];
        if (TestAABBAABB(aabb, volume.bboxMin.xyz, volume.bboxMax.xyz))
        {
            GIVolumeIndexLists[index1D * MAX_GI_VOLUMES_PER_CLUSTER + numVolumes] = i;
            numVolumes++;
        }
    }
    GIVolumeCountList[index1D] = numVolumes;

    // update feature flags if we have any decals
    if (numVolumes > 0)
        flags |= CLUSTER_GI_VOLUME_BIT;

    atomicOr(AABBs[index1D].featureFlags, flags);
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csDebug()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    float depth = fetch2D(DepthBuffer, PointSampler, coord, 0).r;

    // convert screen coord to view-space position
    vec4 worldPos = PixelToWorld(coord * InvFramebufferDimensions, depth, InvView, InvProjection);

    float viewDepth = CalculateViewDepth(View, worldPos.xyz);

    uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewDepth, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    uint flag = AABBs[idx].featureFlags; // add 0 so we can read the value
    vec4 color = vec4(0, 0, 0, 0);
    if (CHECK_FLAG(flag, CLUSTER_GI_VOLUME_BIT))
    {
        uint count = GIVolumeCountList[idx];
        color.r = count / float(NumGIVolumes);
    }
    
    imageStore(DebugOutput, int2(coord), color);
}

//------------------------------------------------------------------------------
/**
*/
program CullGIVolumes [ string Mask = "Cull"; ]
{
    ComputeShader = csCull();
};

//------------------------------------------------------------------------------
/**
*/
program ClusterDebug [ string Mask = "Debug"; ]
{
    ComputeShader = csDebug();
};
