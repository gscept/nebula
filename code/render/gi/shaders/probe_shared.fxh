//------------------------------------------------------------------------------
//  @file probe_shared.fxh
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef PROBE_SHARED_FXH
#define PROBE_SHARED_FXH
#include <lib/std.fxh>
group(SYSTEM_GROUP) constant VolumeConstants
{
    mat4x4 TemporalRotation;

    vec3 Scale;
    uint Options;

    vec3 Offset;
    int NumIrradianceTexels;

    ivec3 ProbeGridDimensions;
    int ProbeIndexStart;

    ivec3 ProbeScrollOffsets;
    int ProbeIndexCount;

    vec4 Rotation;

    vec3 ProbeGridSpacing;
    int NumDistanceTexels;

    vec4 MinimalDirections[32];
    vec4 ExtraDirections[1024-32];
    vec4 Directions[1024];

    float InverseGammaEncoding;
    float Hysteresis;
    float IrradianceGamma;
    uint RaysPerProbe;
    
    float NormalBias;
    float ViewBias;
    float IrradianceScale;
    float DistanceExponent;
    
    float ChangeThreshold;
    float BrightnessThreshold;
    float BackfaceThreshold;
    float ProbeDistanceScale;

    float MinFrontfaceDistance;
    uint ProbeIrradiance;
    uint ProbeDistances;
    uint ProbeOffsets;

    uint ProbeStates;
    uint ProbeScrollSpace;
    uint ProbeRadiance;
    
    float DebugSize;
};

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIGetProbeDirection(int rayIndex, mat4x4 rotation, uint options)
{
    vec3 direction;
    if ((options & (RELOCATION_OPTION | CLASSIFICATION_OPTION)) != 0)
    {
        bool useFixedRays = rayIndex < DDGI_NUM_FIXED_RAYS;
        uint  adjustedRayIndex = useFixedRays ? rayIndex : rayIndex - DDGI_NUM_FIXED_RAYS;
        direction = useFixedRays ? MinimalDirections[adjustedRayIndex].xyz : ExtraDirections[adjustedRayIndex].xyz;
        if (useFixedRays)
        {
            return direction;
        }
    }
    else
    {
        direction = Directions[rayIndex].xyz;
    }
    return normalize((rotation * vec4(direction, 0)).xyz);
}

//------------------------------------------------------------------------------
/**
*/
uvec2
DDGIThreadBaseCoord(int probeIndex, ivec3 probeGridCounts, int probeNumTexels)
{
    int probesPerPlane = DDGIProbesPerPlane(probeGridCounts);
    int planeIndex = probeIndex / probesPerPlane;
    int probeIndexInPlane = probeIndex % probesPerPlane;
    
    int planeWidthInProbes = probeGridCounts.x;
    
    ivec2 probeCoordInPlane = ivec2(probeIndexInPlane % planeWidthInProbes, probeIndexInPlane / planeWidthInProbes);
    int baseCoordX = (planeWidthInProbes * planeIndex + probeCoordInPlane.x) * probeNumTexels;
    int baseCoordY = probeCoordInPlane.y * probeNumTexels;
    return uvec2(baseCoordX, baseCoordY);
}

#endif // PROBE_SHARED_FXH
