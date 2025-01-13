//------------------------------------------------------------------------------
//  @file probe_relocate.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/ddgi.fxh>

group(SYSTEM_GROUP) read_write rgba16f image2D ProbeOffsetsOutput;

#include <probe_shared.fxh>

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 4
[local_size_z] = 1
shader void 
ProbeRelocation()
{
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    int probeIndex = DDGIProbeIndex(texel, ProbeGridDimensions);
    
    int numProbes = ProbeGridDimensions.x * ProbeGridDimensions.y * ProbeGridDimensions.z;
    if (probeIndex >= numProbes)
        return;
    
    int probeRRIndex = (probeIndex < ProbeIndexStart) ? probeIndex + numProbes : probeIndex;
    if (probeRRIndex >= ProbeIndexStart + ProbeIndexCount)
        return;
        
    int storageProbeIndex;
    if ((Options & SCROLL_OPTION) != 0)
        storageProbeIndex = DDGIProbeIndexOffset(probeIndex, ProbeGridDimensions, ProbeScrollOffsets);
    else
        storageProbeIndex = probeIndex;
    
    ivec2 offsetTexelPosition = ivec2(DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions));
    
    vec3 currentOffset = DDGIDecodeProbeOffsets(offsetTexelPosition, ProbeGridSpacing, ProbeOffsets);
    
    int closestBackfaceIndex = -1;
    int closestFrontfaceIndex = -1;
    int farthestFrontfaceIndex = -1;
    float closestBackfaceDistance = 1e27f;
    float closestFrontfaceDistance = 1e27f;
    float farthestFrontfaceDistance = 0;
    float backfaceCount = 0;
    
    int numRays = min(int(RaysPerProbe), DDGI_NUM_FIXED_RAYS);
    
    for (int rayIndex = 0; rayIndex < numRays; rayIndex++)
    {
        ivec2 rayTexCoord = ivec2(rayIndex, probeIndex);
        
        float hitDistance = fetch2D(ProbeRadiance, Basic2DSampler, rayTexCoord, 0).w;

        if (hitDistance < 0.0f)
        {
            backfaceCount++;
            hitDistance = hitDistance * -5.0f;
            if (hitDistance < closestBackfaceDistance)
            {
                closestBackfaceDistance = hitDistance;
                closestBackfaceIndex = rayIndex;
            }
        }
        else
        {
            if (hitDistance < closestFrontfaceDistance)
            {
                closestFrontfaceDistance = hitDistance;
                closestFrontfaceIndex = rayIndex;
            }
            else if (hitDistance > farthestFrontfaceDistance)
            {
                farthestFrontfaceDistance = hitDistance;
                farthestFrontfaceIndex = rayIndex;
            }
        }
    }
    
    vec3 fulloffset = vec3(1e27);
    if (closestBackfaceIndex != -1 && (float(backfaceCount) / numRays) > BackfaceThreshold)
    {
        vec3 closestBackfaceDirection = closestBackfaceDistance * normalize(DDGIGetProbeDirection(closestBackfaceIndex, TemporalRotation, Options));
        fulloffset = currentOffset + closestBackfaceDirection * (ProbeDistanceScale + 1.0f);
    }
    else if (closestFrontfaceDistance < MinFrontfaceDistance)
    {
        vec3 closestFrontfaceDirection = DDGIGetProbeDirection(closestFrontfaceIndex, TemporalRotation, Options);
        vec3 farthestFrontfaceDirection = DDGIGetProbeDirection(farthestFrontfaceIndex, TemporalRotation, Options);
        
        if (dot(closestFrontfaceDirection, farthestFrontfaceDirection) <= 0.f)
        {
            farthestFrontfaceDistance *= min(farthestFrontfaceDistance, 1.0f);
            
            fulloffset = currentOffset + farthestFrontfaceDirection * ProbeDistanceScale;
        }
    }
    else if (closestFrontfaceDistance > MinFrontfaceDistance + ProbeDistanceScale)
    {
        float moveBackMargin = min(closestFrontfaceDistance - MinFrontfaceDistance, length(currentOffset));
        vec3 moveBackDirection = normalize(-currentOffset);
        fulloffset = currentOffset + (moveBackMargin * moveBackDirection);
    }
    
    vec3 normalizedOffset = fulloffset / ProbeGridSpacing;
    if (dot(normalizedOffset, normalizedOffset) < 0.2025f)
    {
        currentOffset = fulloffset;
    }
    
    imageStore(ProbeOffsetsOutput, offsetTexelPosition, vec4(currentOffset / ProbeGridSpacing, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
program ProbeRelocate[string Mask = "ProbeRelocate"; ]
{
    ComputeShader = ProbeRelocation();
};
