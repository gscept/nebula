//------------------------------------------------------------------------------
//  histogram_cs.gpul
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.gpuh>
#include <lib/util.gpuh>

sampler2D ColorSource;

struct HistogramData
{
    Histogram: [256] u32;
};
uniform HistogramBuffer : *mutable HistogramData;

struct HistogramParams
{
    WindowOffset: i32x2;
    TextureSize: i32x2;
    Mip: i32;
    InvLogLuminanceRange: f32;
    MinLogLuminance: f32;
};
uniform HistogramConstants : *HistogramParams;

workgroup LocalHistogram : [256]i32;

//------------------------------------------------------------------------------
/**
*/
local_size_x(256)
entry_point
csClear() void
{
    Histogram[gl_LocalInvocationIndex] = 0;
}

//------------------------------------------------------------------------------
/**
*/
local_size_x(256)
entry_point
csMain() void
{
    const histogramIndex = computeGetIndexInWorkgroup();
    const pixel = ivec2(computeGetGlobalInvocationIndices().xy);
    pixel += HistogramConstants.WindowOffset;

    // clear LDS histogram chunk, since we will execute 256 threads per workgroup, each thread could clear one value
    LocalHistogram[histogramIndex] = 0;
    memoryBarrierWorkgroup();

    // don't sample from outside the texture
    if (all(pixel < TextureSize))
    {
        const color = textureFetch(ColorSource, pixel, Mip);
        const luminance = dot(color.rgb, Luminance);

        const logLuminance = 0.0f;
        if (luminance >= 1.0f)
        {
            logLuminance = clamp(((log2(luminance) - MinLogLuminance) * InvLogLuminanceRange), 0.0f, 1.0f);
        }

        // floor the value of luminance scaled to ubyte to find which bucket it belongs in
        const bucket = i32(floor(logLuminance * 255.0f));
        atomicAdd(LocalHistogram[bucket], 1, MemorySemantics.Relaxed);
    }

    // synchronize and add to global atomic
    memoryBarrierWorkgroup();

    // same as with clear, each thread will contribute exactly one bucket to the global histogram
    atomicAdd(Histogram[histogramIndex], LocalHistogram[histogramIndex], MemorySemantics.Relaxed);
}

//------------------------------------------------------------------------------
/**
*/
@Mask("HistogramCategorize")
program HistogramCategorize
{
    ComputeShader = csMain;
};

@Mask("HistogramClear")
program HistogramClear
{
    ComputeShader = csClear;
};