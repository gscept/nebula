//------------------------------------------------------------------------------
/**
	blur_cs.fxh

	Blurring kernel used for 2D and array textures. Implements a double pass X-Y blur with a defined kernel size.
	First pass samples from a render-able texture (Alt0) and works in the X-axis.
	The second pass resamples from the same texture and blurs in the Y-axis.

	Include this header and then define if you want an RGBA16F, RG16F or RG32F image as input.

  (C) 2016 Individual contributors, see AUTHORS file
*/
#include <lib/shared.fxh>
#include <lib/techniques.fxh>

#if IMAGE_IS_RGBA16F
#define IMAGE_FORMAT_TYPE rgba16f
#define IMAGE_LOAD_VEC vec4
#define IMAGE_LOAD_SWIZZLE(vec) vec.xyzw
#define RESULT_TO_VEC4(vec) vec
#elif IMAGE_IS_RGB16F
#define IMAGE_FORMAT_TYPE rgba16f
#define IMAGE_LOAD_VEC vec3
#define IMAGE_LOAD_SWIZZLE(vec) vec.xyz
#define RESULT_TO_VEC4(vec) vec4(vec, 1)
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

sampler_state InputSampler
{
	Filter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

constant BlurUniforms
{
    ivec2 Viewport;
};

#if !(BLUR_KERNEL_8 || BLUR_KERNEL_16 || BLUR_KERNEL_32 || BLUR_KERNEL_64)
#define BLUR_KERNEL_16 1
#endif

#if BLUR_KERNEL_8
#define KERNEL_RADIUS 4
#define GAUSSIAN_KERNEL_SIZE_9 1
#elif BLUR_KERNEL_16
#define KERNEL_RADIUS 8
#define GAUSSIAN_KERNEL_SIZE_17 1
#elif BLUR_KERNEL_32
#define KERNEL_RADIUS 16
#define GAUSSIAN_KERNEL_SIZE_33 1
#elif BLUR_KERNEL_64
#define KERNEL_RADIUS 32
#define GAUSSIAN_KERNEL_SIZE_65 1
#endif

#define BLUR_TILE_WIDTH (64 - KERNEL_RADIUS * 2)
const uint BlurTileWidth = BLUR_TILE_WIDTH;
#define SHARED_MEM_SIZE (KERNEL_RADIUS + BLUR_TILE_WIDTH + KERNEL_RADIUS)

const float weights[] = {
#if GAUSSIAN_KERNEL_SIZE_65 // sigma 10
0.00024		,0.000328	,0.000445	,0.000598	,0.000795	,0.001046	,0.001363	,0.001759	,0.002246	,0.002841	,0.003557	,0.00441	,0.005412	,0.006576	,0.007912	,0.009423	,0.011112	,0.012973	,0.014996	,0.017162	,0.019445	,0.021812	,0.024225	,0.026637	,0.028998	,0.031255	,0.033352	,0.035236	,0.036857	,0.038168	,0.039134	,0.039725	,0.039924	,0.039725	,0.039134	,0.038168	,0.036857	,0.035236	,0.033352	,0.031255	,0.028998	,0.026637	,0.024225	,0.021812	,0.019445	,0.017162	,0.014996	,0.012973	,0.011112	,0.009423	,0.007912	,0.006576	,0.005412	,0.00441	,0.003557	,0.002841	,0.002246	,0.001759	,0.001363	,0.001046	,0.000795	,0.000598	,0.000445	,0.000328	,0.00024
#elif GAUSSIAN_KERNEL_SIZE_33 // sigma 5
0.000485	,0.000899	,0.001603	,0.002745	,0.004519	,0.007147	,0.010863	,0.015864	,0.022263	,0.030022	,0.038903	,0.048441	,0.05796	,0.066638	,0.073622	,0.078159	,0.079733	,0.078159	,0.073622	,0.066638	,0.05796	,0.048441	,0.038903	,0.030022	,0.022263	,0.015864	,0.010863	,0.007147	,0.004519	,0.002745	,0.001603	,0.000899	,0.000485
#elif GAUSSIAN_KERNEL_SIZE_17 // sigma 4
0.014076	,0.022439	,0.033613	,0.047318	,0.062595	,0.077812	,0.090898	,0.099783	,0.102934	,0.099783	,0.090898	,0.077812	,0.062595	,0.047318	,0.033613	,0.022439	,0.014076
#elif GAUSSIAN_KERNEL_SIZE_9 // sigma 1
0.000229	,0.005977	,0.060598	,0.241732	,0.382928	,0.241732	,0.060598	,0.005977	,0.000229
#endif
};
groupshared IMAGE_LOAD_VEC SharedMemory[SHARED_MEM_SIZE];

#if IMAGE_IS_ARRAY
texture2DArray InputImageX;
texture2DArray InputImageY;
write IMAGE_FORMAT_TYPE image2DArray BlurImageX;
write IMAGE_FORMAT_TYPE image2DArray BlurImageY;
#else
texture2D InputImageX;
texture2D InputImageY;
write IMAGE_FORMAT_TYPE image2D BlurImageX;
write IMAGE_FORMAT_TYPE image2D BlurImageY;
#endif

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = SHARED_MEM_SIZE
shader
void
csMainX()
{
	// get full resolution and inverse full resolution
	ivec2 size = Viewport;

	// calculate offsets
	const int         tileStart = int(gl_WorkGroupID.x) * BLUR_TILE_WIDTH;
	const int           tileEnd = tileStart + BLUR_TILE_WIDTH;
	const int        apronStart = int(tileStart) - KERNEL_RADIUS;
	const int          apronEnd = tileEnd + KERNEL_RADIUS;

    const int x = max(0, min(apronStart + int(gl_LocalInvocationID.x), size.x - 1));
    const int y = int(gl_WorkGroupID.y);
    const uint z = gl_WorkGroupID.z;

    if (x >= Viewport.x || y >= Viewport.y)
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_VEC(0);
    else
    {
    // load into workgroup saved memory, this allows us to use the original pixel even though
    // we might have replaced it with the result from this thread!
#if IMAGE_IS_ARRAY
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageFetch2DArray(InputImageX, InputSampler, ivec3(x, y, z), 0));
#else
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageFetch2D(InputImageX, InputSampler, ivec2(x, y), 0));
#endif
    }
	groupMemoryBarrier();
	barrier();

	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.x));

	if (writePos < tileEndClamped)
	{
		IMAGE_LOAD_VEC blurTotal = IMAGE_LOAD_VEC(0);

		int i;
#pragma unroll
        for (i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
            int j = max(0, min(int(gl_LocalInvocationID.x) + i, SHARED_MEM_SIZE - 1));
            blurTotal += weights[i] * SharedMemory[j];
        }

#if IMAGE_IS_ARRAY
		imageStore(BlurImageX, ivec3(writePos, y, z), RESULT_TO_VEC4(blurTotal));
#else
        imageStore(BlurImageX, ivec2(writePos, y), RESULT_TO_VEC4(blurTotal));
#endif
	}
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = SHARED_MEM_SIZE
shader
void
csMainY()
{
	// get full resolution and inverse full resolution
	ivec2 size = Viewport;

	// calculate offsets
	const int         tileStart = int(gl_WorkGroupID.x) * BLUR_TILE_WIDTH;
	const int           tileEnd = tileStart + BLUR_TILE_WIDTH;
	const int        apronStart = int(tileStart) - KERNEL_RADIUS;
	const int          apronEnd = tileEnd + KERNEL_RADIUS;

    const int x = int(gl_WorkGroupID.y);
    const int y = max(0, min(apronStart + int(gl_LocalInvocationID.x), size.y - 1));
    const uint z = gl_WorkGroupID.z;

    if (x >= Viewport.x || y >= Viewport.y)
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_VEC(0);
    else
    {
    // load into workgroup saved memory, this allows us to use the original pixel even though
    // we might have replaced it with the result from this thread!
#if IMAGE_IS_ARRAY
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageFetch2DArray(InputImageY, InputSampler, ivec3(x, y, z), 0));
#else
        SharedMemory[gl_LocalInvocationID.x] = IMAGE_LOAD_SWIZZLE(imageFetch2D(InputImageY, InputSampler, ivec2(x, y), 0));
#endif
    }
	groupMemoryBarrier();
	barrier();

	const uint writePos = tileStart + gl_LocalInvocationID.x;
	const uint tileEndClamped = min(tileEnd, uint(size.y));

	if (writePos < tileEndClamped)
	{
		IMAGE_LOAD_VEC blurTotal = IMAGE_LOAD_VEC(0);

#pragma unroll
        for (int i = 0; i <= KERNEL_RADIUS * 2; ++i)
		{
            int j = max(0, min(int(gl_LocalInvocationID.x) + i, SHARED_MEM_SIZE - 1));
            blurTotal += weights[i] * SharedMemory[j];
        }

#if IMAGE_IS_ARRAY
		imageStore(BlurImageY, ivec3(x, writePos, z), RESULT_TO_VEC4(blurTotal));
#else
        imageStore(BlurImageY, ivec2(x, writePos), RESULT_TO_VEC4(blurTotal));
#endif
	}
}
