//------------------------------------------------------------------------------
//  msaaresolve.fxh
//  (C) 2022 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "../lib/shared.fxh"


#ifndef IMAGE_TYPE
#define IMAGE_TYPE_R32F
#endif

#ifdef IMAGE_TYPE_R32F
#define FORMAT r32f
#define TYPE float
#else
#error "Unknown format"
#endif

#ifndef SAMPLES
#define SAMPLES 4
#endif


#define METHOD_ZERO_SAMPLE 0
#define METHOD_AVG_SAMPLE 1
#define METHOD_MIN_SAMPLE 2
#define METHOD_MAX_SAMPLE 3
#ifndef METHOD
#define METHOD METHOD_ZERO_SAMPLE
#endif

write FORMAT image2D resolve;

constant ResolveBlock
{
    textureHandle resolveSource;
    uint width;
};

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 64
shader
void
main()
{
    TYPE result = TYPE(0);
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
    if (uv.x >= width)
        return;
#if METHOD == METHOD_ZERO_SAMPLE
    imageStore(resolve, uv, fetch2DMS(resolveSource, PointSampler, uv, 0));
#else
        for (int i = 0; i < SAMPLES; i++)
        {
            TYPE samp = fetch2DMS(resolveSource, PointSampler, uv, i);
    #if METHOD == METHOD_MAX_SAMPLE
            result = max(result, samp);
    #elif METHOD == METHOD_MIN_SAMPLE 
            result = min(result, samp);
    #elif METHOD == METHOD_AVG_SAMPLE
            result += samp;
    #endif
        }

    #ifdef METHOD == METHOD_AVG_SAMPLE
        imageStore(resolve, uv, result / SAMPLES);
    #else
        imageStore(resolve, uv, result);
    #endif

#endif
}