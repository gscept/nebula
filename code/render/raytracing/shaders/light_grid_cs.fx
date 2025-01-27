//------------------------------------------------------------------------------
//  @file light_grid_cs.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/shared.fxh>
#include <lib/clustering.fxh>

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
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
        if (TestAABBSphere(aabb, light.position, light.range))
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
        if (TestAABBSphere(aabb, light.position, light.range))
        {
            // then do more refined cone test, if previous test passed
            if (TestAABBCone(aabb, light.position, light.forward, light.range, light.angleSinCos))
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

    numLights = 0;
    for (uint i = 0; i < NumAreaLights; i++)
    {
        const AreaLight light = AreaLights[i];
        vec3 worldSpaceMin = light.center - light.extents;
        vec3 worldSpaceMax = light.center + light.extents;
        if (TestAABBAABB(aabb, worldSpaceMin, worldSpaceMax))
        {
            AreaLightIndexList[index1D * MAX_LIGHTS_PER_CLUSTER + numLights] = i;
            numLights++;
        }
    }
    AreaLightCountList[index1D] = numLights;

    if (numLights > 0)
        flags |= CLUSTER_AREALIGHT_BIT;

    atomicOr(AABBs[index1D].featureFlags, flags);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void csClusterAABB()
{
    uint index1D = gl_GlobalInvocationID.x;
    uvec3 index3D = Unpack1DTo3D(index1D, NumCells.x, NumCells.y);

    if (index1D > NumCells.x * NumCells.y * NumCells.z)
        return;

    vec3 offsetMin = vec3(index3D.x - NumCells.x * 0.5f, index3D.y - NumCells.y * 0.5f, index3D.z - NumCells.z * 0.5f) * BlockSize.y;
    vec3 offsetMax = vec3((index3D.x + 1) - NumCells.x * 0.5f, (index3D.y + 1) - NumCells.y * 0.5f, (index3D.z + 1) - NumCells.z * 0.5f) * BlockSize.y;

    // Calculate AABB using min and max
    ClusterAABB aabb;
    vec3 eye = EyePos.xyz;
    aabb.minPoint = vec4(eye.xyz + offsetMin, 1);
    aabb.maxPoint = vec4(eye.xyz + offsetMax, 1);
    vec3 extents = (aabb.maxPoint.xyz - aabb.minPoint.xyz) * 0.5f;
    float radius = dot(extents, extents);
    aabb.featureFlags = 0;
    aabb.minPoint.w = radius;
    aabb.maxPoint.w = sqrt(radius);
    AABBs[index1D] = aabb;
}

//------------------------------------------------------------------------------
/**
*/
program Cull[string Mask = "Cull";]
{
    ComputeShader = csCull();
};

program AABBGenerate[string Mask = "AABBGenerate";]
{
    ComputeShader = csClusterAABB();
};
