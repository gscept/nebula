//------------------------------------------------------------------------------
//  @file probe_finalize.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/ddgi.fxh>

group(SYSTEM_GROUP) read_write r11g11b10f image2D IrradianceOutput;
group(SYSTEM_GROUP) read_write rg16f image2D DistanceOutput;
group(SYSTEM_GROUP) read_write r8 image2D ScrollSpaceOutput;

group(SYSTEM_GROUP) constant BlendConstants
{
    mat4x4 TemporalRotation;
    vec4 Directions[1024];
    uint Options;
    vec4 MinimalDirections[32];
    uint RaysPerProbe;
    ivec3 ProbeGridDimensions;
    int ProbeIndexStart;
    ivec3 ProbeScrollOffsets;
    int ProbeIndexCount;
    vec3 ProbeGridSpacing;
    
    float InverseGammaEncoding;
    float Hysteresis;
    float NormalBias;
    float ViewBias;
    
    float IrradianceScale;
    float DistanceExponent;
    float ChangeThreshold;
    float BrightnessThreshold;
    
    uint ProbeRadiance;
    uint ProbeIrradiance;
    uint ProbeDistances;
    uint ProbeOffsets;
    uint ProbeStates;
};

#include "probe_shared.fxh"

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
vec4
LoadRadiance(ivec2 coords, uint mode)
{
#ifdef USE_COMPRESSED_RADIANCE
    vec2 value = fetch2D(ProbeRadiance, Basic2DSampler, coords, 0).xy;
    vec4 res = vec4(0);
    if (mode == RADIANCE_MODE)
        res.xyz = UnpackUIntToFloat3(floatBitsToUint(value.x));
    res.w = value.y;
    return res;
#else
    vec4 value = fetch2D(ProbeRadiance, Basic2DSampler, coords, 0);
    return value;
#endif
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
    int probeIndex = DDGIProbeIndex(texel, ProbeGridDimensions, NUM_TEXELS_PER_PROBE);
    int groupIndex = int(gl_LocalInvocationIndex);
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
                
            vec4 value = LoadRadiance(ivec2(rayIndex, probeIndex), MODE);
            if (MODE == RADIANCE_MODE)
                Radiance[rayIndex] = value.xyz;
            Distance[rayIndex] = value.w;
            Direction[rayIndex] = DDGIGetProbeDirection(rayIndex, TemporalRotation, Options);
        }
    }
    
    groupMemoryBarrier();
    barrier();
    
    if (earlyOut)
    {
        return;
    }

    int rayIndex = 0;
    if ((Options & (CLASSIFICATION_OPTION | RELOCATION_OPTION)) == (CLASSIFICATION_OPTION | RELOCATION_OPTION))
    {
        rayIndex = DDGI_NUM_FIXED_RAYS;
    }
        
    uint backfaces;
    uint maxBackfaces;
    if (MODE == RADIANCE_MODE)
    {
        backfaces = 0;
        maxBackfaces = uint((RaysPerProbe - rayIndex) * 0.1f);
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
    result.rgb *= 1.0f / max(2.0f * result.w, epsilon);
    
    
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
        result.xyz = pow(result.xyz, vec3(InverseGammaEncoding));

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
                result = vec4(mix(result.xyz, previous.xyz, hysteresis), 1.0f);
            }
        }
        else
        {
            result = vec4(mix(result.xyz, previous.xyz, hysteresis), 1.0f);
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
void
BorderRows(const uint MODE)
{
    const int NUM_TEXELS_PER_PROBE = MODE == RADIANCE_MODE ? NUM_IRRADIANCE_TEXELS_PER_PROBE : NUM_DISTANCE_TEXELS_PER_PROBE;
    
    uint probeSideLength = (NUM_TEXELS_PER_PROBE + 2);
    uint probeSideLengthMinusOne = (probeSideLength - 1);
    
    uvec2 thread = gl_GlobalInvocationID.xy;
    thread.y *= probeSideLength;
    
    int mod = int(gl_GlobalInvocationID.x % probeSideLength);
    if (mod == 0 || mod == probeSideLengthMinusOne)
        return;
        
    uint probeStart = uint(thread.x / probeSideLength) * probeSideLength;
    uint offset = probeSideLengthMinusOne - (thread.x % probeSideLength);
    
    uvec2 copyCoordinates = uvec2(probeStart + offset, (thread.y + 1));
     
    vec4 load;
    if (MODE == RADIANCE_MODE)
    {
        load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
        imageStore(IrradianceOutput, ivec2(thread), load);
        
        thread.y += probeSideLengthMinusOne;
        copyCoordinates = uvec2(probeStart + offset, thread.y - 1);
        
        load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
        imageStore(IrradianceOutput, ivec2(thread), load);
    }
    else if (MODE == DISTANCE_MODE)
    {
        load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
        imageStore(DistanceOutput, ivec2(thread), load);
        
        thread.y += probeSideLengthMinusOne;
        copyCoordinates = uvec2(probeStart + offset, thread.y - 1);
        
        load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
        imageStore(DistanceOutput, ivec2(thread), load);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BorderColumns(const uint MODE)
{
    const int NUM_TEXELS_PER_PROBE = MODE == RADIANCE_MODE ? NUM_IRRADIANCE_TEXELS_PER_PROBE : NUM_DISTANCE_TEXELS_PER_PROBE;
    
    uint probeSideLength = (NUM_TEXELS_PER_PROBE + 2);
    uint probeSideLengthMinusOne = (probeSideLength - 1);
    
    uvec2 thread = gl_GlobalInvocationID.xy;
    thread.x *= probeSideLength;
    
    uvec2 copyCoordinates = uvec2(0);
    
    int mod = int(gl_GlobalInvocationID.y % probeSideLength);
    if (mod == 0 || mod == probeSideLengthMinusOne)
    {
        copyCoordinates.x = thread.x + NUM_TEXELS_PER_PROBE;
        copyCoordinates.y = thread.y - sign(mod - 1) * NUM_TEXELS_PER_PROBE;
        
        vec4 load;
        if (MODE == RADIANCE_MODE)
        {
            load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
            imageStore(IrradianceOutput, ivec2(thread), load);
        }
        else if (MODE == DISTANCE_MODE)
        {
            load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
            imageStore(DistanceOutput, ivec2(thread), load);
        }
        
        thread.x += probeSideLengthMinusOne;
        copyCoordinates.x = thread.x - NUM_TEXELS_PER_PROBE;
        
        if (MODE == RADIANCE_MODE)
        {
            load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
            imageStore(IrradianceOutput, ivec2(thread), load);
        }
        else if (MODE == DISTANCE_MODE)
        {
            load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
            imageStore(DistanceOutput, ivec2(thread), load);
        }
        return;
    }
        
    uint probeStart = uint(thread.y / probeSideLength) * probeSideLength;
    uint offset = probeSideLengthMinusOne - (thread.y % probeSideLength);
    
    copyCoordinates = uvec2(thread.x + 1, probeStart + offset);
     
    vec4 load;
    if (MODE == RADIANCE_MODE)
    {
        load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
        imageStore(IrradianceOutput, ivec2(thread), load);
    }
    else if (MODE == DISTANCE_MODE)
    {
        load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
        imageStore(DistanceOutput, ivec2(thread), load);
    }
    
    thread.x += probeSideLengthMinusOne;
    copyCoordinates = uvec2(thread.x - 1, probeStart + offset);
    
    if (MODE == RADIANCE_MODE)
    {
        load = imageLoad(IrradianceOutput, ivec2(copyCoordinates));
        imageStore(IrradianceOutput, ivec2(thread), load);
    }
    else if (MODE == DISTANCE_MODE)
    {
        load = imageLoad(DistanceOutput, ivec2(copyCoordinates));
        imageStore(DistanceOutput, ivec2(thread), load);
    }
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
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader void
ProbeFinalizeBorderRowsRadiance()
{
    BorderRows(RADIANCE_MODE);
}


//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader void
ProbeFinalizeBorderColumnsRadiance()
{
    BorderColumns(RADIANCE_MODE);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader void
ProbeFinalizeBorderRowsDistance()
{
    BorderRows(DISTANCE_MODE);
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 8
[local_size_y] = 8
[local_size_z] = 1
shader void
ProbeFinalizeBorderColumnsDistance()
{
    BorderColumns(DISTANCE_MODE);
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

//------------------------------------------------------------------------------
/**
*/
program RadianceBorderRowsFixup[ string Mask = "ProbeFinalizeBorderRowsRadiance"; ]
{
    ComputeShader = ProbeFinalizeBorderRowsRadiance();
};

//------------------------------------------------------------------------------
/**
*/
program RadianceBorderColumnsFixup[ string Mask = "ProbeFinalizeBorderColumnsRadiance"; ]
{
    ComputeShader = ProbeFinalizeBorderColumnsRadiance();
};

//------------------------------------------------------------------------------
/**
*/
program DistanceBorderRowsFixup[ string Mask = "ProbeFinalizeBorderRowsDistance"; ]
{
    ComputeShader = ProbeFinalizeBorderRowsDistance();
};

//------------------------------------------------------------------------------
/**
*/
program DistanceBorderColumnsFixup[ string Mask = "ProbeFinalizeBorderColumnsDistance"; ]
{
    ComputeShader = ProbeFinalizeBorderColumnsDistance();
};
