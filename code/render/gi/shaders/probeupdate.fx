//------------------------------------------------------------------------------
//  @file probeupdate.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include <lib/pbr.fxh>
#include "ddgi.fxh"

group(SYSTEM_GROUP) write rg32f image2D RadianceOutput;

const uint NumColorSamples = 16;
const uint NumDepthSamples = 8;

struct Probe
{
    vec3 position;
};

group(SYSTEM_GROUP) rw_buffer ProbeBuffer
{
    Probe Probes[];
};

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
    ivec3 ProbeGridSpacing;
    int NumDistanceTexels;
    vec3 MinimalDirections[32];
    float IrradianceGamma;
    vec3 Directions[1024];
    float NormalBias;
    float ViewBias;
    float IrradianceScale;
    
    uint ProbeIrradiance;
    uint ProbeDistances;
    uint ProbeOffsets;
    uint ProbeStates;
};

//------------------------------------------------------------------------------
/**
*/
ivec2
GetOutputPixel(uvec2 rayIndex, uint numSamples) 
{
    uint row = rayIndex.x / numSamples;
    uint column = (rayIndex.x % numSamples) * numSamples + rayIndex.y;
    return ivec2(row, column); 
}

//------------------------------------------------------------------------------
/**
*/
ivec2 
GetProbeTexel(int probe, int3 probeGridDimensions)
{
    return ivec2(probe % (probeGridDimensions.x * probeGridDimensions.y), probe / (probeGridDimensions.x * probeGridDimensions.y));
}

//------------------------------------------------------------------------------
/**
*/
vec3
GetProbeDirection(int rayIndex, mat4x4 rotation, uint options)
{
    vec3 direction;
    if ((options & (RELOCATION_OPTION | CLASSIFICATION_OPTION)) != 0)
    {
        bool useFixedRays = rayIndex < NUM_FIXED_RAYS;
        int adjustedRayIndex = useFixedRays ? rayIndex : rayIndex - NUM_FIXED_RAYS;
        direction = useFixedRays ? MinimalDirections[adjustedRayIndex] : Directions[adjustedRayIndex];
        if (useFixedRays)
        {
            return direction;
        }
    }
    else
    {
        direction = Directions[rayIndex];
    }
    return normalize((rotation * vec4(direction, 0)).xyz);
}

//------------------------------------------------------------------------------
/**
*/
uint
FloatToUInt(float v, float scale)
{
    return uint(floor(v * scale + 0.5f));
}

//------------------------------------------------------------------------------
/**
*/
uint
PackFloat3ToUInt(vec3 val)
{
    return FloatToUInt(val.x, 1023.0f) | (FloatToUInt(val.y, 1023.0f) << 10) | (FloatToUInt(val.z, 1023.0f) << 20);
}

//------------------------------------------------------------------------------
/**
*/
void
StoreRadianceAndDepth(ivec2 coordinate, vec3 radiance, float depth)
{
    radiance *= IrradianceScale;
    imageStore(RadianceOutput, coordinate, float4(
        uintBitsToFloat(
            PackFloat3ToUInt(
                clamp(radiance, vec3(0.0f), vec3(1.0f))
            )
        ), depth, 0.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
shader void
RayGen(
    [ray_payload] out HitResult payload
)
{
    //payload.radiance = vec3(0);
    payload.normal = vec3(0, 1, 0);
    int rayIndex = int(gl_LaunchIDEXT.x);
    int probeIndex = int(gl_LaunchIDEXT.y);

    Probe probe = Probes[probeIndex];
    vec3 direction = normalize((vec4(Directions[rayIndex], 0) * Rotation).xyz);
    
    int storageProbeIndex;
    if ((Options & SCROLL_OPTION) != 0)
    {
        storageProbeIndex = DDGIProbeIndexOffset(probeIndex, ProbeGridDimensions, ProbeScrollOffsets);
    }
    else
    {
        storageProbeIndex = probeIndex;
    }
    ivec2 texelPosition = DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions);
    
    int numProbes = ProbeGridDimensions.x * ProbeGridDimensions.y * ProbeGridDimensions.z;
    int probeRRIndex = (probeIndex < ProbeIndexStart) ? probeIndex + numProbes : probeIndex;
    if (probeRRIndex >= ProbeIndexStart + ProbeIndexCount)
        return;

    int state = floatBitsToInt(fetch2D(ProbeStates, Basic2DSampler, texelPosition, 0).x);
    if ((Options & CLASSIFICATION_OPTION) != 0)
    {
        if (state == PROBE_STATE_INACTIVE && rayIndex >= NUM_FIXED_RAYS)
        {
            return;
        }
    }
    
    vec3 probeWorldPosition;
    if ((Options & RELOCATION_OPTION) != 0)
    {
        if ((Options & SCROLL_OPTION) != 0)
        {
            probeWorldPosition = DDGIProbeWorldPositionWithScrollAndOffset(probeIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing, ProbeScrollOffsets, ProbeOffsets);
        }
        else
        {
            probeWorldPosition = DDGIProbeWorldPositionWithOffset(probeIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing, ProbeOffsets);
        }
    }
    else
    {
        probeWorldPosition = DDGIProbeWorldPosition(probeIndex, Offset, Rotation, ProbeGridDimensions, ProbeGridSpacing);
    }
    
    vec3 probeRayDirection = GetProbeDirection(rayIndex, TemporalRotation, Options); 

    const float MaxDistance = 10000.0f;
    payload.bits = 0x0;
    
    vec3 radiance = vec3(0,0,0);

    traceRayEXT(TLAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, probeWorldPosition, 0.01f, probeRayDirection, MaxDistance, 0);
    if ((payload.bits & RAY_MISS_BIT) != 0)
    {
        vec3 lightDir = normalize(GlobalLightDirWorldspace.xyz);
        vec3 dir = normalize(direction);
        vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
        radiance += atmo;
        StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), atmo, 1e27f);
        return;
    }
    
    // If hit is back face but is a two-sided material
    if ((payload.bits & (RAY_BACK_FACE_BIT | RAY_MATERIAL_TWO_SIDED_BIT)) == (RAY_BACK_FACE_BIT | RAY_MATERIAL_TWO_SIDED_BIT))
    {
        StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), vec3(0), -payload.depth * 0.2f);
        return;

    }
    
    // If probe is inactive, store depth in case it needs to be reactivated
    if ((Options & CLASSIFICATION_OPTION) != 0 && state == PROBE_STATE_INACTIVE)
    {
        StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), vec3(0), payload.depth);
        return;
    }
    
    // If all conditions fall through, light the probe
    vec3 probeLighting = vec3(0);
    vec3 albedo = payload.albedo - payload.albedo * payload.material[MAT_METALLIC];
    
    DDGIVolume volumeArg;
    volumeArg.origin = Offset;
    volumeArg.rotationQuat = Rotation;
    volumeArg.probeGridCounts = ProbeGridDimensions;
    volumeArg.probeGridSpacing = ProbeGridSpacing;
    volumeArg.probeScrollOffsets = ProbeScrollOffsets;
    volumeArg.numIrradianceTexels = NumIrradianceTexels;
    volumeArg.numDistanceTexels = NumDistanceTexels;
    volumeArg.probeIrradianceGamma = IrradianceGamma;
    volumeArg.irradianceHandle = ProbeIrradiance;
    volumeArg.distanceHandle = ProbeDistances;
    volumeArg.offsetsHandle = ProbeOffsets;
    volumeArg.statesHandle = ProbeStates; 
    
    vec3 worldSpacePos = probeWorldPosition + probeRayDirection * payload.depth;
        
    vec3 relativePos = abs(worldSpacePos - Offset);
    if (relativePos.x > Scale.x || relativePos.y > Scale.y || relativePos.z > Scale.z)
    {
        probeLighting = vec3(0);
    }
    else
    {
        vec3 surfaceBias = DDGISurfaceBias(payload.normal, probeRayDirection, NormalBias, ViewBias);
        vec3 irradiance = EvaluateDDGIIrradiance(worldSpacePos, surfaceBias, payload.normal, volumeArg, Options);
        
        float maxAlbedo = 0.9f;
        probeLighting = irradiance * (min(albedo, maxAlbedo) / PI);
        probeLighting /= IrradianceScale;
    }
    
    StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), probeLighting, payload.depth);
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in HitResult payload
)
{
    payload.bits |= RAY_MISS_BIT;
}


//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "ProbeRayGen"; ]
{
    RayGenerationShader = RayGen();
    RayMissShader = Miss();
};
