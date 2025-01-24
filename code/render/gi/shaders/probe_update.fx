//------------------------------------------------------------------------------
//  @file probe_update.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include <lib/pbr.fxh>
#include <lib/ddgi.fxh>

group(SYSTEM_GROUP) write rgba32f image2D RadianceOutput;



#include "probe_shared.fxh"

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
    const float ValueThreshold = 1 / 255.0f;
    if (DDGIMaxComponent(radiance) <= ValueThreshold)
        radiance = vec3(0);
    radiance *= IrradianceScale;
#ifdef USE_COMPRESSED_RADIANCE
    imageStore(RadianceOutput, coordinate, vec4(
            uintBitsToFloat(
                PackFloat3ToUInt(
                    clamp(radiance, vec3(0.0f), vec3(1.0f))
                )
            ), depth, 0.0f, 0.0f));
#else
    imageStore(RadianceOutput, coordinate, vec4(radiance, depth));
#endif
}

//------------------------------------------------------------------------------
/**
*/
shader void
RayGen(
    [ray_payload] out LightResponsePayload payload
)
{
    //payload.radiance = vec3(0);
    payload.normal = vec3(0, 1, 0);
    int rayIndex = int(gl_LaunchIDEXT.x);
    int probeIndex = int(gl_LaunchIDEXT.y);

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
        if (state == PROBE_STATE_INACTIVE && rayIndex >= DDGI_NUM_FIXED_RAYS)
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
    
    vec3 probeRayDirection = DDGIGetProbeDirection(rayIndex, TemporalRotation, Options); 

    const float MaxDistance = 10000.0f;
    payload.bits = 0x0;
    payload.albedo = vec3(0);
    payload.material = vec4(0);
    payload.radiance = vec3(0);
    payload.alpha = 0.0f;
    payload.normal = vec3(0);
    payload.depth = 0;

    traceRayEXT(TLAS, gl_RayFlagsNoneEXT , 0xff, 0, 0, 0, probeWorldPosition, 0.01f, probeRayDirection, MaxDistance, 0);
    if ((payload.bits & RAY_MISS_BIT) != 0)
    {
        vec3 lightDir = normalize(GlobalLightDirWorldspace.xyz);
        vec3 dir = normalize(probeRayDirection);
        vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
        StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), atmo, 1e27f);
        return;
    }
    
    // If hit is back face and it's not a 2 sided material two-sided material
    if ((payload.bits & (RAY_BACK_FACE_BIT | RAY_MATERIAL_TWO_SIDED_BIT)) == RAY_BACK_FACE_BIT)
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
    
    // TODO: Calculate light
    // If all conditions fall through, light the probe
    vec3 probeLighting = vec3(0);
    vec3 albedo = payload.albedo - payload.albedo * payload.material[MAT_METALLIC];
    
    GIVolume volumeArg;
    volumeArg.Offset = Offset;
    volumeArg.Rotation = Rotation;
    volumeArg.GridCounts = ProbeGridDimensions;
    volumeArg.GridSpacing = ProbeGridSpacing;
    volumeArg.ScrollOffsets = ProbeScrollOffsets;
    volumeArg.NumIrradianceTexels = NumIrradianceTexels;
    volumeArg.NumDistanceTexels = NumDistanceTexels;
    volumeArg.EncodingGamma = IrradianceGamma;
    volumeArg.Irradiance = ProbeIrradiance;
    volumeArg.Distances = ProbeDistances;
    volumeArg.Offsets = ProbeOffsets;
    volumeArg.States = ProbeStates;
    volumeArg.NormalBias = NormalBias;
    volumeArg.ViewBias = ViewBias;
    volumeArg.IrradianceScale = IrradianceScale; 
    volumeArg.Options = Options;
    
    vec3 worldSpacePos = probeWorldPosition + probeRayDirection * payload.depth;
    
    vec3 light = payload.radiance;

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
    
    StoreRadianceAndDepth(ivec2(gl_LaunchIDEXT.xy), max(vec3(0), light + probeLighting), payload.depth);
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in LightResponsePayload payload
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
