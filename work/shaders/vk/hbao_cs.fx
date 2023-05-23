//------------------------------------------------------------------------------
//  hbao_cs.fx
//  (C) 2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"

constant HBAOBlock
{
    vec2 UVToViewA = vec2(0.0f, 0.0f);
    vec2 UVToViewB = vec2(0.0f, 0.0f);
    vec2 AOResolution = vec2(0.0f, 0.0f);
    vec2 InvAOResolution = vec2(0.0f, 0.0f);
    float TanAngleBias = 0.0f;
    float Strength = 0.0f;
    float R2 = 0.0f;
};

// Step size in number of pixels
#ifndef STEP_SIZE
#define STEP_SIZE 8
#endif

// Number of shared-memory samples per direction
#ifndef NUM_STEPS
#define NUM_STEPS 8
#endif

//sampler2D RandomMap;
readwrite rg16f image2D HBAO0;
write rg16f image2D HBAO1;

sampler_state ClampSampler
{
    //Samplers = { DepthBuffer };
    Filter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Maximum kernel radius in number of pixels
#define KERNEL_RADIUS (NUM_STEPS*STEP_SIZE)

// The last sample has weight = exp(-KERNEL_FALLOFF)
#define KERNEL_FALLOFF 3.0f

// Must match the HBAO_TILE_WIDTH value from AOAlgorithm
#define HBAO_TILE_WIDTH 256
const uint HBAOTileWidth = HBAO_TILE_WIDTH;

#define SHARED_MEM_SIZE (KERNEL_RADIUS + HBAO_TILE_WIDTH + KERNEL_RADIUS)
groupshared vec2 SharedMemory[SHARED_MEM_SIZE]; 

//----------------------------------------------------------------------------------
float Tangent(vec2 V)
{
    // Add an epsilon to avoid any division by zero.
    return V.y / (abs(V.x) + 1.e-6f);
}

//----------------------------------------------------------------------------------
float TanToSin(float x)
{
    return x * rsqrt(x*x + 1.0f);
}

//----------------------------------------------------------------------------------
float Falloff(float sampleId)
{
    // Pre-computed by fxc.
    float r = sampleId / (NUM_STEPS-1);
    return exp(-KERNEL_FALLOFF*r*r);
}

//----------------------------------------------------------------------------------
vec2 MinDiff(vec2 P, vec2 Pr, vec2 Pl)
{
    vec2 V1 = Pr - P;
    vec2 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

//----------------------------------------------------------------------------------
// Load (X,Z) view-space coordinates from shared memory.
// On Fermi, such strided 64-bit accesses should not have any bank conflicts.
//----------------------------------------------------------------------------------
vec2 SharedMemoryLoad(int centerId, int x)
{
    return SharedMemory[centerId + x];
}

//----------------------------------------------------------------------------------
// Compute (X,Z) view-space coordinates from the depth texture.
//----------------------------------------------------------------------------------
vec2 LoadXZFromTexture(uint x, uint y)
{ 
    vec2 uv = (vec2(x, y) + 0.5f) * InvAOResolution;
    float z_eye = fetch2D(DepthBuffer, ClampSampler, ivec2(x, y), 0).r;
    vec4 viewSpace = PixelToView(uv, z_eye, InvProjection);
    return vec2(viewSpace.x, viewSpace.z);
}

//----------------------------------------------------------------------------------
// Compute (Y,Z) view-space coordinates from the depth texture.
//----------------------------------------------------------------------------------
vec2 LoadYZFromTexture(uint x, uint y)
{
    vec2 uv = (vec2(x, y) + 0.5f) * InvAOResolution;
    float z_eye = fetch2D(DepthBuffer, ClampSampler, ivec2(x, y), 0).r;
    vec4 viewSpace = PixelToView(uv, z_eye, InvProjection);
    return vec2(viewSpace.y, viewSpace.z);
}

//----------------------------------------------------------------------------------
// Compute the HBAO contribution in a given direction on screen by fetching 2D 
// view-space coordinates available in shared memory:
// - (X,Z) for the horizontal directions (approximating Y by a constant).
// - (Y,Z) for the vertical directions (approximating X by a constant).
//----------------------------------------------------------------------------------
float IntegrateDirection(float iAO,
                        vec2 P,
                        float tanT,
                        int threadId,
                        int X0,
                        int deltaX)
{
    float tanH = tanT;
    float sinH = TanToSin(tanH);
    float sinT = TanToSin(tanT);
    float ao = iAO;

    #pragma unroll
    for (int sampleId = 0; sampleId < NUM_STEPS; ++sampleId)
    {
        vec2 S = SharedMemoryLoad(threadId, sampleId * deltaX + X0);
        vec2 V = S - P;
        float tanS = Tangent(V);
        float d2 = dot(V, V);
        
        if ((d2 < R2) && (tanS > tanH))
        {
            // Accumulate AO between the horizon and the sample
            float sinS = TanToSin(tanS);
            ao += Falloff(sampleId) * (sinS - sinH);
            
            // Update the current horizon angle
            tanH = tanS;
            sinH = sinS;
        }
    }
    return ao;
}

//----------------------------------------------------------------------------------
// Bias tangent angle and compute HBAO in the +X/-X or +Y/-Y directions.
//----------------------------------------------------------------------------------
float ComputeHBAO(vec2 P, vec2 T, int centerId)
{
    float ao = 0;
    float tanT = Tangent(T);
    ao = IntegrateDirection(ao, P,  tanT + TanAngleBias, centerId,  STEP_SIZE,  STEP_SIZE);
    ao = IntegrateDirection(ao, P, -tanT + TanAngleBias, centerId, -STEP_SIZE, -STEP_SIZE);
    return ao;
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = HBAO_TILE_WIDTH
shader
void
csMainX() 
{
    const uint         tileStart = uint(gl_WorkGroupID.x) * HBAO_TILE_WIDTH;
    const uint           tileEnd = tileStart + HBAO_TILE_WIDTH;
    const uint        apronStart = tileStart - KERNEL_RADIUS;
    const uint          apronEnd = tileEnd   + KERNEL_RADIUS;

    const uint x = apronStart + uint(gl_LocalInvocationID.x);
    const uint y = uint(gl_WorkGroupID.y);

    // Load vec2 samples into shared memory
    SharedMemory[gl_LocalInvocationID.x] = LoadXZFromTexture(x,y);
    SharedMemory[min(2 * KERNEL_RADIUS + int(gl_LocalInvocationID.x), SHARED_MEM_SIZE - 1)] = LoadXZFromTexture(2 * KERNEL_RADIUS + x, y);
    groupMemoryBarrier();
    barrier();

    const uint writePos = tileStart + uint(gl_LocalInvocationID.x);
    const uint tileEndClamped = min(tileEnd, uint(AOResolution.x));
    
    if (writePos < tileEndClamped)
    {
        int centerId = int(gl_LocalInvocationID.x) + KERNEL_RADIUS;
        uint ox = writePos; 
        uint oy = gl_WorkGroupID.y;

        // Fetch the 2D coordinates of the center point and its nearest neighbors
        vec2 P =  SharedMemoryLoad(centerId, 0);
        vec2 Pr = SharedMemoryLoad(centerId, 1);
        vec2 Pl = SharedMemoryLoad(centerId, -1);
        
        // Compute tangent vector using central differences
        vec2 T = MinDiff(P, Pr, Pl);

        float ao = ComputeHBAO(P, T, centerId);
        imageStore(HBAO0, int2(ox, oy), vec4(ao, 0, 0, 0));
    }
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = HBAO_TILE_WIDTH
shader
void
csMainY() 
{
    const uint         tileStart = uint(gl_WorkGroupID.x) * HBAO_TILE_WIDTH;
    const uint           tileEnd = tileStart + HBAO_TILE_WIDTH;
    const uint        apronStart = tileStart - KERNEL_RADIUS;
    const uint          apronEnd = tileEnd   + KERNEL_RADIUS;

    const uint x = uint(gl_WorkGroupID.y);
    const uint y = apronStart + uint(gl_LocalInvocationID.x);

    // Load vec2 samples into shared memory
    SharedMemory[gl_LocalInvocationID.x] = LoadYZFromTexture(x,y);
    SharedMemory[min(2 * KERNEL_RADIUS + int(gl_LocalInvocationID.x), SHARED_MEM_SIZE - 1)] = LoadYZFromTexture(x, 2 * KERNEL_RADIUS + y);
    groupMemoryBarrier();
    barrier();

    const uint writePos = tileStart + gl_LocalInvocationID.x;
    const uint tileEndClamped = min(tileEnd, uint(AOResolution.y));
    
    if (writePos < tileEndClamped)
    {
        int centerId = int(gl_LocalInvocationID.x) + KERNEL_RADIUS;
        uint ox = gl_WorkGroupID.y;
        uint oy = writePos;

        // Fetch the 2D coordinates of the center point and its nearest neighbors
        vec2 P =  SharedMemoryLoad(centerId, 0);
        vec2 Pt = SharedMemoryLoad(centerId, 1);
        vec2 Pb = SharedMemoryLoad(centerId, -1);

        // Compute tangent vector using central differences
        vec2 T = MinDiff(P, Pt, Pb);

        float aoy = ComputeHBAO(P, T, centerId);
        float aox = imageLoad(HBAO0, int2(gl_WorkGroupID.y, writePos)).x;
        float ao = (aox + aoy) * 0.5f;
        groupMemoryBarrier();
        ao = saturate(ao * Strength);
        imageStore(HBAO1, int2(ox, oy), vec4(ao, P.y, 0, 0));
    }
}

//------------------------------------------------------------------------------
/**
*/
program HBAOX [ string Mask = "Alt0"; ]
{
    ComputeShader = csMainX();
};

program HBAOY [ string Mask = "Alt1"; ]
{
    ComputeShader = csMainY();
};