//------------------------------------------------------------------------------
//  @file probefinalize.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include "ddgi.fxh"

group(SYSTEM_GROUP) write r11g11b10f image2D IrradianceOutput;
group(SYSTEM_GROUP) write rg16f image2D DistanceOutput;
group(SYSTEM_GROUP) read_write r8 image2D ScrollSpaceOutput;

group(SYSTEM_GROUP) constant FinalizeConstants
{
    uint Options;
    vec3 Directions[1024];
    uint RaysPerProbe;
    ivec3 ProbeGridDimensions;
    int ProbeIndexStart;
    ivec3 ProbeScrollOffsets;
    int ProbeIndexCount;
    ivec3 ProbeGridSpacing;
    int NumTexelsPerProbe;
    float IrradianceGamma;
    float NormalBias;
    float ViewBias;
    float IrradianceScale;
    float DistanceExponent;
    
    
    uint ProbeIrradiance;
    uint ProbeDistances;
    uint ProbeOffsets;
    uint ProbeStates;
};

groupshared vec3 Radiance[1024];
groupshared float Distance[1024];
groupshared vec3 Direction[1024];

//------------------------------------------------------------------------------
/**
*/
vec3
UnpackUIntToFloat3(uint val)
{
    vec3 unpacked;
    unpacked.x = float((val & 0x000003FF)) / 1023.0f;
    unpacked.y = float(((val >> 10) & 0x000003FF)) / 1023.0f;
    unpacked.z = float(((val >> 20) & 0x000003FF)) / 1023.0f;
    return unpacked;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = NUM_IRRADIANCE_TEXELS_PER_PROBE
[local_size_y] = NUM_IRRADIANCE_TEXELS_PER_PROBE
[local_size_z] = 1
shader void
ProbeFinalizeRadiance()
{
    bool earlyOut = false;
    vec4 result = vec4(0); 
    
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    int probeIndex = DDGIProbeIndex(texel, ProbeGridDimensions);
    int groupIndex = int(gl_WorkGroupID.x + gl_WorkGroupID.y * NUM_IRRADIANCE_TEXELS_PER_PROBE + gl_WorkGroupID.z * NUM_IRRADIANCE_TEXELS_PER_PROBE * NUM_IRRADIANCE_TEXELS_PER_PROBE);
    if (probeIndex < 0)
    {
        earlyOut = true;
    }
    
    uvec3 prevProbeSpace;
    uvec3 probeSpace = uvec3(0);
    if ((Options & SCROLL_OPTION) != 0)
    {
        int storageProbeIndex = DDGIProbeIndexOffset(probeIndex, ProbeGridDimensions, ProbeScrollOffsets);
        ivec2 texelPosition = DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions);
        
        prevProbeSpace.x = floatBitsToUint(imageLoad(ScrollSpaceOutput, texelPosition).x) & 0x01;
        prevProbeSpace.y = (floatBitsToUint(imageLoad(ScrollSpaceOutput, texelPosition).x) & 0x02) >> 1;
        prevProbeSpace.z = (floatBitsToUint(imageLoad(ScrollSpaceOutput, texelPosition).x) & 0x01) >> 2;
        
        ivec3 probeGridCoord = DDGIProbeCoords(probeIndex, ProbeGridDimensions);
        probeSpace = (probeGridCoord + ProbeScrollOffsets) / ProbeGridDimensions;
        probeSpace = probeSpace % 2;
        
        if ((Options & PARTIAL_UPDATE_OPTION) != 0)
        {
            if (prevProbeSpace.x == probeSpace.x && prevProbeSpace.y == probeSpace.y && prevProbeSpace.z == probeSpace.z)
                earlyOut = true;
        }
    }
    
    if ((Options & PARTIAL_UPDATE_OPTION) != 0)
    {
        if (!earlyOut)
        {
            int numProbes = ProbeGridDimensions.x * ProbeGridDimensions.y * ProbeGridDimensions.z;
            int probeRRIndex = (probeIndex < ProbeIndexStart) ? probeIndex + numProbes : probeIndex;
            if (probeRRIndex >= ProbeIndexStart + ProbeIndexCount)
                earlyOut = true;
        }
    }
    
    uvec2 probeTexCoords = uvec2(0);
    int storageProbeIndex;
    if (!earlyOut)
    {
        if ((Options & SCROLL_OPTION) != 0)
        {
            storageProbeIndex = DDGIProbeIndexOffset(probeIndex, ProbeGridDimensions, ProbeScrollOffsets);
            uvec2 intraProbeTexelOffset = gl_GlobalInvocationID.xy % uvec2(NumTexelsPerProbe, NumTexelsPerProbe);
            probeTexCoords = DDGIThreadBaseCoord(storageProbeIndex, ProbeGridDimensions, NumTexelsPerProbe) + intraProbeTexelOffset;
            probeTexCoords.xy = probeTexCoords.xy + uvec2(1) + (probeTexCoords.xy / NumTexelsPerProbe) * 2;
        }
        else
        {
            storageProbeIndex = probeIndex;
            probeTexCoords = gl_GlobalInvocationID.xy + uvec2(1);
            probeTexCoords.xy += (gl_GlobalInvocationID.xy / NumTexelsPerProbe) * 2;
        }
        
        if ((Options & CLASSIFICATION_OPTION) != 0)
        {
            ivec2 texelPosition = DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions);
            int probeState = floatBitsToInt(fetch2D(ProbeStates, Basic2DSampler, texelPosition, 0).x);
            if (probeState == PROBE_STATE_INACTIVE)
            {
                earlyOut = true;
            }
        }
    }
    
    vec2 probeOctantUV = vec2(0);
    probeOctantUV = DDGINormalizedOctahedralCoordinates(texel, NumTexelsPerProbe);
    vec3 probeRayDirection = DDGIOctahedralDirection(probeOctantUV);
    
    if (!earlyOut)
    {
        int totalIterations = int(ceil(float(RaysPerProbe) / float(NumTexelsPerProbe * NumTexelsPerProbe)));
        for (int iteration = 0; iteration < totalIterations; iteration++)
        {
            int rayIndex = groupIndex * totalIterations + iteration;
            if (rayIndex >= RaysPerProbe)
                break;
                
            vec2 value = fetch2D(ProbeIrradiance, Basic2DSampler, ivec2(rayIndex, probeIndex), 0).xy;
            Radiance[rayIndex] = UnpackUIntToFloat3(floatBitsToUint(value.x));
            Distance[rayIndex] = value.y;
            Direction[rayIndex] = Directions[rayIndex];
        }
    }
    
    groupMemoryBarrier();
    
    if (earlyOut)
    {
        return;
    }
    
    uint backfaces = 0;
    uint maxBackfaces = RaysPerProbe * 0.1f;
    int rayIndex = 0;
    if ((Options & CLASSIFICATION_OPTION | RELOCATION_OPTION) == CLASSIFICATION_OPTION | RELOCATION_OPTION)
    {
        rayIndex = NUM_FIXED_RAYS;
    }
    
    for (; rayIndex < RaysPerProbe; rayIndex++)
    {
        vec3 rayDirection = Direction[rayIndex];
        float weight = max(0.0f, dot(probeRayDirection, rayDirection));
        ivec2 probeRayIndex = ivec2(rayIndex, probeIndex);
        
        vec3 probeRayRadiance = Radiance[rayIndex];
        float probeRayDistance = Distance[rayIndex];
        
        if (probeRayDistance < 0.0f)
        {
            backfaces++;
            if (backfaces >= maxBackfaces)
                return;
            continue;
        }
        
        result += vec4(probeRayRadiance * weight, weight);
        
        float probeMaxRayDistance = length(ProbeGridSpacing) * 1.5f;
        weight = pow(weight, DistanceExponent);
        
        float probeRayDistance = min(abs(Distance[rayIndex]), probeMaxRayDistance);
        
        result += vec4(probeRayDistance * weight, (probeRayDistance * probeRayDistance), 0.0f, weight);
    }
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = NUM_DISTANCE_TEXELS_PER_PROBE
[local_size_y] = NUM_DISTANCE_TEXELS_PER_PROBE
[local_size_z] = 1
shader void
ProbeFinalizeDistance()
{
}

//------------------------------------------------------------------------------
/**
*/
program RadianceFinalize[string Mask = "ProbeFinalizeRadiance"; ]
{
    ComputeShader = ProbeFinalizeRadiance();
};

//------------------------------------------------------------------------------
/**
*/
program DistanceFinalize[string Mask = "ProbeFinalizeDistance"; ]
{
    ComputeShader = ProbeFinalizeDistance();
};
