//------------------------------------------------------------------------------
//  histogram_cs.fx
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"

sampler2D ColorSource;

rw_buffer HistogramBuffer [ string Visibility = "CS"; ]
{
    // 
    uint Histogram[256];
};

constant HistogramConstants [ string Visibility = "CS"; ]
{
    ivec2 WindowOffset;
    ivec2 TextureSize;
    int Mip;
    float InvLogLuminanceRange;
    float MinLogLuminance;
};

groupshared int LocalHistogram[256];

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 256
shader
void
csClear()
{
    Histogram[gl_LocalInvocationIndex] = 0;
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = 256
shader
void
csMain()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    pixel += WindowOffset;

    // clear LDS histogram chunk, since we will execute 256 threads per workgroup, each thread could clear one value
    LocalHistogram[gl_LocalInvocationIndex] = 0;
    groupMemoryBarrier();

    // don't sample from outside the texture
    if (all(lessThan(pixel, TextureSize)))
    {
        vec4 color = texelFetch(ColorSource, pixel, Mip);
        float luminance = dot(color, Luminance);

        float logLuminance = 0.0f;
        if (luminance >= 1.0f)
        {
            logLuminance = clamp(((log2(luminance) - MinLogLuminance) * InvLogLuminanceRange), 0.0f, 1.0f);
        }

        // floor the value of luminance scaled to ubyte to find which bucket it belongs in
        int bucket = int(floor(logLuminance * 255.0f));
        atomicAdd(LocalHistogram[bucket], 1);
    }

    // synchronize and add to global atomic
    groupMemoryBarrier();

    // same as with clear, each thread will contribute exactly one bucket to the global histogram
    atomicAdd(Histogram[gl_LocalInvocationIndex], LocalHistogram[gl_LocalInvocationIndex]);
}

//------------------------------------------------------------------------------
/**
*/
program HistogramCategorize [ string Mask = "HistogramCategorize"; ]
{
    ComputeShader = csMain();
};

program HistogramClear [ string Mask = "HistogramClear"; ]
{
    ComputeShader = csClear();
};