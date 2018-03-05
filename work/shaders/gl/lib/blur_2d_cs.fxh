//------------------------------------------------------------------------------
//  blur_2d_cs.fxh
//
//	Blurring kernel used for 2D textures. Implements a double pass X-Y blur with a defined kernel size.
//
//	Include this header and then define if you want an RGBA16F, RG16F or RG32F image as input.
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/compute.fxh"

#if IMAGE_IS_RGBA16F
#define IMAGE_FORMAT_TYPE rgba16f
#define IMAGE_LOAD_VEC vec4
#define IMAGE_LOAD_SWIZZLE(vec) vec.xyzw
#define RESULT_TO_VEC4(vec) vec
#elif IMAGE_IS_RGBA16
#define IMAGE_FORMAT_TYPE rgba16
#define IMAGE_LOAD_VEC vec4
#define IMAGE_LOAD_SWIZZLE(vec) vec4(vec.xyzw)
#define RESULT_TO_VEC4(vec) vec
#elif IMAGE_IS_BGRA16F
#define IMAGE_FORMAT_TYPE rgba16f
#define IMAGE_LOAD_VEC vec4
#define IMAGE_LOAD_SWIZZLE(vec) vec.zyxw
#define RESULT_TO_VEC4(vec) vec
#elif IMAGE_IS_RG16F
#define IMAGE_FORMAT_TYPE rg16f
#define IMAGE_LOAD_VEC vec2
#define IMAGE_LOAD_SWIZZLE(vec) vec.xy
#define RESULT_TO_VEC4(vec) vec4(vec.xy, 0, 0)
#elif IMAGE_IS_RG32F
#define IMAGE_FORMAT_TYPE rg32f
#define IMAGE_LOAD_VEC vec2
#define IMAGE_LOAD_SWIZZLE(vec) vec.xy
#define RESULT_TO_VEC4(vec) vec4(vec.xy, 0, 0)
#endif

readwrite IMAGE_FORMAT_TYPE image2D WriteImage;
#define INV_LN2 1.44269504f
#define SQRT_LN2 0.832554611f

#ifndef BLUR_SHARPNESS
	#define BLUR_SHARPNESS 8.0f
#endif

#ifndef KERNEL_RADIUS
	#define KERNEL_RADIUS 9
#endif
#define HALF_KERNEL_RADIUS ((KERNEL_RADIUS - 1)/2)

#define BLUR_TILE_WIDTH 320
#define SHARED_MEM_SIZE (KERNEL_RADIUS + BLUR_TILE_WIDTH + KERNEL_RADIUS)
groupshared IMAGE_LOAD_VEC SharedMemory[SHARED_MEM_SIZE];

#if KERNEL_RADIUS == 15
const float GaussianWeights[] = { 
0.023089, 0.034587, 0.048689, 0.064408, 0.080066, 0.093531, 0.102673, 0.105915, 0.102673, 0.093531, 0.080066, 0.064408, 0.048689, 0.034587, 0.023089};
#elif KERNEL_RADIUS == 9
const float GaussianWeights[] = { 
	0.000229,
	0.005977,
	0.060598,
	0.241732,
	0.382928,
	0.241732,
	0.060598,
	0.005977,
	0.000229
};
#elif KERNEL_RADIUS == 5
const float GaussianWeights[] = { 0.166852f, 0.215677f, 0.234942f, 0.215677f, 0.166852f };
#endif

//------------------------------------------------------------------------------
/**
*/
IMAGE_LOAD_VEC
GaussianBlur(uint index, IMAGE_LOAD_VEC samp)
{
	return samp * GaussianWeights[index];
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
	ivec2 size = imageSize(WriteImage);
	
	// calculate offsets
	uint x, y, start, end;
	ComputePixelX(BLUR_TILE_WIDTH, HALF_KERNEL_RADIUS, gl_WorkGroupID.xy, gl_LocalInvocationID.xy, x, y, start, end);
	
	// load into workgroup saved memory, this allows us to use the original pixel even though 
	// we might have replaced it with the result from this thread!
	SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageLoad(WriteImage, ivec2(x, y)));
	barrier();
	
	const uint writePos = start + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(end, uint(size.x));
	
	if (writePos < tileEndClamped)
	{
		IMAGE_LOAD_VEC blurTotal = IMAGE_LOAD_VEC(0);
		uint i;

		#pragma unroll
		for (i = 0; i < KERNEL_RADIUS; ++i)
		{
			uint j = uint(i) + gl_LocalInvocationID.x;
			IMAGE_LOAD_VEC samp = SharedMemory[j];
			blurTotal += GaussianBlur(i, samp);
		}
		
		IMAGE_LOAD_VEC blur = blurTotal;
		imageStore(WriteImage, ivec2(writePos, y), RESULT_TO_VEC4(blur));
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
	ivec2 size = imageSize(WriteImage);
	
	// calculate offsets
	uint x, y, start, end;
	ComputePixelY(BLUR_TILE_WIDTH, HALF_KERNEL_RADIUS, gl_WorkGroupID.xy, gl_LocalInvocationID.xy, x, y, start, end);
	
	// load into workgroup saved memory, this allows us to use the original pixel even though 
	// we might have replaced it with the result from this thread!
	SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageLoad(WriteImage, ivec2(x, y)));
	barrier();
	
	const uint writePos = start + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(end, uint(size.y));
	
	if (writePos < tileEndClamped)
	{
		IMAGE_LOAD_VEC blurTotal = IMAGE_LOAD_VEC(0);
		uint i;

		#pragma unroll
		for (i = 0; i < KERNEL_RADIUS; ++i)
		{
			uint j = uint(i) + gl_LocalInvocationID.x;
			IMAGE_LOAD_VEC samp = SharedMemory[j];
			blurTotal += GaussianBlur(i, samp);
		}
		
		IMAGE_LOAD_VEC blur = blurTotal;
		imageStore(WriteImage, ivec2(x, writePos), RESULT_TO_VEC4(blur));
	}
}