//------------------------------------------------------------------------------
//  @file ddgi.fxh
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#ifndef DDGI_FXH
#define DDGI_FXH
#include <lib/shared.fxh>

const int PROBE_STATE_INACTIVE = 0;
const int PROBE_STATE_ACTIVE = 1;
const int NUM_FIXED_RAYS = 32;

const int NUM_IRRADIANCE_TEXELS_PER_PROBE = 6;
const int NUM_DISTANCE_TEXELS_PER_PROBE = 14;

const uint RELOCATION_OPTION = 0x1;
const uint SCROLL_OPTION = 0x2;
const uint CLASSIFICATION_OPTION = 0x4;
const uint LOW_PRECISION_IRRADIANCE_OPTION = 0x8;
const uint PARTIAL_UPDATE_OPTION = 0x10;

//------------------------------------------------------------------------------
/**
*/
int 
DDGIProbesPerPlane(ivec3 probeGridCounts)
{
    return probeGridCounts.x * probeGridCounts.z;
}

//------------------------------------------------------------------------------
/**
*/
int
DDGIProbeIndex(ivec2 texCoord, ivec3 probeGridCounts)
{
    return texCoord.x + (texCoord.y * (probeGridCounts.x * probeGridCounts.y));
}

//------------------------------------------------------------------------------
/**
*/
int 
DDGIProbeIndex(ivec3 probeCoords, ivec3 probeGridCounts)
{
    return probeCoords.x + (probeGridCounts.x * probeGridCounts.z) + (probeGridCounts.x * probeGridCounts.z) * probeCoords.y;
}

//------------------------------------------------------------------------------
/**
*/
int 
DDGIProbeIndex(uvec2 threadCoords, ivec3 probeGridCounts, int numTexels)
{
    int probesPerPlane = DDGIProbesPerPlane(probeGridCounts);
    int planeIndex = int(threadCoords.x / (probeGridCounts.x * numTexels));
    int probeIndexInPlane = int(threadCoords.x / numTexels) - (planeIndex * probeGridCounts.x) + (probeGridCounts.x * int(threadCoords.y / numTexels));
    return planeIndex * probesPerPlane + probeIndexInPlane;
}

//------------------------------------------------------------------------------
/**
*/
ivec2 
DDGIProbeTexelPosition(int probeIndex, ivec3 probeGridCounts)
{
    return ivec2(probeIndex % (probeGridCounts.x * probeGridCounts.y), probeIndex / (probeGridCounts.x * probeGridCounts.y));
}


//------------------------------------------------------------------------------
/**
*/
ivec3
DDGIProbeCoords(int probeIndex, ivec3 probeGridCounts)
{
    ivec3 ret;
    ret.x = probeIndex % probeGridCounts.x;
    ret.y = probeIndex / (probeGridCounts.x * probeGridCounts.z);
    ret.z = (probeIndex / probeGridCounts.x) % probeGridCounts.z;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
vec2 
DDGIProbeUV(int probeIndex, vec2 octantCoords, ivec3 probeGridCounts, int numTexels, ivec3 probeScrollOffsets, uint options)
{
    int probesPerPlane = DDGIProbesPerPlane(probeGridCounts);
    int planeIndex = probeIndex / probesPerPlane;
    
    float probeInteriorTexels = float(numTexels);
    float probeTexels = probeInteriorTexels * 2;
    
    int gridSpaceX = probeIndex % probeGridCounts.x;
    int gridSpaceY = probeIndex / probeGridCounts.x;
    
    if ((options & SCROLL_OPTION) != 0)
    {
        gridSpaceX = (gridSpaceX + probeScrollOffsets.x) % probeGridCounts.x;
        gridSpaceY = (gridSpaceY + probeScrollOffsets.z) % probeGridCounts.z;
        planeIndex = (planeIndex + probeScrollOffsets.y) % probeGridCounts.y;
    }
    
    int x = gridSpaceX + (planeIndex * probeGridCounts.x);
    int y = gridSpaceY % probeGridCounts.z;
    
    float textureWidth = probeTexels * (probeGridCounts.x * probeGridCounts.y);
    float textureHeight = probeTexels * probeGridCounts.z;
    
    vec2 uv = vec2(x * probeTexels, y * probeTexels) + (probeTexels * 0.5f);
    uv += octantCoords.xy * (probeInteriorTexels * 0.5f);
    uv /= vec2(textureWidth, textureHeight);
    return uv;
}

//------------------------------------------------------------------------------
/**
*/
ivec3 
DDGIBaseProbeGridCoordinates(vec3 worldPosition, vec3 volumeOrigin, vec4 rotation, ivec3 probeGridCounts, vec3 probeGridSpacing)
{
    vec3 position = worldPosition - volumeOrigin;
    // TODO: handle rotations
    
    position += (probeGridSpacing * (probeGridCounts - 1)) * 0.5f;
    ivec3 probeCoords = ivec3(position / probeGridSpacing);
    
    probeCoords = clamp(probeCoords, ivec3(0), (probeGridCounts - ivec3(1)));
    
    return probeCoords; 
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIProbeWorldPosition(int3 probeCoords, vec3 origin, vec4 rotation, ivec3 probeGridCounts, vec3 probeGridSpacing)
{
    vec3 probeGridWorldPosition = probeCoords * probeGridSpacing;
    vec3 probeGridShift = (probeGridSpacing * (probeGridCounts - 1)) * 0.5f;
    
    vec3 probeWorldPosition = probeGridWorldPosition - probeGridShift;
    // TODO: ROTATE
    probeWorldPosition += origin;
    return probeWorldPosition;
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIProbeWorldPosition(int probeIndex, vec3 origin, vec4 rotation, ivec3 probeGridCounts, vec3 probeGridSpacing)
{
    ivec3 probeCoords = DDGIProbeCoords(probeIndex, probeGridCounts);
    return DDGIProbeWorldPosition(probeCoords, origin, rotation, probeGridCounts, probeGridSpacing);
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIDecodeProbeOffsets(ivec2 probeOffsetTexcoord, vec3 probeGridSpacing, uint probeOffsets)
{
    return fetch2D(probeOffsets, Basic2DSampler, probeOffsetTexcoord, 0).xyz * probeGridSpacing; 
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIProbeWorldPositionWithOffset(int probeIndex, vec3 origin, vec4 rotation, ivec3 probeGridCounts, vec3 probeGridSpacing, uint probeOffsets)
{
    int textureWidth = probeGridCounts.x * probeGridCounts.y;
    ivec2 offsetTexCoords = ivec2(probeIndex % textureWidth, probeIndex / textureWidth);
    return DDGIDecodeProbeOffsets(offsetTexCoords, probeGridSpacing, probeOffsets) + DDGIProbeWorldPosition(probeIndex, origin, rotation, probeGridCounts, probeGridSpacing);
}

//------------------------------------------------------------------------------
/**
*/
int
DDGIProbeIndexOffset(int baseProbeIndex, ivec3 probeGridCounts, ivec3 probeScrollOffsets)
{
    ivec3 probeGridCoord = DDGIProbeCoords(baseProbeIndex, probeGridCounts);
    ivec3 offsetProbeGridCoord = (probeGridCoord + probeScrollOffsets) % probeGridCounts;
    int offsetProbeIndex = DDGIProbeIndex(offsetProbeGridCoord, probeGridCounts);
    return offsetProbeIndex;
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIProbeWorldPositionWithScrollAndOffset(int probeIndex, vec3 origin, vec4 rotation, ivec3 probeGridCounts, vec3 probeGridSpacing, ivec3 probeScrollOffsets, uint probeOffsets)
{
    int textureWidth = probeGridCounts.x * probeGridCounts.y;
    int storageProbeIndex = DDGIProbeIndexOffset(probeIndex, probeGridCounts, probeScrollOffsets);
    ivec2 offsetTexCoords = ivec2(storageProbeIndex % textureWidth, storageProbeIndex / textureWidth);
    return DDGIDecodeProbeOffsets(offsetTexCoords, probeGridSpacing, probeOffsets) + DDGIProbeWorldPosition(probeIndex, origin, rotation, probeGridCounts, probeGridSpacing);
}

//------------------------------------------------------------------------------
/**
*/
float
DDGISignNotZero(float v)
{
    return v >= 0.0f ? 1.0f : -1.0f;
}

//------------------------------------------------------------------------------
/**
*/
vec2
DDGISignNotZero(vec2 v)
{
    return vec2(DDGISignNotZero(v.x), DDGISignNotZero(v.y));
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGISurfaceBias(vec3 normal, vec3 cameraDirection, float normalBias, float viewBias)
{
    return (normal * normalBias) + (-cameraDirection * viewBias);
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

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIGetProbeDirection(int rayIndex, mat4x4 rotation, uint options)
{
    vec3 direction;
    if ((options & (RELOCATION_OPTION | CLASSIFICATION_OPTION)) != 0)
    {
        bool useFixedRays = rayIndex < NUM_FIXED_RAYS;
        int adjustedRayIndex = useFixedRays ? rayIndex : rayIndex - NUM_FIXED_RAYS;
        direction = useFixedRays ? MinimalDirections[adjustedRayIndex].xyz : Directions[adjustedRayIndex].xyz;
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
vec2 
DDGINormalizedOctahedralCoordinates(ivec2 threadCoords, int numTexels)
{
    vec2 octahedralTexelCoord = vec2(threadCoords.x % numTexels, threadCoords.y % numTexels);
    octahedralTexelCoord.xy += 0.5f;
    octahedralTexelCoord.xy /= float(numTexels);
    octahedralTexelCoord *= 2.0f;
    octahedralTexelCoord -= vec2(1.0f);
    return octahedralTexelCoord;
}

//------------------------------------------------------------------------------
/**
*/
vec3
DDGIOctahedralDirection(vec2 coords)
{
    vec3 direction = vec3(coords.x, coords.y, 1.0f - abs(coords.x) - abs(coords.y));
    if (direction.z < 0.0f)
    {
        direction.xy = (1.0f - abs(direction.yx)) * DDGISignNotZero(direction.xy);
    }
    return normalize(direction);
}

//------------------------------------------------------------------------------
/**
*/
vec2 
DDGIOctahedralCoordinates(vec3 direction)
{
    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
    vec2 uv = direction.xy * (1.0f / l1norm);
    if (direction.z < 0.0f)
    {
        uv = (1.0f - abs(uv.yx)) * DDGISignNotZero(direction.xy);
    }
    return uv;
}

struct DDGIVolume
{
    vec3 origin;
    vec4 rotationQuat;
    ivec3 probeGridCounts;
    vec3 probeGridSpacing;
    ivec3 probeScrollOffsets;
    int numIrradianceTexels;
    int numDistanceTexels;
    float probeIrradianceGamma;
    uint irradianceHandle;
    uint distanceHandle;
    uint offsetsHandle;
    uint statesHandle;
};

//------------------------------------------------------------------------------
/**
*/
vec3 
EvaluateDDGIIrradiance(
    vec3 worldPosition,
    vec3 surfaceBias,
    vec3 direction,
    GIVolume volume,
    uint options
)
{
    vec3 irradiance = vec3(0, 0, 0);
    float accumulatedWeights = 0.0f;
    
    vec3 biasedWorldPosition = worldPosition + surfaceBias;
    ivec3 baseProbeCoordinates = DDGIBaseProbeGridCoordinates(biasedWorldPosition, volume.Offset, volume.Rotation, volume.GridCounts, volume.GridSpacing);
    vec3 baseProbeWorldPosition = DDGIProbeWorldPosition(baseProbeCoordinates, volume.Offset, volume.Rotation, volume.GridCounts, volume.GridSpacing);
    
    vec3 distanceVolumeSpace = biasedWorldPosition - baseProbeWorldPosition;
    // TODO: ROTATE
    
    vec3 alpha = clamp(distanceVolumeSpace / volume.GridSpacing, vec3(0), vec3(1));
    for (int probeIndex = 0; probeIndex < 8; probeIndex++)
    {
        ivec3 adjacentProbeOffset = ivec3(probeIndex, probeIndex >> 1, probeIndex >> 2) & ivec3(1);
        ivec3 adjacentProbeCoords = clamp(baseProbeCoordinates + adjacentProbeOffset, ivec3(0), volume.GridCounts - ivec3(1));
        float3 adjacentProbeWorldPosition; 
        
        if ((options & RELOCATION_OPTION) != 0)
        {
            if ((options & SCROLL_OPTION) != 0)
            {
                adjacentProbeWorldPosition = DDGIProbeWorldPositionWithScrollAndOffset(probeIndex, volume.Offset, volume.Rotation, volume.GridCounts, volume.GridSpacing, volume.ScrollOffsets, volume.Offsets);
            }
            else
            {
                adjacentProbeWorldPosition = DDGIProbeWorldPositionWithOffset(probeIndex, volume.Offset, volume.Rotation, volume.GridCounts, volume.GridSpacing, volume.Offsets);
            }
        }
        else
        {
            adjacentProbeWorldPosition = DDGIProbeWorldPosition(adjacentProbeCoords, volume.Offset, volume.Rotation, volume.GridCounts, volume.GridSpacing);
        }
        
        int adjacentProbeIndex = DDGIProbeIndex(adjacentProbeCoords, volume.GridCounts);
        
        if ((options & CLASSIFICATION_OPTION) != 0)
        {
            int probeIndex;
            if ((options & SCROLL_OPTION) != 0)
            {
                probeIndex = DDGIProbeIndexOffset(adjacentProbeIndex, volume.GridCounts, volume.ScrollOffsets);
            }
            else
            {
                probeIndex = adjacentProbeIndex;
            }
            ivec2 texelPosition = DDGIProbeTexelPosition(probeIndex, volume.GridCounts);
            int state = floatBitsToInt(fetch2D(volume.States, Basic2DSampler, texelPosition, 0).x);
            if (state == PROBE_STATE_INACTIVE)
                continue;
        }
        
        vec3 worldPosToAdjacentProbe = normalize(adjacentProbeWorldPosition - worldPosition);
        vec3 biasedPosToAdjacentProbe = normalize(adjacentProbeWorldPosition - biasedWorldPosition);
        float biasedPosToAdjustedProbeDistance = length(adjacentProbeWorldPosition - biasedWorldPosition);
        
        vec3 trilinear = max(vec3(0.001f), lerp(1.0f - alpha, alpha, vec3(adjacentProbeOffset)));
        float trilinearWeight = (trilinear.x * trilinear.y * trilinear.z);
        float weight = 1.0f;
        
        float wrapShading = (dot(worldPosToAdjacentProbe, direction) + 1.0f) * 0.5f;
        weight *= (wrapShading * wrapShading) + 0.2f;
        
        vec2 octantCoords = DDGIOctahedralCoordinates(-biasedPosToAdjacentProbe);
        vec2 probeTextureCoords;
        if ((options & SCROLL_OPTION) != 0)
        {
            probeTextureCoords = DDGIProbeUV(adjacentProbeIndex, octantCoords, volume.GridCounts, volume.NumIrradianceTexels, volume.ScrollOffsets, options);
        }
        else
        {
            probeTextureCoords = DDGIProbeUV(adjacentProbeIndex, octantCoords, volume.GridCounts, volume.NumIrradianceTexels, ivec3(0), options);
        }
        vec2 filteredDistance = sample2DLod(volume.Distances, Basic2DSampler, probeTextureCoords, 0).xy;
        
        float meanDistanceToSurface = filteredDistance.x;
        float variance = abs((filteredDistance.x * filteredDistance.x) - filteredDistance.y);
        
        float chebyshevWeight = 1.0f;
        if (biasedPosToAdjustedProbeDistance > meanDistanceToSurface)
        {
            float v = biasedPosToAdjustedProbeDistance - meanDistanceToSurface;
            chebyshevWeight = variance / (variance + (v*v));
            
            chebyshevWeight = max((chebyshevWeight * chebyshevWeight * chebyshevWeight), 0.0f);
        }        
        
        weight *= max(0.05f, chebyshevWeight);
        weight = max(0.000001f, weight);
        
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
        {
            weight *= (weight * weight) * (1.0f / (crushThreshold * crushThreshold));
        }
        
        weight *= trilinearWeight;
        
        octantCoords = DDGIOctahedralCoordinates(direction);
        if ((options & SCROLL_OPTION) != 0)
        {
            probeTextureCoords = DDGIProbeUV(adjacentProbeIndex, octantCoords, volume.GridCounts, volume.NumIrradianceTexels, volume.ScrollOffsets, options);
        }
        else
        {
            probeTextureCoords = DDGIProbeUV(adjacentProbeIndex, octantCoords, volume.GridCounts, volume.NumIrradianceTexels, ivec3(0), options);
        }
        vec3 probeIrradiance = sample2DLod(volume.Irradiance, Basic2DSampler, probeTextureCoords, 0).xyz;
        
        vec3 exponent = vec3(volume.EncodingGamma * 0.5f);
        probeIrradiance = pow(probeIrradiance, exponent);
        
        irradiance += (weight * probeIrradiance);
        accumulatedWeights += weight;
    }
    
    if (accumulatedWeights == 0.0f)
        return vec3(0);
        
    irradiance *= (1.0f / accumulatedWeights);
    irradiance *= irradiance;
    irradiance *= PI * 2.0f;
    
    if ((options & LOW_PRECISION_IRRADIANCE_OPTION) != 0)
        irradiance *= 1.0989f; 
    
    return irradiance;
}

#endif // DDGI_FXH
