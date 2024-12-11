//------------------------------------------------------------------------------
//  @file probe_shared.fxh
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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
