//------------------------------------------------------------------------------
//  @file probefinalize.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include "ddgi.fxh"

group(SYSTEM_GROUP) read_write r11g11b10f image2D IrradianceOutput;
group(SYSTEM_GROUP) read_write rg16f image2D DistanceOutput;
group(SYSTEM_GROUP) read_write r8 image2D ScrollSpaceOutput;

group(SYSTEM_GROUP) constant BlendConstants
{
    vec3 Directions[1024];
    uint Options;
    uint RaysPerProbe;
    ivec3 ProbeGridDimensions;
    int ProbeIndexStart;
    ivec3 ProbeScrollOffsets;
    int ProbeIndexCount;
    ivec3 ProbeGridSpacing;
    
    float InverseGammaEncoding;
    float Hysteresis;
    float NormalBias;
    float ViewBias;
    
    float IrradianceScale;
    float DistanceExponent;
    float ChangeThreshold;
    float BrightnessThreshold;
    
    uint ProbeIrradiance;
    uint ProbeDistances;
    uint ProbeOffsets;
    uint ProbeStates;
};

groupshared vec3 Radiance[1024];
groupshared float Distance[1024];
groupshared vec3 Direction[1024];

const uint RADIANCE_MODE = 0;
const uint DISTANCE_MODE = 1;

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
    Finds the smallest component of the vector.
*/
float 
DDGIMinComponent(vec3 a)
{
    return min(a.x, min(a.y, a.z));
}

//------------------------------------------------------------------------------
/**
    Finds the largest component of the vector.
*/
float
DDGIMaxComponent(vec3 a)
{
    return max(a.x, max(a.y, a.z));
}

//------------------------------------------------------------------------------
/**
*/
vec3
RadianceHysteresis(vec3 previous, vec3 current, float hysteresis)
{
    if (DDGIMaxComponent(previous - current) > ChangeThreshold)
    {
        hysteresis = max(0.0f, hysteresis - 0.75f);
    }
    
    vec3 delta = (current - previous);
    if (length(delta) > BrightnessThreshold)
    {
        current = previous + (delta * 0.25f);
    } 
    const vec3 ConstThreshold = vec3(1.0f / 1024.0f);
    vec3 lerpDelta = (1.0f - hysteresis) * delta;
    if (DDGIMaxComponent(current) < DDGIMaxComponent(previous))
    {
        lerpDelta = min(max(ConstThreshold, abs(lerpDelta)), abs(delta)) * sign(lerpDelta);
    }
    return previous + lerpDelta;
}

//------------------------------------------------------------------------------
/**
*/
void
Blend(const uint MODE)
{
    bool earlyOut = false;
    const int NUM_TEXELS_PER_PROBE = MODE == RADIANCE_MODE ? NUM_IRRADIANCE_TEXELS_PER_PROBE : NUM_DISTANCE_TEXELS_PER_PROBE;
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
    ivec2 texelPosition;
    if ((Options & SCROLL_OPTION) != 0)
    {
        int storageProbeIndex = DDGIProbeIndexOffset(probeIndex, ProbeGridDimensions, ProbeScrollOffsets);
        texelPosition = DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions);
        
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
            uvec2 intraProbeTexelOffset = gl_GlobalInvocationID.xy % uvec2(NUM_TEXELS_PER_PROBE, NUM_TEXELS_PER_PROBE);
            probeTexCoords = DDGIThreadBaseCoord(storageProbeIndex, ProbeGridDimensions, NUM_TEXELS_PER_PROBE) + intraProbeTexelOffset;
            probeTexCoords.xy = probeTexCoords.xy + uvec2(1) + (probeTexCoords.xy / NUM_TEXELS_PER_PROBE) * 2;
        }
        else
        {
            storageProbeIndex = probeIndex;
            probeTexCoords = gl_GlobalInvocationID.xy + uvec2(1);
            probeTexCoords.xy += (gl_GlobalInvocationID.xy / NUM_TEXELS_PER_PROBE) * 2;
        }
        
        if ((Options & CLASSIFICATION_OPTION) != 0)
        {
            texelPosition = DDGIProbeTexelPosition(storageProbeIndex, ProbeGridDimensions);
            int probeState = floatBitsToInt(fetch2D(ProbeStates, Basic2DSampler, texelPosition, 0).x);
            if (probeState == PROBE_STATE_INACTIVE)
            {
                earlyOut = true;
            }
        }
    }
    
    vec2 probeOctantUV = vec2(0);
    probeOctantUV = DDGINormalizedOctahedralCoordinates(texel, NUM_TEXELS_PER_PROBE);
    vec3 probeRayDirection = DDGIOctahedralDirection(probeOctantUV);
    
    if (!earlyOut)
    {
        int totalIterations = int(ceil(float(RaysPerProbe) / float(NUM_TEXELS_PER_PROBE * NUM_TEXELS_PER_PROBE)));
        for (int iteration = 0; iteration < totalIterations; iteration++)
        {
            int rayIndex = groupIndex * totalIterations + iteration;
            if (rayIndex >= RaysPerProbe)
                break;
                
            vec2 value = fetch2D(ProbeIrradiance, Basic2DSampler, ivec2(rayIndex, probeIndex), 0).xy;
            if (MODE == RADIANCE_MODE)
                Radiance[rayIndex] = UnpackUIntToFloat3(floatBitsToUint(value.x));
            Distance[rayIndex] = value.y;
            Direction[rayIndex] = Directions[rayIndex]; // TODO: Add rotation
        }
    }
    
    groupMemoryBarrier();
    
    if (earlyOut)
    {
        return;
    }
    
    uint backfaces;
    uint maxBackfaces;
    if (MODE == RADIANCE_MODE)
    {
        backfaces = 0;
        maxBackfaces = uint(RaysPerProbe * 0.1f);
    }
    
    int rayIndex = 0;
    if ((Options & (CLASSIFICATION_OPTION | RELOCATION_OPTION)) == (CLASSIFICATION_OPTION | RELOCATION_OPTION))
    {
        rayIndex = NUM_FIXED_RAYS;
    }
    
    for (; rayIndex < RaysPerProbe; rayIndex++)
    {
        vec3 rayDirection = Direction[rayIndex];
        float weight = max(0.0f, dot(probeRayDirection, rayDirection));
        ivec2 probeRayIndex = ivec2(rayIndex, probeIndex);
        
        if (MODE == RADIANCE_MODE)
        {
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
        }
        else
        {
            float probeMaxRayDistance = length(ProbeGridSpacing) * 1.5f;
            weight = pow(weight, DistanceExponent);
            
            float probeRayDistance = min(abs(Distance[rayIndex]), probeMaxRayDistance);
            
            result += vec4(probeRayDistance * weight, (probeRayDistance * probeRayDistance), 0.0f, weight);
        }
    }
    
    const float epsilon = 1e-9f * float(RaysPerProbe);
    result.rgb *= 1.0f / max(2.0f * result.a, epsilon);
    
    
    if (MODE == DISTANCE_MODE)
    {
        uint probeSpacePacked = 0;
        probeSpacePacked = probeSpace.x;
        probeSpacePacked = probeSpace.y << 1;
        probeSpacePacked = probeSpace.z << 2;
        imageStore(ScrollSpaceOutput, texelPosition, vec4(uintBitsToFloat(probeSpacePacked), 0, 0, 0));
    }
    
    vec3 previous;
    if (MODE == RADIANCE_MODE)
        previous = imageLoad(IrradianceOutput, ivec2(probeTexCoords)).rgb;
    else
        previous = imageLoad(DistanceOutput, ivec2(probeTexCoords)).rgb;
    float hysteresis = Hysteresis;
    
    if (MODE == RADIANCE_MODE)
    {
        result.rgb = pow(result.rgb, vec3(InverseGammaEncoding));
        
        if ((Options & SCROLL_OPTION) != 0)
        {
            if (probeSpace.x == prevProbeSpace.x && probeSpace.y == prevProbeSpace.y && probeSpace.z == prevProbeSpace.z)
            {
                result = vec4(RadianceHysteresis(previous, result.rgb, hysteresis), 1.0f);
            }
        }
        else
        {
            result = vec4(RadianceHysteresis(previous, result.rgb, hysteresis), 1.0f);
        }
    }
    else
    {
        if ((Options & SCROLL_OPTION) != 0)
        {
            if (probeSpace.x == prevProbeSpace.x && probeSpace.y == prevProbeSpace.y && probeSpace.z == prevProbeSpace.z)
            {
                result = vec4(mix(result.rg, previous.rg, hysteresis), 0.0f, 1.0f);
            }
        }
        else
        {
            result = vec4(mix(result.rg, previous.rg, hysteresis), 0.0f, 1.0f);
        }
    }
    
    if (MODE == RADIANCE_MODE)
        imageStore(IrradianceOutput, ivec2(probeTexCoords), result);
    else
        imageStore(DistanceOutput, ivec2(probeTexCoords), result);
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
    Blend(RADIANCE_MODE);
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
    Blend(DISTANCE_MODE);
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
