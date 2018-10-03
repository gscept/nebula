//------------------------------------------------------------------------------
//  hbaoblur_cs.fx
//  (C) 2014 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

varblock HBAOBlur
{
	float PowerExponent = 1.0f;
	float BlurFalloff;
	float BlurDepthThreshold;
};

samplerstate LinearState
{
	//Samplers = {HBAOReadLinear};
	Filter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

samplerstate PointState
{
	//Samplers = {HBAOReadPoint};
	Filter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

texture2D HBAOX;
texture2D HBAOY;
write rg16f image2D HBAORG;
write r16f image2D HBAOR;

#define KERNEL_RADIUS 16
#define KERNEL_RADIUS_FLOAT 16.0f
#define HALF_KERNEL_RADIUS (KERNEL_RADIUS/2.0f)

#define HBAO_TILE_WIDTH 320
#define SHARED_MEM_SIZE (KERNEL_RADIUS + HBAO_TILE_WIDTH + KERNEL_RADIUS)
groupshared vec2 SharedMemory[SHARED_MEM_SIZE];

//------------------------------------------------------------------------------
/**
*/
float
CrossBilateralWeight(float r, float d, float d0)
{
    return exp2(-r*r*BlurFalloff) * float(abs(d - d0) < BlurDepthThreshold);
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = SHARED_MEM_SIZE
shader
void
csMainX() 
{
	// get full resolution and inverse full resolution
	ivec2 size = textureSize(sampler2D(HBAOX, LinearState), 0);
	
	// calculate offsets
	const uint         tileStart = int(gl_WorkGroupID.x) * HBAO_TILE_WIDTH;
	const uint           tileEnd = tileStart + HBAO_TILE_WIDTH;
	const uint        apronStart = tileStart - KERNEL_RADIUS;
	const uint          apronEnd = tileEnd   + KERNEL_RADIUS;
	
	const uint x = apronStart + gl_LocalInvocationID.x;
	const uint y = gl_WorkGroupID.y;
	SharedMemory[gl_LocalInvocationID.x] = texelFetch(sampler2D(HBAOX, LinearState), ivec2(x, y), 0).xy;
	groupMemoryBarrier();
	
	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.x));
	
	if (writePos < tileEndClamped)
	{
		// Fetch (ao,z) at the kernel center
        vec2 AoDepth = texelFetch(sampler2D(HBAOX, PointState), ivec2(writePos, y), 0).xy;
		float ao_total = AoDepth.x;
		float center_d = AoDepth.y;
		float w_total = 1;
		float i;

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + (-KERNEL_RADIUS_FLOAT + 0.5f);
		    uint j = 2*uint(i) + gl_LocalInvocationID.x;
		    vec2 samp = SharedMemory[j];
		    float w = CrossBilateralWeight(r, samp.y, center_d);
		    ao_total += w * samp.x;
		    w_total += w;
		}

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + 1.5f;
		    uint j = 2*uint(i) + gl_LocalInvocationID.x + KERNEL_RADIUS + 1;
		    vec2 samp = SharedMemory[j];
		    float w = CrossBilateralWeight(r, samp.y, center_d);
		    ao_total += w * samp.x;
		    w_total += w;
		}
		
		float ao = ao_total / w_total;
		imageStore(HBAORG, ivec2(writePos, y), vec4(ao, center_d, 0, 0));
	}
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = SHARED_MEM_SIZE
shader
void
csMainY() 
{
	// get full resolution and inverse full resolution
	ivec2 size = textureSize(sampler2D(HBAOY, LinearState), 0);
	vec2 inverseSize = 1 / vec2(size);
	
	// calculate offsets
	const uint         tileStart = int(gl_WorkGroupID.x) * HBAO_TILE_WIDTH;
	const uint           tileEnd = tileStart + HBAO_TILE_WIDTH;
	const uint        apronStart = tileStart - KERNEL_RADIUS;
	const uint          apronEnd = tileEnd   + KERNEL_RADIUS;
	
	const uint x = gl_WorkGroupID.y;
	const uint y = apronStart + gl_LocalInvocationID.x;
	SharedMemory[gl_LocalInvocationID.x] = texelFetch(sampler2D(HBAOY, LinearState), ivec2(x, y), 0).xy;
	groupMemoryBarrier();
	
	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.y));
	
	if (writePos < tileEndClamped)
	{
		// Fetch (ao,z) at the kernel center
        vec2 AoDepth = texelFetch(sampler2D(HBAOY, PointState), ivec2(x, writePos), 0).xy;
		float ao_total = AoDepth.x;
		float center_d = AoDepth.y;
		float w_total = 1;
		float i;

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + (-KERNEL_RADIUS_FLOAT + 0.5f);
		    uint j = 2*uint(i) + gl_LocalInvocationID.x;
		    vec2 samp = SharedMemory[j];
		    float w = CrossBilateralWeight(r, samp.y, center_d);
		    ao_total += w * samp.x;
		    w_total += w;
		}

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + 1.5f;
		    uint j = 2*uint(i) + gl_LocalInvocationID.x + KERNEL_RADIUS + 1;
		    vec2 samp = SharedMemory[j];
		    float w = CrossBilateralWeight(r, samp.y, center_d);
		    ao_total += w * samp.x;
		    w_total += w;
		}
		
		float ao = ao_total / w_total;
		imageStore(HBAOR, ivec2(x, writePos), vec4(ao, 0, 0, 0));
	}
}

//------------------------------------------------------------------------------
/**
*/
program BlurX [ string Mask = "Alt0"; ]
{
	ComputeShader = csMainX();
};

program BlurY [ string Mask = "Alt1"; ]
{
	ComputeShader = csMainY();
};