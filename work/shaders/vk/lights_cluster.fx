//------------------------------------------------------------------------------
//  lights_cluster.fx
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"
#include "lib/lights_clustered.fxh"
#include "lib/preetham.fxh"
#include "lib/mie-rayleigh.fxh" 

write rgba16f image2D Lighting;
write rgba16f image2D DebugOutput;

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 64
shader 
void csCull()
{
    uint index1D = gl_GlobalInvocationID.x;

    if (index1D > NumLightClusters)
        return;

    ClusterAABB aabb = AABBs[index1D];

    uint flags = 0;

    // update pointlights
    uint numLights = 0;
    for (uint i = 0; i < NumPointLights; i++)
    {
        const PointLight light = PointLights[i];
        if (TestAABBSphere(aabb, light.position.xyz, light.position.w))
        {
            PointLightIndexList[index1D * MAX_LIGHTS_PER_CLUSTER + numLights] = i;
            numLights++;
        }
    }
    PointLightCountList[index1D] = numLights;

    // update feature flags if we have any lights
    if (numLights > 0)
        flags |= CLUSTER_POINTLIGHT_BIT;

    // update spotlights
    numLights = 0;
    for (uint i = 0; i < NumSpotLights; i++)
    {
        const SpotLight light = SpotLights[i];
        // first do fast discard sphere test
        if (TestAABBSphere(aabb, light.position.xyz, light.position.w))
        {
            // then do more refined cone test, if previous test passed
            if (TestAABBCone(aabb, light.position.xyz, light.forward.xyz, light.position.w, light.angleSinCos))
            {
                SpotLightIndexList[index1D * MAX_LIGHTS_PER_CLUSTER + numLights] = i;
                numLights++;
            }
        }       
    }
    SpotLightCountList[index1D] = numLights;

    // update feature flags if we have any lights
    if (numLights > 0)
        flags |= CLUSTER_SPOTLIGHT_BIT;

    atomicOr(AABBs[index1D].featureFlags, flags);
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 64
shader
void csDebug()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;

    // convert screen coord to view-space position
    vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth, InvProjection);

    uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
    uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

    uint flag = AABBs[idx].featureFlags; // add 0 so we can read the value
    vec4 color = vec4(0, 0, 0, 0);
    if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
    {
        uint count = PointLightCountList[idx];
        color.r = count / float(NumPointLights);
    }
    if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
    {
        uint count = SpotLightCountList[idx];
        color.g = count / float(NumSpotLights);
    }
    
    imageStore(DebugOutput, int2(coord), color); 
}

//------------------------------------------------------------------------------
/**
*/
program Cull [ string Mask = "Cull"; ]
{
    ComputeShader = csCull();
};

//------------------------------------------------------------------------------
/**
*/
program Debug [ string Mask = "Debug"; ]
{
    ComputeShader = csDebug();
};
