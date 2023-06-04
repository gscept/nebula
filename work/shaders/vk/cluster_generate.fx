//------------------------------------------------------------------------------
//  cluster_generate.fx
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"

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

    // Calculate near and far plane in the XY plane, offset at our Z offset
    vec4 nearPlane  = vec4(0, 0, 1.0f, -FocalLengthNearFar.z * pow(ZDistribution, index3D.z / float(NumCells.z)));
    vec4 farPlane   = vec4(0, 0, 1.0f, -FocalLengthNearFar.z * pow(ZDistribution, (index3D.z + 1) / float(NumCells.z)));

    // Transform the corners to view space
    vec4 minCorner = PixelToView(index3D.xy * vec2(BlockSize) * InvFramebufferDimensions, 1, InvProjection);
    vec4 maxCorner = PixelToView((index3D.xy + ivec2(1.0f)) * vec2(BlockSize) * InvFramebufferDimensions, 1, InvProjection);

    // Trace a ray from the eye (origin) towards the four corner points
    vec3 nearMin, nearMax, farMin, farMax;
    vec3 eye = vec3(0);
    IntersectLineWithPlane(eye, minCorner.xyz, nearPlane, nearMin);
    IntersectLineWithPlane(eye, maxCorner.xyz, nearPlane, nearMax);
    IntersectLineWithPlane(eye, minCorner.xyz, farPlane, farMin);
    IntersectLineWithPlane(eye, maxCorner.xyz, farPlane, farMax);

    // Calculate AABB using min and max
    ClusterAABB aabb;
    aabb.minPoint = vec4(min(nearMin, min(nearMax, min(farMin, farMax))), 1);
    aabb.maxPoint = vec4(max(nearMin, max(nearMax, max(farMin, farMax))), 1);
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
program AABBGenerate [ string Mask = "AABBGenerate"; ]
{
    ComputeShader = csClusterAABB();
};
