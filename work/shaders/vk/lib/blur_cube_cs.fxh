//------------------------------------------------------------------------------
//  blur_cube_cs.fxh
//
//	Blurring kernel used for cube textures. Implements a double pass X-Y blur with a defined kernel size.
//
//	Include this header and then define if you want an RGBA16F, RG16F or RG32F image as input.
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

#if IMAGE_IS_RGBA16F
#define IMAGE_FORMAT_TYPE rgba16f
#define IMAGE_LOAD_VEC vec4
#define IMAGE_LOAD_SWIZZLE(vec) vec.xyzw
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

samplerstate InputSampler
{
	Filter = Point;
};

textureHandle InputImage;
write IMAGE_FORMAT_TYPE imageCube WriteImage;
#define INV_LN2 1.44269504f
#define SQRT_LN2 0.832554611f
#define BLUR_SHARPNESS 8.0f

#define KERNEL_RADIUS 16
#define KERNEL_RADIUS_FLOAT 16.0f
#define HALF_KERNEL_RADIUS (KERNEL_RADIUS/2.0f)

#define BLUR_TILE_WIDTH 320
#define SHARED_MEM_SIZE (KERNEL_RADIUS + BLUR_TILE_WIDTH + KERNEL_RADIUS)
groupshared IMAGE_LOAD_VEC SharedMemory[SHARED_MEM_SIZE];

//------------------------------------------------------------------------------
/**
	Calculate bilateral weight function, which is
	
	Fcolor(abs(I[xi] - I[x])) * Fcoord(abs(xi - x))
	The coords are 1D for our kernel since it operators on a single row/column at a time.
*/
IMAGE_LOAD_VEC
BilateralWeight(IMAGE_LOAD_VEC p, IMAGE_LOAD_VEC pi, float u, float ui)
{
	const float sigma = (KERNEL_RADIUS * 2 + 1) * 0.5f;
	const float falloff = INV_LN2 / (2.0f * sigma * sigma);
	IMAGE_LOAD_VEC pixelDiff = IMAGE_LOAD_VEC(lessThan(saturate(pi - p), IMAGE_LOAD_VEC(2.0f * SQRT_LN2 / BLUR_SHARPNESS)));
	//IMAGE_LOAD_VEC pixelDiff = pi;
	//IMAGE_LOAD_VEC pixelDiff = saturate(pi - p);
	//IMAGE_LOAD_VEC pixelDiff = pi;
    return abs(ui - u) * pixelDiff;
}

//------------------------------------------------------------------------------
/**
*/
vec3
GenerateCubemapCoord(in vec2 uv, in uint face)
{
	vec3 v;
	vec2 coord = uv * 2 - 1;
	switch(face)
	{
		case 0: v = vec3( 1.0, 		 coord.y, 	 coord.x); 	break; // +X
		case 1: v = vec3(-1.0,  	 coord.y, 	 coord.x); 	break; // -X
		case 2: v = vec3( coord.x,   1.0, 	 	 coord.y); 	break; // +Y
		case 3: v = vec3( coord.x, 	-1.0, 	 	 coord.y); 	break; // -Y
		case 4: v = vec3( coord.x,	 coord.y,    1.0); 		break; // +Z
		case 5: v = vec3( coord.x,   coord.y, 	-1.0); 		break; // -Z
	}
	return normalize(v);
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
	const uint         tileStart = int(gl_WorkGroupID.x) * BLUR_TILE_WIDTH;
	const uint           tileEnd = tileStart + BLUR_TILE_WIDTH;
	const uint        apronStart = tileStart - KERNEL_RADIUS;
	const uint          apronEnd = tileEnd   + KERNEL_RADIUS;
	
	const uint x = apronStart + gl_LocalInvocationID.x;
	const uint y = gl_WorkGroupID.y;
	
	// load into workgroup saved memory, this allows us to use the original pixel even though 
	// we might have replaced it with the result from this thread!
	SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(fetchCube(InputImage, InputSampler, ivec3(x, y, gl_WorkGroupID.z), 0));
	barrier();

	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.x));
	
	if (writePos < tileEndClamped)
	{
		// Fetch (ao,z) at the kernel center
		IMAGE_LOAD_VEC color = IMAGE_LOAD_SWIZZLE(fetchCube(InputImage, InputSampler, ivec3(writePos, y, gl_WorkGroupID.z), 0));
		//IMAGE_LOAD_VEC color = IMAGE_LOAD_SWIZZLE(textureLod(ReadImagePoint, ivec3(writePos, y, gl_WorkGroupID.z), 0));
		IMAGE_LOAD_VEC blurTotal = color;
		IMAGE_LOAD_VEC wTotal = IMAGE_LOAD_VEC(1);
		float i;

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + (-KERNEL_RADIUS_FLOAT + 0.5f);
		    uint j = 2*uint(i) + gl_LocalInvocationID.x;
			
		    IMAGE_LOAD_VEC samp = SharedMemory[j];
		    IMAGE_LOAD_VEC w = BilateralWeight(samp, color, r, writePos);
			blurTotal += color * w;
		    wTotal += w;
		}

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f*i + 1.5f;
		    uint j = 2*uint(i) + gl_LocalInvocationID.x + KERNEL_RADIUS + 1;
			
		    IMAGE_LOAD_VEC samp = SharedMemory[j];
			IMAGE_LOAD_VEC w = BilateralWeight(samp, color, r, writePos);
			blurTotal += color * w;
		    wTotal += w;
		}
		
		IMAGE_LOAD_VEC blur = blurTotal / wTotal;
		imageStore(WriteImage, ivec3(writePos, y, gl_WorkGroupID.z), RESULT_TO_VEC4(blur));
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
	const uint         tileStart = int(gl_WorkGroupID.x) * BLUR_TILE_WIDTH;
	const uint           tileEnd = tileStart + BLUR_TILE_WIDTH;
	const uint        apronStart = tileStart - KERNEL_RADIUS;
	const uint          apronEnd = tileEnd   + KERNEL_RADIUS;
	
	const uint x = gl_WorkGroupID.y;
	const uint y = apronStart + gl_LocalInvocationID.x;
	
	// load into workgroup saved memory, this allows us to use the original pixel even though 
	// we might have replaced it with the result from this thread!
	SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(fetchCube(InputImage, InputSampler, ivec3(x, y, gl_WorkGroupID.z), 0));
	barrier();
	
	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.y));
	
	if (writePos < tileEndClamped)
	{
		// Fetch (ao,z) at the kernel center
		IMAGE_LOAD_VEC color = IMAGE_LOAD_SWIZZLE(fetchCube(InputImage, InputSampler, ivec3(x, writePos, gl_WorkGroupID.z), 0));
		IMAGE_LOAD_VEC blurTotal = color;
		IMAGE_LOAD_VEC wTotal = IMAGE_LOAD_VEC(1);
		float i;

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f * i + (-KERNEL_RADIUS_FLOAT + 0.5f);
		    uint j = 2 * uint(i) + gl_LocalInvocationID.x;
		    IMAGE_LOAD_VEC samp = SharedMemory[j];
		    IMAGE_LOAD_VEC w = BilateralWeight(samp, color, r, writePos);
			blurTotal += color * w;
		    wTotal += w;
		}

		#pragma unroll
		for (i = 0; i < HALF_KERNEL_RADIUS; ++i)
		{
		    // Sample the pre-filtered data with step size = 2 pixels
		    float r = 2.0f * i + 1.5f;
		    uint j = 2 * uint(i) + gl_LocalInvocationID.x + KERNEL_RADIUS + 1;
			
		    IMAGE_LOAD_VEC samp = SharedMemory[j];
		    IMAGE_LOAD_VEC w = BilateralWeight(samp, color, r, writePos);
			blurTotal += color * w;
		    wTotal += w;
		}
		
		IMAGE_LOAD_VEC blur = blurTotal / wTotal;
		imageStore(WriteImage, ivec3(x, writePos, gl_WorkGroupID.z), RESULT_TO_VEC4(blur));
	}
}